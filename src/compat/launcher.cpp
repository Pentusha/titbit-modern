/* Standalone modern settings UI. Not part of the original game executable. */
#define UNICODE
#define _UNICODE
#include <windows.h>
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#pragma intrinsic(memset)
static HWND mode_box, size_box, ratio_box, size_label;
static WCHAR game_folder[MAX_PATH], config_path[MAX_PATH];
static const WCHAR *sizes[] = {L"auto", L"800x600", L"1024x768", L"1280x960",
 L"1440x1080", L"1600x1200", L"1920x1080", L"1920x1440", L"2560x1440", L"3840x2160"};
static HINSTANCE app;
static int dpi_value = 96;
static HFONT app_font;
static HWND Control(HWND parent, const WCHAR *kind, const WCHAR *text, DWORD style,
                    int x, int y, int width, int height, int id) {
 HWND h = CreateWindowW(kind, text, WS_CHILD | WS_VISIBLE | style,
                         MulDiv(x,dpi_value,96), MulDiv(y,dpi_value,96),
                         MulDiv(width,dpi_value,96), MulDiv(height,dpi_value,96),
                         parent, (HMENU)id, app, 0);
 SendMessageW(h, WM_SETFONT, (WPARAM)app_font, TRUE);
 return h;
}
static bool Save(HWND window) {
 int mode = (int)SendMessageW(mode_box, CB_GETCURSEL, 0, 0);
 int size = (int)SendMessageW(size_box, CB_GETCURSEL, 0, 0);
 bool fit = SendMessageW(ratio_box, BM_GETCHECK, 0, 0) == BST_CHECKED;
 if (size < 0 || size >= sizeof(sizes)/sizeof(sizes[0])) size = 2;
 BOOL ok = WritePrivateProfileStringW(L"compat", L"display", mode == 1 ? L"windowed" : L"fullscreen", config_path);
 ok &= WritePrivateProfileStringW(L"compat", L"resolution", sizes[size], config_path);
 ok &= WritePrivateProfileStringW(L"compat", L"aspect", fit ? L"fit" : L"stretch", config_path);
 ok &= WritePrivateProfileStringW(L"compat", L"cd", L"folder", config_path);
 ok &= WritePrivateProfileStringW(L"compat", L"setup", L"modern", config_path);
 if (!ok) MessageBoxW(window, L"\x041d\x0435\x0020\x0443\x0434\x0430\x043b\x043e\x0441\x044c\x0020\x0441\x043e\x0445\x0440\x0430\x043d\x0438\x0442\x044c\x0020\x043d\x0430\x0441\x0442\x0440\x043e\x0439\x043a\x0438\x002e\x0020\x041f\x0440\x043e\x0432\x0435\x0440\x044c\x0442\x0435\x0020\x0434\x043e\x0441\x0442\x0443\x043f\x0020\x043a\x0020\x043f\x0430\x043f\x043a\x0435\x0020\x0438\x0433\x0440\x044b\x002e",
                      L"\x042f\x0434\x0435\x0440\x043d\x044b\x0439\x0020\x0442\x0438\x0442\x0431\x0438\x0442", MB_OK | MB_ICONERROR);
 return ok != FALSE;
}
static bool Play(HWND window) {
 WCHAR file[MAX_PATH], command[MAX_PATH + 4];
 lstrcpyW(file, game_folder); lstrcatW(file, L"TitbitModern.exe");
 if (GetFileAttributesW(file) == INVALID_FILE_ATTRIBUTES) {
  MessageBoxW(window, L"\x0054\x0069\x0074\x0062\x0069\x0074\x004d\x006f\x0064\x0065\x0072\x006e\x002e\x0065\x0078\x0065\x0020\x043d\x0435\x0020\x043d\x0430\x0439\x0434\x0435\x043d\x002e\x0020\x041f\x043e\x043c\x0435\x0441\x0442\x0438\x0442\x0435\x0020\x043d\x0430\x0441\x0442\x0440\x043e\x0439\x043a\x0438\x0020\x0432\x0020\x043f\x0430\x043f\x043a\x0443\x0020\x0441\x043e\x0432\x0440\x0435\x043c\x0435\x043d\x043d\x043e\x0439\x0020\x0441\x0431\x043e\x0440\x043a\x0438\x0020\x0438\x0433\x0440\x044b\x002e",
               L"\x042f\x0434\x0435\x0440\x043d\x044b\x0439\x0020\x0442\x0438\x0442\x0431\x0438\x0442", MB_OK | MB_ICONERROR);
  return false;
 }
 lstrcpyW(command, L"\""); lstrcatW(command, file); lstrcatW(command, L"\"");
 STARTUPINFOW startup; PROCESS_INFORMATION process;
 ZeroMemory(&startup, sizeof(startup)); ZeroMemory(&process, sizeof(process));
 startup.cb = sizeof(startup);
 if (!CreateProcessW(file, command, 0, 0, FALSE, 0, 0, game_folder, &startup, &process)) {
  MessageBoxW(window, L"\x041d\x0435\x0020\x0443\x0434\x0430\x043b\x043e\x0441\x044c\x0020\x0437\x0430\x043f\x0443\x0441\x0442\x0438\x0442\x044c\x0020\x0438\x0433\x0440\x0443\x002e", L"\x042f\x0434\x0435\x0440\x043d\x044b\x0439\x0020\x0442\x0438\x0442\x0431\x0438\x0442", MB_OK | MB_ICONERROR);
  return false;
 }
 CloseHandle(process.hThread); CloseHandle(process.hProcess);
 return true;
}
static LRESULT CALLBACK SettingsProc(HWND window, UINT msg, WPARAM w, LPARAM l) {
 if (msg == WM_CREATE) {
  Control(window, L"STATIC", L"\x0420\x0435\x0436\x0438\x043c\x0020\x044d\x043a\x0440\x0430\x043d\x0430", 0, 24, 24, 160, 22, 0);
  mode_box = Control(window, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, 200, 20, 330, 150, 101);
  SendMessageW(mode_box, CB_ADDSTRING, 0, (LPARAM)L"\x041d\x0430\x0020\x0432\x0435\x0441\x044c\x0020\x044d\x043a\x0440\x0430\x043d\x0020\x0028\x0440\x0435\x043a\x043e\x043c\x0435\x043d\x0434\x0443\x0435\x0442\x0441\x044f\x0029");
  SendMessageW(mode_box, CB_ADDSTRING, 0, (LPARAM)L"\x0412\x0020\x043e\x043a\x043d\x0435");
  WCHAR value[64];
  GetPrivateProfileStringW(L"compat", L"display", L"fullscreen", value, 64, config_path);
  SendMessageW(mode_box, CB_SETCURSEL, lstrcmpiW(value, L"windowed") == 0 ? 1 : 0, 0);
  size_label = Control(window, L"STATIC", L"\x0420\x0430\x0437\x043c\x0435\x0440\x0020\x043e\x043a\x043d\x0430", 0, 24, 74, 160, 22, 0);
  size_box = Control(window, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_TABSTOP | WS_VSCROLL, 200, 70, 330, 250, 102);
  GetPrivateProfileStringW(L"compat", L"resolution", L"1024x768", value, 64, config_path);
  int selected = 2;
  for (int i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i) {
   SendMessageW(size_box, CB_ADDSTRING, 0, (LPARAM)(i == 0 ? L"\x0410\x0432\x0442\x043e\x043c\x0430\x0442\x0438\x0447\x0435\x0441\x043a\x0438" : sizes[i]));
   if (lstrcmpiW(value, sizes[i]) == 0) selected = i;
  }
  SendMessageW(size_box, CB_SETCURSEL, selected, 0);
  ratio_box = Control(window, L"BUTTON", L"\x0421\x043e\x0445\x0440\x0430\x043d\x044f\x0442\x044c\x0020\x043e\x0440\x0438\x0433\x0438\x043d\x0430\x043b\x044c\x043d\x044b\x0435\x0020\x043f\x0440\x043e\x043f\x043e\x0440\x0446\x0438\x0438\x0020\x0034\x003a\x0033", BS_AUTOCHECKBOX | WS_TABSTOP, 24, 120, 510, 26, 103);
  GetPrivateProfileStringW(L"compat", L"aspect", L"fit", value, 64, config_path);
  SendMessageW(ratio_box, BM_SETCHECK, lstrcmpiW(value, L"stretch") != 0 ? BST_CHECKED : BST_UNCHECKED, 0);
  Control(window, L"STATIC", L"\x041d\x0430\x0020\x0432\x0435\x0441\x044c\x0020\x044d\x043a\x0440\x0430\x043d\x0020\x0438\x0441\x043f\x043e\x043b\x044c\x0437\x0443\x0435\x0442\x0441\x044f\x0020\x0440\x0430\x0437\x0440\x0435\x0448\x0435\x043d\x0438\x0435\x0020\x043c\x043e\x043d\x0438\x0442\x043e\x0440\x0430\x002e\x000a\x041f\x0440\x0438\x0020\x0441\x043e\x0445\x0440\x0430\x043d\x0435\x043d\x0438\x0438\x0020\x0034\x003a\x0033\x0020\x043f\x043e\x0020\x043a\x0440\x0430\x044f\x043c\x0020\x043c\x043e\x0433\x0443\x0442\x0020\x0431\x044b\x0442\x044c\x0020\x0447\x0451\x0440\x043d\x044b\x0435\x0020\x043f\x043e\x043b\x044f\x002e\x000a\x0041\x006c\x0074\x002b\x0045\x006e\x0074\x0065\x0072\x0020\x0438\x043b\x0438\x0020\x0046\x0031\x0031\x0020\x2014\x0020\x043f\x0435\x0440\x0435\x043a\x043b\x044e\x0447\x0435\x043d\x0438\x0435\x0020\x0440\x0435\x0436\x0438\x043c\x0430\x0020\x0432\x0020\x0438\x0433\x0440\x0435\x002e", 0, 24, 165, 510, 65, 0);
  Control(window, L"STATIC", L"\x0417\x0430\x043f\x0443\x0441\x043a\x0020\x0431\x0435\x0437\x0020\x0434\x0438\x0441\x043a\x0430\x0020\x0432\x043a\x043b\x044e\x0447\x0451\x043d\x002e\x0020\x041d\x0430\x0441\x0442\x0440\x043e\x0439\x043a\x0438\x0020\x043f\x0440\x0438\x043c\x0435\x043d\x044f\x044e\x0442\x0441\x044f\x0020\x043f\x0440\x0438\x0020\x0437\x0430\x043f\x0443\x0441\x043a\x0435\x002e", 0, 24, 240, 510, 38, 0);
  Control(window, L"BUTTON", L"\x0421\x043e\x0445\x0440\x0430\x043d\x0438\x0442\x044c", BS_PUSHBUTTON | WS_TABSTOP, 160, 290, 150, 34, 104);
  Control(window, L"BUTTON", L"\x0421\x043e\x0445\x0440\x0430\x043d\x0438\x0442\x044c\x0020\x0438\x0020\x0438\x0433\x0440\x0430\x0442\x044c", BS_DEFPUSHBUTTON | WS_TABSTOP, 325, 290, 205, 34, 105);
  bool enabled = SendMessageW(mode_box, CB_GETCURSEL, 0, 0) == 1;
  EnableWindow(size_box, enabled); EnableWindow(size_label, enabled);
  return 0;
 }
 if (msg == WM_COMMAND) {
  int id = LOWORD(w);
  if (id == 101 && HIWORD(w) == CBN_SELCHANGE) {
   bool enabled = SendMessageW(mode_box, CB_GETCURSEL, 0, 0) == 1;
   EnableWindow(size_box, enabled); EnableWindow(size_label, enabled); return 0;
  }
  if (id == 104 && Save(window)) { DestroyWindow(window); return 0; }
  if (id == 105 && Save(window) && Play(window)) { DestroyWindow(window); return 0; }
 }
 if (msg == WM_CLOSE) { DestroyWindow(window); return 0; }
 if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
 return DefWindowProcW(window, msg, w, l);
}
extern "C" void __cdecl WinMainCRTStartup(void) {
 app = GetModuleHandleW(0);
 typedef BOOL (WINAPI *DpiContext)(HANDLE);
 DpiContext dpi = (DpiContext)GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext");
 if (dpi) dpi((HANDLE)-4);
 HDC screen = GetDC(0); dpi_value = GetDeviceCaps(screen, LOGPIXELSY); ReleaseDC(0,screen);
 app_font = CreateFontW(-MulDiv(16,dpi_value,96),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
                        DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                        DEFAULT_PITCH,L"Segoe UI");
 if (!app_font) app_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
 DWORD n = GetModuleFileNameW(0, game_folder, MAX_PATH);
 if (!n || n >= MAX_PATH) ExitProcess(1);
 while (n && game_folder[n - 1] != L'\\' && game_folder[n - 1] != L'/') --n;
 game_folder[n] = 0;
 lstrcpyW(config_path, game_folder); lstrcatW(config_path, L"titbit_compat.ini");
 WNDCLASSW wc; ZeroMemory(&wc, sizeof(wc));
 wc.lpfnWndProc = SettingsProc; wc.hInstance = app; wc.lpszClassName = L"TitbitModernSettings";
 wc.hCursor = LoadCursorW(0, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
 RegisterClassW(&wc);
 RECT bounds = {0, 0, MulDiv(564,dpi_value,96), MulDiv(350,dpi_value,96)};
 AdjustWindowRect(&bounds, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
 int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
 HWND window = CreateWindowW(wc.lpszClassName, L"\x042f\x0434\x0435\x0440\x043d\x044b\x0439\x0020\x0442\x0438\x0442\x0431\x0438\x0442\x0020\x2014\x0020\x043d\x0430\x0441\x0442\x0440\x043e\x0439\x043a\x0438",
  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
  (GetSystemMetrics(SM_CXSCREEN)-width)/2, (GetSystemMetrics(SM_CYSCREEN)-height)/2,
  width, height, 0, 0, app, 0);
 if (!window) ExitProcess(2);
 ShowWindow(window, SW_SHOWNORMAL);
 MSG msg;
 while (GetMessageW(&msg, 0, 0, 0) > 0) {
  if (!IsDialogMessageW(window, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
 }
 ExitProcess(0);
}

