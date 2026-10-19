/* CD check (TITBIT_COMPAT).
 *
 * CheckGameDisc (src/native/platform_startup.cpp) looks for a CD-ROM drive with
 * the volume label "TITBIT" and otherwise shows "Не найден CD-ROM с игрой..."
 * and quits. The game itself reads everything from its install folder
 * (SetArchiveRoot(".\\"), QUEST.EZD), the disc is only checked.
 *
 * Setting cd (default "folder"): when the game data (QUEST.EZD) is in the
 * current folder, the first drive the check probes is reported as a CD-ROM
 * labelled TITBIT, so the original check passes unchanged. cd=disc keeps the
 * original behaviour (a real disc or mounted image is required).
 */
#define TITBIT_COMPAT_IMPLEMENTATION
#include <windows.h>
#include "compat.h"
#include "compat_internal.h"
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#ifndef FILE_READ_ONLY_VOLUME
#define FILE_READ_ONLY_VOLUME 0x00080000
#endif

static char faked_root[4];            /* the root reported as the game CD */

static bool UseInstallFolder() {
 if (!CompatSettingIs("cd", "folder", "folder")) return false;
 DWORD attributes = GetFileAttributesA("QUEST.EZD");
 return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

extern "C" int __stdcall CompatGetVolumeInformationA(const char *root, char *label, unsigned long label_size,
                                                     unsigned long *serial, unsigned long *max_component,
                                                     unsigned long *flags, char *fs_name, unsigned long fs_size) {
 if (root && (faked_root[0] == 0 || lstrcmpiA(root, faked_root) == 0) && UseInstallFolder()) {
  lstrcpynA(faked_root, root, sizeof(faked_root));
  CompatLog("cd", "install folder stands in for the game CD, drive letter", root[0]);
  if (label && label_size) lstrcpynA(label, "TITBIT", label_size);
  if (serial) *serial = 0;
  if (max_component) *max_component = 255;
  if (flags) *flags = FILE_READ_ONLY_VOLUME;
  if (fs_name && fs_size) lstrcpynA(fs_name, "CDFS", fs_size);
  return TRUE;
 }
 return GetVolumeInformationA(root, label, label_size, serial, max_component, flags, fs_name, fs_size);
}

extern "C" unsigned __stdcall CompatGetDriveTypeA(const char *root) {
 if (root && faked_root[0] && lstrcmpiA(root, faked_root) == 0) return DRIVE_CDROM;
 return GetDriveTypeA(root);
}

/* The game reads everything from QUEST.EZD in its own folder (even with
 * cd=disc, the disc is only checked) and crashes later without it. On the first
 * start of the portable package open TitbitSetup.exe next to the game so the
 * player can import the data; without it, say what is missing. Returns 1 when
 * the game must not start. */
extern "C" int __stdcall CompatOpenSetupWithoutData(void) {
 if (GetFileAttributesA("QUEST.EZD") != INVALID_FILE_ATTRIBUTES) return 0;
 WCHAR setup[MAX_PATH + 16];
 DWORD n = GetModuleFileNameW(0, setup, MAX_PATH);
 if (n && n < MAX_PATH) {
  while (n && setup[n - 1] != L'\\' && setup[n - 1] != L'/') --n;
  lstrcpyW(setup + n, L"TitbitSetup.exe");
  STARTUPINFOW startup; PROCESS_INFORMATION process;
  ZeroMemory(&startup, sizeof(startup)); startup.cb = sizeof(startup);
  if (GetFileAttributesW(setup) != INVALID_FILE_ATTRIBUTES &&
      CreateProcessW(setup, 0, 0, 0, FALSE, 0, 0, 0, &startup, &process)) {
   CloseHandle(process.hThread); CloseHandle(process.hProcess);
   CompatLog("cd", "no QUEST.EZD: opened TitbitSetup.exe", 0);
   return 1;
  }
 }
 /* "Ядерный Титбит" / "Не найдены данные игры (QUEST.ezd). Запустите TitbitSetup.exe ..."
  * as UTF-16 escapes: VC6 reads source files in the ANSI code page. */
 MessageBoxW(0,
  L"\x041d\x0435 \x043d\x0430\x0439\x0434\x0435\x043d\x044b \x0434\x0430\x043d\x043d\x044b\x0435 \x0438\x0433\x0440\x044b (QUEST.ezd).\n\n"
  L"\x0417\x0430\x043f\x0443\x0441\x0442\x0438\x0442\x0435 TitbitSetup.exe \x0438 \x0434\x043e\x0431\x0430\x0432\x044c\x0442\x0435 \x0444\x0430\x0439\x043b\x044b \x0441\x043e \x0441\x0432\x043e\x0435\x0433\x043e \x0434\x0438\x0441\x043a\x0430, \x043e\x0431\x0440\x0430\x0437\x0430 \x0438\x043b\x0438 \x0438\x0437 \x0443\x0441\x0442\x0430\x043d\x043e\x0432\x043b\x0435\x043d\x043d\x043e\x0439 \x0438\x0433\x0440\x044b.",
  L"\x042f\x0434\x0435\x0440\x043d\x044b\x0439 \x0422\x0438\x0442\x0431\x0438\x0442", MB_OK | MB_ICONINFORMATION);
 CompatLog("cd", "no QUEST.EZD and no TitbitSetup.exe: told the player", 0);
 return 1;
}
