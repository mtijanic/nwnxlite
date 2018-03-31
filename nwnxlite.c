#include "nwnxlite.h"
#include "windows.h"
#include "detours.h"
#include "stdio.h"

__declspec(dllexport) void dummy() { }

FILE *logfile;
struct cfg cfg;

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
CExoString (__fastcall *CNWSScriptVarTable__GetString)(CNWSScriptVarTable* thisPtr, int edx, CExoString* result, CExoString* name);
void       (__fastcall *CNWSScriptVarTable__SetString)(CNWSScriptVarTable* thisPtr, int edx, CExoString* name,   CExoString* value);

void SQLExecDirect(char *name, char *value);
char *SQLFetch(char *name);
char *SQLGetData(char *name);
struct {
	const char *cmd;
	SetStringHandler handler;
} set_string_handlers[] = {
	{ "SQLExecDirect", SQLExecDirect}
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
	{ "test", NULL }
};

uint32_t __fastcall hook_GetObject(CNWSScriptVarTable* thisPtr, int edx, CExoString* name) {
	LOG_INFO("GetObject(\"%s\")", name->m_sString);
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

CExoString __fastcall hook_GetString(CNWSScriptVarTable* thisPtr, int edx, CExoString* result, CExoString* name) {
	LOG_INFO("GetString(\"%s\")\n", name->m_sString);
	if (!strncmp(name->m_sString, "NWNXLITE!", 9)) {
		char *cmd = name->m_sString + 9;
		for (int i = 0; i < count_of(get_string_handlers); i++) {
			if (!strncmp(cmd, get_string_handlers[i].cmd, strlen(get_string_handlers[i].cmd))) {
				*result = make_cexostr(get_string_handlers[i].handler(cmd));
				return *result;
			}
		}
	}
	return CNWSScriptVarTable__GetString(thisPtr, edx, result, name);
}
void __fastcall hook_SetString(CNWSScriptVarTable* thisPtr, int edx, CExoString* name, CExoString* value)
{
	LOG_INFO("SetString(\"%s\", \"%s\")\n", name->m_sString, value->m_sString);
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
	// Offsets for 8166 nwserver.exe
	// TODO: Load dynamically for auto updating
	const uintptr_t NWNXEntryPoint_base = 0x0040B7B0;
	const uintptr_t CNWSScriptVarTable__GetObject_base = 0x005A2A00;
	const uintptr_t CNWSScriptVarTable__GetString_base = 0x005A2A60;
	const uintptr_t CNWSScriptVarTable__SetString_base = 0x005A3280;

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