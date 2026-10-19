/* Win32 OpenGL calls pop their arguments on x86; ELF libGL functions do not.
 * Only these prefixed entry points cross that boundary. Never bind an engine
 * stdcall declaration directly to a Linux GL symbol. */
#include <GL/gl.h>
#include <GL/glu.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__i386__)
#define WAPI __attribute__((stdcall))
#else
#define WAPI
#endif
static void *gl_library;
static pthread_once_t library_once=PTHREAD_ONCE_INIT;
static void load_libraries(void) {
 gl_library=dlopen("libGL.so.1",RTLD_NOW|RTLD_LOCAL);
 if(!gl_library){fprintf(stderr,"Titbit: OpenGL runtime load failed: %s\n",dlerror());abort();}
}
static void *resolve(const char *name) {
 void *result;
 /* GLU is linked into the executable (statically with --minimal), so it is
  * taken from there: the system may have no 32-bit libGLU.so.1 at all. */
 if(!strcmp(name,"gluPerspective"))return (void *)&gluPerspective;
 if(!strcmp(name,"gluBuild2DMipmaps"))return (void *)&gluBuild2DMipmaps;
 pthread_once(&library_once,load_libraries);
 result=dlsym(gl_library,name);
 if(!result){fprintf(stderr,"Titbit: required OpenGL symbol missing: %s\n",name);abort();}
 return result;
}
#define GET_PROC(name,params) typedef void (*Fn) params; static void *cached; void *p=__atomic_load_n(&cached,__ATOMIC_ACQUIRE);if(!p){p=resolve(#name);__atomic_store_n(&cached,p,__ATOMIC_RELEASE);} Fn fn=(Fn)p
#define VOID_WRAP(name,params,args) void WAPI tb_gl_stdcall_##name params {GET_PROC(name,params);fn args;}
#define RET_WRAP(type,name,params,args) type WAPI tb_gl_stdcall_##name params {typedef type (*Fn) params;static void *cached;void *p=__atomic_load_n(&cached,__ATOMIC_ACQUIRE);if(!p){p=resolve(#name);__atomic_store_n(&cached,p,__ATOMIC_RELEASE);}return ((Fn)p) args;}
VOID_WRAP(glTexGeni,(GLenum a,GLenum b,GLint c),(a,b,c))
VOID_WRAP(glMaterialf,(GLenum a,GLenum b,GLfloat c),(a,b,c))
VOID_WRAP(glCullFace,(GLenum a),(a))
VOID_WRAP(glMaterialfv,(GLenum a,GLenum b,const GLfloat *c),(a,b,c))
VOID_WRAP(glPointSize,(GLfloat a),(a))
VOID_WRAP(glClearDepth,(GLdouble a),(a))
VOID_WRAP(glDeleteTextures,(GLsizei a,const GLuint *b),(a,b))
VOID_WRAP(glTexImage2D,(GLenum a,GLint b,GLint c,GLsizei d,GLsizei e,GLint f,GLenum g,GLenum h,const void *i),(a,b,c,d,e,f,g,h,i))
VOID_WRAP(glTexEnvi,(GLenum a,GLenum b,GLint c),(a,b,c))
VOID_WRAP(glTexParameteri,(GLenum a,GLenum b,GLint c),(a,b,c))
VOID_WRAP(glShadeModel,(GLenum a),(a))
VOID_WRAP(glOrtho,(GLdouble a,GLdouble b,GLdouble c,GLdouble d,GLdouble e,GLdouble f),(a,b,c,d,e,f))
VOID_WRAP(glGetFloatv,(GLenum a,GLfloat *b),(a,b))
VOID_WRAP(glLightModelfv,(GLenum a,const GLfloat *b),(a,b))
VOID_WRAP(glLightfv,(GLenum a,GLenum b,const GLfloat *c),(a,b,c))
VOID_WRAP(glGetIntegerv,(GLenum a,GLint *b),(a,b))
VOID_WRAP(glNormal3f,(GLfloat a,GLfloat b,GLfloat c),(a,b,c))
RET_WRAP(GLuint,glGenLists,(GLsizei a),(a))
VOID_WRAP(glNewList,(GLuint a,GLenum b),(a,b))
VOID_WRAP(glEndList,(void),())
VOID_WRAP(glCallList,(GLuint a),(a))
VOID_WRAP(glNormal3fv,(const GLfloat *a),(a))
VOID_WRAP(glDepthMask,(GLboolean a),(a))
VOID_WRAP(glDepthFunc,(GLenum a),(a))
VOID_WRAP(glVertex3f,(GLfloat a,GLfloat b,GLfloat c),(a,b,c))
VOID_WRAP(glLoadMatrixf,(const GLfloat *a),(a))
VOID_WRAP(glMultMatrixf,(const GLfloat *a),(a))
VOID_WRAP(glBegin,(GLenum a),(a))
VOID_WRAP(glVertex3fv,(const GLfloat *a),(a))
VOID_WRAP(glEnd,(void),())
VOID_WRAP(glViewport,(GLint a,GLint b,GLsizei c,GLsizei d),(a,b,c,d))
VOID_WRAP(glDisable,(GLenum a),(a))
VOID_WRAP(glClear,(GLbitfield a),(a))
VOID_WRAP(glEnable,(GLenum a),(a))
VOID_WRAP(glTexCoord2f,(GLfloat a,GLfloat b),(a,b))
VOID_WRAP(glMatrixMode,(GLenum a),(a))
VOID_WRAP(glLoadIdentity,(void),())
VOID_WRAP(glPushMatrix,(void),())
VOID_WRAP(glPopMatrix,(void),())
VOID_WRAP(glTexParameterf,(GLenum a,GLenum b,GLfloat c),(a,b,c))
VOID_WRAP(glTexSubImage2D,(GLenum a,GLint b,GLint c,GLint d,GLsizei e,GLsizei f,GLenum g,GLenum h,const void *i),(a,b,c,d,e,f,g,h,i))
VOID_WRAP(glFlush,(void),())
VOID_WRAP(glColor4f,(GLfloat a,GLfloat b,GLfloat c,GLfloat d),(a,b,c,d))
VOID_WRAP(glBlendFunc,(GLenum a,GLenum b),(a,b))
VOID_WRAP(glBindTexture,(GLenum a,GLuint b),(a,b))
VOID_WRAP(glPixelStorei,(GLenum a,GLint b),(a,b))
RET_WRAP(const GLubyte *,glGetString,(GLenum a),(a))
VOID_WRAP(gluPerspective,(GLdouble a,GLdouble b,GLdouble c,GLdouble d),(a,b,c,d))
RET_WRAP(GLint,gluBuild2DMipmaps,(GLenum a,GLint b,GLsizei c,GLsizei d,GLenum e,GLenum f,const void *g),(a,b,c,d,e,f,g))
VOID_WRAP(glMultiTexCoord1fARB,(GLenum a,GLfloat b),(a,b))
VOID_WRAP(glMultiTexCoord2fARB,(GLenum a,GLfloat b,GLfloat c),(a,b,c))
VOID_WRAP(glMultiTexCoord3fARB,(GLenum a,GLfloat b,GLfloat c,GLfloat d),(a,b,c,d))
VOID_WRAP(glMultiTexCoord4fARB,(GLenum a,GLfloat b,GLfloat c,GLfloat d,GLfloat e),(a,b,c,d,e))
VOID_WRAP(glActiveTextureARB,(GLenum a),(a))
VOID_WRAP(glClientActiveTextureARB,(GLenum a),(a))

