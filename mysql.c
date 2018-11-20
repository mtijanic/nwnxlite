#include "nwnxlite.h"
#include "mysql.h"
#include "string.h"
#include "assert.h"

#define MAX_COLS 64
static struct {
	MYSQL   mysql;
	MYSQL_STMT *stmt;
	MYSQL_BIND params[MAX_COLS];
	MYSQL_RES  *result;
	char   *paramValues[MAX_COLS];
	size_t  paramCount;
	char    lastError[256];
	int     affectedRows;
	int    columns;
	int    currentRow;
	int    totalRows;
	char *resultBuffer[MAX_COLS];
	MYSQL_BIND binds[MAX_COLS];
	unsigned long lengths[MAX_COLS];
} sql;

#define STORE_ERROR() (strcpy(sql.lastError, mysql_error(&sql.mysql)))

void sql_init() {
	LOG_INFO("SQL init");
	mysql_init(&sql.mysql);

	if (!mysql_real_connect(&sql.mysql, cfg.host, cfg.username, cfg.password, cfg.database, 0, NULL, 0)) {
		LOG_ERROR("MySQL connect failed: %s", STORE_ERROR());
	}
}
void sql_shutdown() {
	mysql_close(&sql.mysql);
	LOG_INFO("SQL shutdown");
}
int sql_is_connected() {
	int res = mysql_query(&sql.mysql, "SELECT 1") == 0;
	// Need to read the result before running any other queries.
	mysql_free_result(mysql_store_result(&sql.mysql));
	return res;
}


void sql_destroy_prepared_query() {
	if (sql.result) {
		mysql_free_result(sql.result);
		sql.result = NULL;
	}
	mysql_stmt_free_result(sql.stmt);
	mysql_stmt_close(sql.stmt);
	sql.stmt = NULL;
}
int sql_prepare_query(const char *query) {
	LOG_INFO("SQL prepare query: %s", query);
	if (sql.stmt) {
		LOG_DEBUG("Destroying previous query");
		sql_destroy_prepared_query();
	}

	sql.stmt = mysql_stmt_init(&sql.mysql);
	if (!sql.stmt) {
		LOG_ERROR("Failed to initialize statement: %s", STORE_ERROR());
		return 0;
	}

	if (!mysql_stmt_prepare(sql.stmt, query, strlen(query))) {
		sql.paramCount = mysql_stmt_param_count(sql.stmt);
		LOG_DEBUG("Detected %d parameters.", sql.paramCount);
		assert(sql.paramCount < count_of(sql.params));
		return 1;
	}
	else {
		LOG_ERROR("Failed to prepare statement: %s", STORE_ERROR());
		mysql_stmt_close(sql.stmt);
		sql.stmt = NULL;
		return 0;
	}
}

int sql_execute_prepared_query() {
	static int query_id = 1;
	sql.affectedRows = -1;

	if (mysql_stmt_bind_param(sql.stmt, sql.params)) {
		LOG_ERROR("Failed to bind params: %s", STORE_ERROR());
		return 0;
	}

	if (!mysql_stmt_execute(sql.stmt)) {
		LOG_DEBUG("Successful query");
		sql.result = mysql_stmt_result_metadata(sql.stmt);
		if (sql.result) {
			sql.columns = mysql_num_fields(sql.result);
			sql.affectedRows = (int) mysql_affected_rows(&sql.mysql);

			memset(sql.binds, 0, sizeof(sql.binds));
			memset(sql.lengths, 0, sizeof(sql.lengths));
			for (int i = 0; i < sql.columns; i++)
				sql.binds[i].length = &sql.lengths[i];

			mysql_stmt_bind_result(sql.stmt, sql.binds);
		}
		return query_id++;
	}

	LOG_ERROR("Failed query: %s", STORE_ERROR());
	return 0;
}
int sql_ready_to_read_next_row() {
	return sql.stmt && (sql.affectedRows != -1) && (sql.currentRow != sql.totalRows);
}
int sql_read_next_row() {
	if (!sql.stmt)
		return 0;

	int fetchResult = mysql_stmt_fetch(sql.stmt);
	if (fetchResult == MYSQL_NO_DATA) {
		return 0;
	}
	else if (fetchResult == 1) {
		LOG_ERROR("Error executing mysql_stmt_fetch - error: '%s'", STORE_ERROR());
		return 0;
	}

	for (int i = 0; i < sql.columns; i++) {
		sql.lengths[i] += 128;
		sql.resultBuffer[i] = realloc(sql.resultBuffer[i], sql.lengths[i] + 1);
		//assert(sql.resultBuffer[i]);
		LOG_DEBUG("Column %d of %d, length %d; res: %p", i, sql.columns, sql.lengths[i], sql.resultBuffer[i]);
		
		sql.binds[i].buffer = sql.resultBuffer[i];
		sql.binds[i].buffer_length = sql.lengths[i];
		int fetched = mysql_stmt_fetch_column(sql.stmt, &sql.binds[i], i, 0);
		sql.resultBuffer[i][sql.lengths[i]] = 0;
		LOG_DEBUG("fetched = %d", fetched);
	}
	sql.currentRow++;
	return 1;
}

char *sql_read_data_in_active_row(int column) {
	if (!sql.stmt || column < 0 || column >= sql.columns)
		return "";

	char *value = sql.resultBuffer[column] ? sql.resultBuffer[column] : "";
	LOG_DEBUG("Reading column %d, value \"%s\"", column, value);
	return value;
}
int sql_get_affected_rows() {
	return sql.affectedRows;
}

char *sql_get_database_type() {
	return "MYSQL";
}
char *sql_get_last_error(int bClear) {
	(void)bClear; // meh.
	return sql.lastError;
}
int sql_get_prepared_query_param_count() {
	return sql.paramCount;
}

uint32_t sql_read_full_object_in_active_row(int column) {
	assert(!"not implemented for nwnxlite");
	LOG_ERROR("sql_read_full_object_in_active_row() not implemented for nwnxlite");
	return OBJECT_INVALID;
}

//
// APS wrappers
//

void SQLExecDirect(char *name, char *value) {
	if (sql_prepare_query(value))
		sql_execute_prepared_query();
}

char *SQLFetch(char *name) {
	return sql_read_next_row() ? "1" : "0";
}

char *SQLGetData(char *name) {
	int col;
	if (!sscanf(name, "SQLGetData!%d", &col) || col > MAX_COLS || col < 0)
		return "";

	static char buf[4192];
	strncpy(buf, sql_read_data_in_active_row(col), sizeof(buf));
	return buf;
}
