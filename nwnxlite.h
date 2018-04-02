#pragma once
#define _CRT_SECURE_NO_WARNINGS 1
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

extern FILE *logfile;
extern int loglevel;
#define LOG_INIT(logname)   (logfile = fopen(logname, "w"))
#define LOG_DEBUG(fmt, ...) if (loglevel >= 2) { fprintf(logfile, "[DEBUG] " fmt "\n", ##__VA_ARGS__), fflush(logfile); } else
#define LOG_INFO(fmt, ...)  if (loglevel >= 1) { fprintf(logfile, "[INFO]  " fmt "\n", ##__VA_ARGS__), fflush(logfile); } else
#define LOG_ERROR(fmt, ...) (fprintf(logfile, "[ERROR] " fmt "\n", ##__VA_ARGS__), fflush(logfile))
#define LOG_CLOSE()         (fclose(logfile))

#define count_of(arr) (sizeof(arr)/sizeof(arr[0]))

extern struct cfg {
	char host[256];
	char username[256];
	char password[256];
	char database[256];
} cfg;

typedef struct CExoString {
	char* m_sString;
	uint32_t m_nBufferLength;
} CExoString;

static inline CExoString make_cexostr(const char *str) {
	CExoString c;// = malloc(sizeof(*c));

	c.m_nBufferLength = strlen(str) + 1;
	c.m_sString = malloc(c.m_nBufferLength);
	strcpy(c.m_sString, str);
	LOG_DEBUG("makestr: %s | %s ; %p | %p | %d", str, c.m_sString, str, c.m_sString, c.m_nBufferLength);
	return c;
}


#define OBJECT_INVALID 0x7F000000
typedef void *CNWSScriptVarTable;

typedef void(*SetStringHandler)(char *name, char *value);
typedef char*(*GetStringHandler)(char *name);
typedef uint32_t(*GetObjectHandler)(char *name);

// APS wrappers
void SQLExecDirect(char *name, char *value);
char *SQLFetch(char *name);
char *SQLGetData(char *name);


void sql_init();
void sql_shutdown();
int sql_is_connected();
void sql_destroy_prepared_query();
int sql_prepare_query(const char *query);
int sql_execute_prepared_query();
int sql_ready_to_read_next_row();
int sql_read_next_row();
char *sql_read_data_in_active_row(int column);
int sql_get_affected_rows();
char *sql_get_database_type();
char *sql_get_last_error(int bClear);
int sql_get_prepared_query_param_count();
uint32_t sql_read_full_object_in_active_row(int column);