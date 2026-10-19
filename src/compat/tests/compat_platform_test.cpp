// Test for compat_cd.cpp / compat_display.cpp.
//   compat_platform_test <game folder containing QUEST.EZD>
// Runs the original CheckGameDisc drive loop against the compat functions and
// creates the game's popup window through CompatCreateWindowExA.
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../compat.h"

static int CheckGameDiscLoop() {      // same loop as src/native/platform_startup.cpp
 char root[] = "C:\\";
 DWORD volumeFlags;
 char label[2048];
 char drive;
 for (drive = 'C'; drive < '['; ++drive) {
  root[0] = drive;
  if (GetVolumeInformationA(root, label, sizeof(label), 0, &volumeFlags, &volumeFlags, 0, 0)
      && !strcmp(label, "TITBIT") && GetDriveTypeA(root) == 5) break;
 }
 return drive == '[' ? 1 : 0;
}

int main(int argc, char **argv) {
 int failures = 0;
 char here[MAX_PATH];
 GetCurrentDirectoryA(MAX_PATH, here);
 int without = CheckGameDiscLoop();
 printf("cd check outside the game folder: %s\n", without ? "fails (expected)" : "passes");
 if (argc > 1) SetCurrentDirectoryA(argv[1]);
 int with = CheckGameDiscLoop();
 printf("cd check in the game folder: %s\n", with ? "FAILS" : "passes (expected)");
 failures += with;
 SetCurrentDirectoryA(here);

 HWND window = (HWND)CreateWindowExA(0x40008, "STATIC", "Titbit", 0x86000000, 0, 0, 640, 480, 0, 0, GetModuleHandleA(0), 0);
 RECT client, outer;
 GetClientRect(window, &client);
 GetWindowRect(window, &outer);
 printf("window: client %ldx%ld at (%ld,%ld), exstyle %08lx\n", client.right, client.bottom, outer.left, outer.top,
        (unsigned long)GetWindowLongA(window, GWL_EXSTYLE));
 if (client.right != 640 || client.bottom != 480) { printf("  client size WRONG\n"); failures++; }
 DestroyWindow(window);
 printf("ChangeDisplaySettingsA(0,0) -> %ld\n", ChangeDisplaySettingsA(0, 0));
 printf(failures ? "FAILED\n" : "OK\n");
 return failures;
}
