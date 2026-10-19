#ifndef TITBIT_ENGINE_MODULE_RESOURCE_H
#define TITBIT_ENGINE_MODULE_RESOURCE_H
/* Read-only view of the actual ATL CComModule::m_hInstResource member.
 * Historical SDK sizeof/offsetof metadata proves offset8 and pointer size4.
 * The complete144-byte gameModule is source-owned by app_module.cpp; this
 * view emits neither a second variable nor an absolute linker alias.
 * A forward declaration avoids changing the existing Win32 string API views.
 */
namespace WTL { class CAppModule; }
extern WTL::CAppModule gameModule;
#define observed_resource_instance (*(void *const *)((const char *)&gameModule + 8))
#endif
