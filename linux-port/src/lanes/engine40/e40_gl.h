// OpenGL 1.1 imports used by the 0x40xxxx units.
#ifndef E40_GL_H
#define E40_GL_H
extern "C" {
__declspec(dllimport) void __stdcall glEnable(unsigned);
__declspec(dllimport) void __stdcall glDisable(unsigned);
__declspec(dllimport) void __stdcall glBlendFunc(unsigned, unsigned);
__declspec(dllimport) void __stdcall glColor4f(float, float, float, float);
__declspec(dllimport) void __stdcall glPushMatrix(void);
__declspec(dllimport) void __stdcall glPopMatrix(void);
__declspec(dllimport) void __stdcall glLoadMatrixf(const float *);
__declspec(dllimport) void __stdcall glBindTexture(unsigned, unsigned);
__declspec(dllimport) void __stdcall glBegin(unsigned);
__declspec(dllimport) void __stdcall glEnd(void);
__declspec(dllimport) void __stdcall glTexCoord2f(float, float);
__declspec(dllimport) void __stdcall glVertex3f(float, float, float);
__declspec(dllimport) void __stdcall glVertex3fv(const float *);
__declspec(dllimport) void __stdcall glMultMatrixf(const float *);
__declspec(dllimport) void __stdcall glTexSubImage2D(unsigned, int, int, int, int, int, unsigned, unsigned, const void *);
}
#endif
