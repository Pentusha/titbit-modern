/* Authored names; native evidence is the Win32 import, literal and stores.
 * The initializer thunk/helper are emitted by the historical C++ compiler. */
#include <windows.h>
unsigned int registered_set_rgb_message = RegisterWindowMessageA("commdlg_SetRGBColor");
