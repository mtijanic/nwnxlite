#include "nwnxlite.h"
#include "windows.h"
#include "detours.h"
#include "stdio.h"

__declspec(dllexport) void dummy() { }

FILE *logfile;
struct cfg cfg;

int loglevel = 1;

void parse_config() {
	FILE *f = fopen("nwnxlite.ini", "r");
	if (!f) {
		LOG_INFO("nwnxlite.ini not found, using default values.");
		strcpy(cfg.host, "localhost");
		strcpy(cfg.username, "root");
		strcpy(cfg.password, "root");
		strcpy(cfg.database, "nwn");
		return;
	}

	char line[2048];
	while (fgets(line, sizeof(line), f)) {
		sscanf(line, "HOST=%s", cfg.host) ||
			sscanf(line, "USERNAME=%s", cfg.username) ||
			sscanf(line, "PASSWORD=%s", cfg.password) ||
			sscanf(line, "DATABASE=%s", cfg.database);
	}
	fclose(f);
}

// Original functions
uint32_t   (__fastcall *CNWSScriptVarTable__GetObject)(CNWSScriptVarTable* thisPtr, int edx, CExoString* name);
CExoString*(__fastcall *CNWSScriptVarTable__GetString)(CNWSScriptVarTable* thisPtr, int edx, CExoString* result, CExoString* name);
void       (__fastcall *CNWSScriptVarTable__SetString)(CNWSScriptVarTable* thisPtr, int edx, CExoString* name,   CExoString* value);

void SQLExecDirect(char *name, char *value);
char *SQLFetch(char *name);
char *SQLGetData(char *name);
void SetLogLevel(char *name, char *value) {
	if (!sscanf(value, "%d", &loglevel))
		LOG_ERROR("SetLogLevel failed for level %s, keeping old loglevel=%d", value, loglevel);
}

struct {
	const char *cmd;
	SetStringHandler handler;
} set_string_handlers[] = {
	{ "SQLExecDirect", SQLExecDirect},
	{ "SetLogLevel", SetLogLevel }
};
struct {
	const char *cmd;
	GetStringHandler handler;
} get_string_handlers[] = {
	{ "SQLFetch", SQLFetch },
	{ "SQLGetData", SQLGetData },
};
struct {
	const char *cmd;
	GetObjectHandler handler;
} get_object_handlers[] = {
	{ "CRASH", NULL }
};

uint32_t __fastcall hook_GetObject(CNWSScriptVarTable* thisPtr, int edx, CExoString* name) {
	LOG_DEBUG("GetObject(\"%s\")", name->m_sString);
	if (!strncmp(name->m_sString, "NWNXLITE!", 9)) {
		char *cmd = name->m_sString + 9;
		for (int i = 0; i < count_of(get_object_handlers); i++) {
			if (!strncmp(cmd, get_object_handlers[i].cmd, strlen(get_object_handlers[i].cmd))) {
				return get_object_handlers[i].handler(cmd);
			}
		}
	}
	return CNWSScriptVarTable__GetObject(thisPtr, edx, name);
}

CExoString *__fastcall hook_GetString(CNWSScriptVarTable* thisPtr, int edx, CExoString* result, CExoString* name) {
	LOG_DEBUG("GetString(\"%s\")", name->m_sString);
	if (!strncmp(name->m_sString, "NWNXLITE!", 9)) {
		char *cmd = name->m_sString + 9;
		for (int i = 0; i < count_of(get_string_handlers); i++) {
			if (!strncmp(cmd, get_string_handlers[i].cmd, strlen(get_string_handlers[i].cmd))) {
				CExoString cexostr = make_cexostr(get_string_handlers[i].handler(cmd));
				LOG_DEBUG("Returned %s (%p), length %d, result:%p", cexostr.m_sString, cexostr.m_sString, cexostr.m_nBufferLength, result);
				CNWSScriptVarTable__SetString(thisPtr, edx, name, &cexostr);
				free(cexostr.m_sString);
				break;
			}
		}
	}
	return CNWSScriptVarTable__GetString(thisPtr, edx, result, name);
}
void __fastcall hook_SetString(CNWSScriptVarTable* thisPtr, int edx, CExoString* name, CExoString* value)
{
	LOG_DEBUG("SetString(\"%s\", \"%s\")", name->m_sString, value->m_sString);
	if (!strncmp(name->m_sString, "NWNXLITE!", 9)) {
		char *cmd = name->m_sString + 9;
		for (int i = 0; i < count_of(set_string_handlers); i++) {
			if (!strncmp(cmd, set_string_handlers[i].cmd, strlen(set_string_handlers[i].cmd))) {
				set_string_handlers[i].handler(cmd, value->m_sString);
				return;
			}
		}
	}
	CNWSScriptVarTable__SetString(thisPtr, edx, name, value);
}


