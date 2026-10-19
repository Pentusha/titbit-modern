/* Modern presentation is separate from the reconstructed game. */
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#define TITBIT_COMPAT_IMPLEMENTATION
#include <windows.h>
#include <GL/gl.h>
#include "compat.h"
#include "compat_internal.h"
#include "display_geometry.h"

static HWND game_window;
static WNDPROC game_proc;
static RECT game_rect, window_rect;
static bool is_fullscreen, stretch;
static int logical_width = 1024, logical_height = 768;
static bool exclusive_checked, exclusive;
static bool Exclusive() {
 if (!exclusive_checked) { exclusive = CompatSettingIs("display", "fullscreen", "exclusive"); exclusive_checked = true; }
 return exclusive;
}
static bool IsGameClass(const char *name) {
 return name && (unsigned long)name > 0xffff && lstrcmpiA(name, "OpenGL") == 0;
}
extern "C" void __stdcall CompatPlatformStartup(void) {
 HMODULE user = GetModuleHandleA("user32.dll");
 typedef BOOL (WINAPI *DpiContext)(HANDLE);
 typedef BOOL (WINAPI *DpiAware)(void);
 DpiContext context = (DpiContext)GetProcAddress(user, "SetProcessDpiAwarenessContext");
 if (!context || !context((HANDLE)-4)) {
  DpiAware aware = (DpiAware)GetProcAddress(user, "SetProcessDPIAware");
  if (aware) aware();
 }
 WCHAR folder[MAX_PATH];
 DWORD n = GetModuleFileNameW(0, folder, MAX_PATH);
 if (n && n < MAX_PATH) {
  while (n && folder[n - 1] != L'\\' && folder[n - 1] != L'/') --n;
  if (n) { folder[n] = 0; SetCurrentDirectoryW(folder); }
 }
}
extern "C" int __stdcall CompatPrepareGame(int *width, int *height) {
 if (Exclusive() || CompatSettingIs("setup", "modern", "legacy")) return 0;
 *width = 1024; *height = 768;
 return 1;
}
extern "C" void __stdcall CompatGameCanvas(int *width, int *height) {
 if (!Exclusive()) { *width = 1024; *height = 768; }
 logical_width = *width; logical_height = *height;
}
static RECT MonitorRect(HWND window, bool work) {
 POINT cursor = {0, 0};
 GetCursorPos(&cursor);
 HMONITOR monitor = window ? MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST)
                           : MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
 MONITORINFO info; info.cbSize = sizeof(info);
 if (GetMonitorInfoA(monitor, &info)) return work ? info.rcWork : info.rcMonitor;
 RECT result = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
 return result;
}
static void WindowSize(int *width, int *height) {
 char text[64];
 CompatSetting("resolution", "1024x768", text, sizeof(text));
 *width = 1024; *height = 768;
 if (lstrcmpiA(text, "desktop") == 0 || lstrcmpiA(text, "auto") == 0) {
  RECT area = MonitorRect(0, true);
  *width = (area.right - area.left) * 4 / 5;
  *height = (area.bottom - area.top) * 4 / 5;
  return;
 }
 int w = 0, h = 0;
 const char *p = text;
 while (*p >= '0' && *p <= '9' && w <= 16384) w = w * 10 + (*p++ - '0');
 if (*p != 'x') return;
 ++p;
 while (*p >= '0' && *p <= '9' && h <= 16384) h = h * 10 + (*p++ - '0');
 if (*p == 0 && w >= 640 && h >= 480 && w <= 16384 && h <= 16384) { *width = w; *height = h; }
}
static RECT WindowPlacement(DWORD style, DWORD ex_style) {
 int width, height; WindowSize(&width, &height);
 RECT work = MonitorRect(game_window, true);
 RECT frame = {0, 0, 0, 0};
 AdjustWindowRectEx(&frame, style, FALSE, ex_style);
 int extra_w = frame.right - frame.left, extra_h = frame.bottom - frame.top;
 if (width > work.right - work.left - extra_w) width = work.right - work.left - extra_w;
 if (height > work.bottom - work.top - extra_h) height = work.bottom - work.top - extra_h;
 RECT result = {0, 0, width + extra_w, height + extra_h};
 OffsetRect(&result, work.left + ((work.right - work.left) - result.right) / 2,
                      work.top + ((work.bottom - work.top) - result.bottom) / 2);
 return result;
}
static CompatViewport Viewport(HWND window) {
 RECT rect = {0, 0, logical_width, logical_height};
 if (window) GetClientRect(window, &rect);
 return CompatFitViewport(rect.right, rect.bottom, logical_width, logical_height, stretch);
}
static void ToggleFullscreen() {
 if (!game_window || Exclusive()) return;
 is_fullscreen = !is_fullscreen;
 DWORD ex_style = (DWORD)GetWindowLongA(game_window, GWL_EXSTYLE) & ~WS_EX_TOPMOST;
 DWORD style = is_fullscreen ? WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
                            : WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
 if (is_fullscreen) { GetWindowRect(game_window, &window_rect); game_rect = MonitorRect(game_window, false); }
 else game_rect = window_rect;
 SetWindowLongA(game_window, GWL_STYLE, style);
 SetWindowLongA(game_window, GWL_EXSTYLE, ex_style);
 SetWindowPos(game_window, HWND_NOTOPMOST, game_rect.left, game_rect.top,
              game_rect.right - game_rect.left, game_rect.bottom - game_rect.top,
              SWP_FRAMECHANGED | SWP_NOACTIVATE);
 InvalidateRect(game_window, 0, FALSE);
}
static LRESULT CALLBACK ModernWindowProc(HWND window, UINT message, WPARAM w, LPARAM l) {
 if ((message == WM_SYSKEYDOWN && w == VK_RETURN && (l & 0x20000000)) ||
     (message == WM_KEYDOWN && w == VK_F11)) {
  if (!(l & 0x40000000)) ToggleFullscreen();
  return 0;
 }
 if ((message == WM_SYSKEYUP && w == VK_RETURN) || (message == WM_KEYUP && w == VK_F11)) return 0;
 if (message == 0x02e0) { // WM_DPICHANGED
  RECT next = is_fullscreen ? MonitorRect(window, false) : *(RECT *)l;
  SetWindowPos(window, 0, next.left, next.top, next.right - next.left, next.bottom - next.top,
               SWP_NOZORDER | SWP_NOACTIVATE);
  return 0;
 }
 if (message == WM_ERASEBKGND) return 1;
 if (message >= WM_MOUSEMOVE && message <= WM_MBUTTONDBLCLK) {
  CompatViewport vp = Viewport(window);
  int x = (short)LOWORD(l), y = (short)HIWORD(l), lx, ly;
  bool inside = CompatMapMouse(vp, x, y, logical_width, logical_height, &lx, &ly);
  if (!inside && message != WM_MOUSEMOVE) return 0;
  l = MAKELPARAM((short)lx, (short)ly);
 }
 return CallWindowProcA(game_proc, window, message, w, l);
}
extern "C" long __stdcall CompatChangeDisplaySettingsA(void *mode, unsigned long flags) {
 if (Exclusive()) return ChangeDisplaySettingsA((DEVMODEA *)mode, flags);
 return DISP_CHANGE_SUCCESSFUL;
}
extern "C" void *__stdcall CompatCreateWindowExA(unsigned long ex_style, const char *class_name, const char *title,
 unsigned long style, int x, int y, int width, int height, void *parent, void *menu, void *instance, void *param) {
 bool modern_game = !Exclusive() && !parent && IsGameClass(class_name);
 if (modern_game) {
  stretch = CompatSettingIs("aspect", "fit", "stretch");
  is_fullscreen = CompatSettingIs("display", "fullscreen", "fullscreen");
  bool caption = CompatSettingIs("display", "fullscreen", "windowed");
  ex_style &= ~(unsigned long)WS_EX_TOPMOST;
  style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | (caption ? WS_OVERLAPPEDWINDOW : WS_POPUP);
  window_rect = WindowPlacement(WS_OVERLAPPEDWINDOW, ex_style);
  game_rect = is_fullscreen ? MonitorRect(0, false) : WindowPlacement(style, ex_style);
  x = game_rect.left; y = game_rect.top;
  width = game_rect.right - x; height = game_rect.bottom - y;
 }
 HWND window = CreateWindowExA(ex_style, class_name, title, style, x, y, width, height,
                               (HWND)parent, (HMENU)menu, (HINSTANCE)instance, param);
 if (modern_game && window) {
  game_window = window;
  game_proc = (WNDPROC)SetWindowLongA(window, GWL_WNDPROC, (LONG)ModernWindowProc);
  CompatLog("display", "viewport width", Viewport(window).width);
  CompatLog("display", "viewport height", Viewport(window).height);
 }
 return window;
}
extern "C" int __stdcall CompatMoveWindow(void *window, int x, int y, int width, int height, int repaint) {
 if (!Exclusive() && window && (HWND)window == game_window) return TRUE;
 return MoveWindow((HWND)window, x, y, width, height, repaint);
}
extern "C" int __stdcall CompatShowWindow(void *window, int command) {
 if (!Exclusive() && window && (HWND)window == game_window) {
  if (command == SW_MAXIMIZE) return TRUE;
  if (command == SW_SHOWMINIMIZED || command == SW_MINIMIZE || command == SW_SHOWMINNOACTIVE) return TRUE;
 }
 return ShowWindow((HWND)window, command);
}
extern "C" void __stdcall CompatGLViewport(int x, int y, int width, int height) {
 if (!game_window || Exclusive()) { glViewport(x, y, width, height); return; }
 CompatViewport vp = Viewport(game_window);
 glViewport(vp.x, vp.y, vp.width, vp.height);
}
extern "C" void __stdcall CompatBeginFrame(void) {
 if (!game_window || Exclusive()) return;
 CompatViewport vp = Viewport(game_window);
 if (vp.width <= 0 || vp.height <= 0) return;
 glPushAttrib(GL_COLOR_BUFFER_BIT | GL_SCISSOR_BIT | GL_DEPTH_BUFFER_BIT);
 glDisable(GL_SCISSOR_TEST);
 glColorMask(TRUE, TRUE, TRUE, TRUE); glDepthMask(TRUE);
 glClearColor(0, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 glPopAttrib();
 glViewport(vp.x, vp.y, vp.width, vp.height);
 glScissor(vp.x, vp.y, vp.width, vp.height);
 glEnable(GL_SCISSOR_TEST);
}

