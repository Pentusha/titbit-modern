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
