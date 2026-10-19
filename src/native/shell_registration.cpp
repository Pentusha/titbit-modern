#define LoadStringA SDKUnusedLoadStringA
#include <windows.h>
#include <shellapi.h>
#undef LoadStringA
#define OBSERVED_STRING_RELEASE_EXTERNAL
#include "engine_string.h"
extern "C" void __cdecl ShellExecuteScriptRegistrationTarget(const char *filename) {
 try {
 ObservedString target;
 target=filename;
 SHELLEXECUTEINFOA execute;
 execute.cbSize=sizeof(execute);
 execute.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;
 execute.lpVerb="open";
 execute.lpFile=target;
 execute.lpParameters=0;
 execute.lpDirectory=0;
 execute.nShow=SW_MAXIMIZE;
 ShellExecuteExA(&execute);
 } catch (...) {
 }
}