void init_nwn_hooks()
{
	FILE *f = fopen("nwnxlite-offsets.txt", "r");
	if (f == NULL)
	{
		LOG_ERROR("Can't find offsets file nwnxlite-offsets.txt");
		return;
	}

	uintptr_t NWNXEntryPoint_base = 0;
	uintptr_t CNWSScriptVarTable__GetObject_base = 0;
	uintptr_t CNWSScriptVarTable__GetString_base = 0;
	uintptr_t CNWSScriptVarTable__SetString_base = 0;

	char line[2048];
	while (fgets(line, sizeof(line), f)) {
		sscanf(line, "NWNXEntryPoint=%x", &NWNXEntryPoint_base) ||
			sscanf(line, "CNWSScriptVarTable__GetObject=%x", &CNWSScriptVarTable__GetObject_base) ||
			sscanf(line, "CNWSScriptVarTable__GetString=%x", &CNWSScriptVarTable__GetString_base) ||
			sscanf(line, "CNWSScriptVarTable__SetString=%x", &CNWSScriptVarTable__SetString_base);
	}
	fclose(f);

	if (NWNXEntryPoint_base == 0) LOG_ERROR("NWNXEntryPoint offset not found");
	if (CNWSScriptVarTable__GetObject_base == 0) LOG_ERROR("CNWSScriptVarTable__GetObject offset not found");
	if (CNWSScriptVarTable__GetString_base == 0) LOG_ERROR("CNWSScriptVarTable__GetString offset not found");
	if (CNWSScriptVarTable__SetString_base == 0) LOG_ERROR("CNWSScriptVarTable__SetString offset not found");

	LOG_INFO("Using offsets: NWNXEntryPoint=0x08%x; CNWSScriptVarTable__GetObject=0x%08x; CNWSScriptVarTable__GetString=0x%08x; CNWSScriptVarTable__SetString=0x%08x;",
		NWNXEntryPoint_base, CNWSScriptVarTable__GetObject_base, CNWSScriptVarTable__GetString_base, CNWSScriptVarTable__SetString_base);

	uintptr_t real = (uintptr_t)GetProcAddress(GetModuleHandle(NULL), "NWNXEntryPoint");
	uintptr_t aslr_offset = real - NWNXEntryPoint_base;

	CNWSScriptVarTable__GetObject = (void*)(CNWSScriptVarTable__GetObject_base + aslr_offset);
	CNWSScriptVarTable__GetString = (void*)(CNWSScriptVarTable__GetString_base + aslr_offset);
	CNWSScriptVarTable__SetString = (void*)(CNWSScriptVarTable__SetString_base + aslr_offset);

	DetourTransactionBegin();
	  DetourUpdateThread(GetCurrentThread());
	  DetourAttach((void**)&CNWSScriptVarTable__GetObject, hook_GetObject);
	  DetourAttach((void**)&CNWSScriptVarTable__GetString, hook_GetString);
	  DetourAttach((void**)&CNWSScriptVarTable__SetString, hook_SetString);
	DetourTransactionCommit();
}


BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved) {
	if (dwReason == DLL_PROCESS_ATTACH)	{
		LOG_INIT("nwnxlite.log");
		LOG_INFO("Init start.");

		parse_config();
		init_nwn_hooks();
		sql_init();
		LOG_INFO("Init done.");
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
		LOG_INFO("Shutting down.");
		sql_shutdown();
		LOG_CLOSE();
	}

	return TRUE;
}