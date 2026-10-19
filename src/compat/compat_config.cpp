/* Compat settings (TITBIT_COMPAT): environment variable TITBIT_COMPAT_<KEY>,
 * else titbit_compat.ini next to the executable ([compat] key=value), else
 * the default. Values are compared case-insensitively. */
#define TITBIT_COMPAT_IMPLEMENTATION
#include <windows.h>
#include "compat_internal.h"

const char *CompatSetting(const char *key, const char *fallback, char *buffer, unsigned size) {
 char name[64] = "TITBIT_COMPAT_";
 lstrcatA(name, key);
 CharUpperA(name + 14);
 if (GetEnvironmentVariableA(name, buffer, size) > 0 && buffer[0]) return buffer;
 WCHAR ini[MAX_PATH];
 DWORD n = GetModuleFileNameW(0, ini, MAX_PATH);
 if (!n || n >= MAX_PATH) { lstrcpynA(buffer, fallback, size); return buffer; }
 while (n && ini[n - 1] != L'\\' && ini[n - 1] != L'/') n--;
 ini[n] = 0;
 lstrcatW(ini, L"titbit_compat.ini");
 WCHAR wide_key[64], wide_fallback[512], value[512];
 MultiByteToWideChar(CP_ACP, 0, key, -1, wide_key, 64);
 MultiByteToWideChar(CP_ACP, 0, fallback, -1, wide_fallback, 512);
 GetPrivateProfileStringW(L"compat", wide_key, wide_fallback, value, 512, ini);
 if (!WideCharToMultiByte(CP_ACP, 0, value, -1, buffer, size, 0, 0)) {
  lstrcpynA(buffer, fallback, size);
 }
 return buffer;
}

bool CompatSettingIs(const char *key, const char *fallback, const char *value) {
 char buffer[64];
 return lstrcmpiA(CompatSetting(key, fallback, buffer, sizeof(buffer)), value) == 0;
}

void CompatLog(const char *module, const char *what, long value) {
 static int enabled = -1;
 if (enabled < 0) enabled = CompatSettingIs("log", "0", "1");
 if (!enabled) return;
 char line[200];
 int n = wsprintfA(line, "titbit compat %s: %s 0x%08lx\r\n", module, what, value);
 OutputDebugStringA(line);
 HANDLE f = CreateFileA("titbit_compat.log", GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
 if (f != INVALID_HANDLE_VALUE) {
  DWORD written;
  SetFilePointer(f, 0, 0, FILE_END);
  WriteFile(f, line, n, &written, 0);
  CloseHandle(f);
 }
}