struct Entry { const char *name;void *address; };
#define ENTRY(name) {#name,(void *)&tb_gl_stdcall_##name}
static const struct Entry entries[]={
 ENTRY(glTexGeni),ENTRY(glMaterialf),ENTRY(glCullFace),ENTRY(glMaterialfv),ENTRY(glPointSize),ENTRY(glClearDepth),
 ENTRY(glDeleteTextures),ENTRY(glTexImage2D),ENTRY(glTexEnvi),ENTRY(glTexParameteri),ENTRY(glShadeModel),ENTRY(glOrtho),
 ENTRY(glGetFloatv),ENTRY(glLightModelfv),ENTRY(glLightfv),ENTRY(glGetIntegerv),ENTRY(glNormal3f),ENTRY(glGenLists),
 ENTRY(glNewList),ENTRY(glEndList),ENTRY(glCallList),ENTRY(glNormal3fv),ENTRY(glDepthMask),ENTRY(glDepthFunc),
 ENTRY(glVertex3f),ENTRY(glLoadMatrixf),ENTRY(glMultMatrixf),ENTRY(glBegin),ENTRY(glVertex3fv),ENTRY(glEnd),ENTRY(glViewport),
 ENTRY(glDisable),ENTRY(glClear),ENTRY(glEnable),ENTRY(glTexCoord2f),ENTRY(glMatrixMode),ENTRY(glLoadIdentity),
 ENTRY(glPushMatrix),ENTRY(glPopMatrix),ENTRY(glTexParameterf),ENTRY(glTexSubImage2D),ENTRY(glFlush),ENTRY(glColor4f),
 ENTRY(glBlendFunc),ENTRY(glBindTexture),ENTRY(glPixelStorei),ENTRY(glGetString),ENTRY(gluPerspective),ENTRY(gluBuild2DMipmaps),
 ENTRY(glMultiTexCoord1fARB),ENTRY(glMultiTexCoord2fARB),ENTRY(glMultiTexCoord3fARB),ENTRY(glMultiTexCoord4fARB),
 ENTRY(glActiveTextureARB),ENTRY(glClientActiveTextureARB)
};
void *tb_gl_stdcall_proc(const char *name) {
 for(unsigned i=0;i<sizeof(entries)/sizeof(entries[0]);i++)if(!strcmp(name,entries[i].name))return entries[i].address;
 /* An unknown function has no known stdcall signature. Returning a native
  * cdecl pointer here would corrupt the engine stack. */
 return NULL;
}
