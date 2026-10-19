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

// Verified WTL Init body; original WTL release and native cleanup remain unresolved.
#include "app_module.h"
namespace WTL {
HRESULT CAppModule::Init(ATL::_ATL_OBJMAP_ENTRY* pObjMap, HINSTANCE hInstance, const GUID* pLibID)
{
		HRESULT hRet = CComModule::Init(pObjMap, hInstance, pLibID);
		if(FAILED(hRet))
			return hRet;

		m_dwMainThreadID = ::GetCurrentThreadId();
		typedef ATL::CSimpleMap<DWORD, CMessageLoop*>   _mapClass;
		m_pMsgLoopMap = NULL;
		ATLTRY(m_pMsgLoopMap = new _mapClass);
		if(m_pMsgLoopMap == NULL)
			return E_OUTOFMEMORY;
		m_pSettingChangeNotify = NULL;

		return hRet;
	}
}
WTL::CAppModule gameModule;
