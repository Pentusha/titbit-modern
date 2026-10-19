// Windows Template Library - WTL version 7.5
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// This file is a part of the Windows Template Library.
// The use and distribution terms for this software are covered by the
// Common Public License 1.0 (http://opensource.org/licenses/cpl.php)
// which can be found in the file CPL.TXT at the root of this distribution.
// By using this software in any fashion, you are agreeing to be bound by
// the terms of this license. You must not remove this notice, or
// any other, from this software.

// Verified reconstruction excerpt: the nonvirtual Init and three data members
// are selected from pinned WTL7.5 build5333; original target version is unknown.
// ATL CComModule/base implementation comes unchanged from installed VC6.5 SDK.
#ifndef TITBIT_APP_MODULE_H
#define TITBIT_APP_MODULE_H
#ifndef _ATL_MIN_CRT
#define _ATL_MIN_CRT
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <atlbase.h>
namespace WTL {
class CMessageLoop;
class CAppModule : public ATL::CComModule
{
public:
	DWORD m_dwMainThreadID;
	ATL::CSimpleMap<DWORD, CMessageLoop*>* m_pMsgLoopMap;
	ATL::CSimpleArray<HWND>* m_pSettingChangeNotify;

// Overrides of CComModule::Init and Term
	HRESULT Init(ATL::_ATL_OBJMAP_ENTRY* pObjMap, HINSTANCE hInstance, const GUID* pLibID = NULL);
	// Native game cleanup remains unbuilt; do not use the base-only Term.
	void Term();

};
}
#endif

extern WTL::CAppModule gameModule;
