#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "detours.h"

static const char exename[] = "nwserver.exe";
static const char dllname[] = "nwnxlite.dll";
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	static char cmd[4192];
	snprintf(cmd, sizeof(cmd), "%s %s", exename, lpCmdLine);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);

	DetourCreateProcessWithDll(NULL, cmd, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi, dllname, NULL);

	return 0;
}