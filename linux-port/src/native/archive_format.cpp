// Native-evidenced legacy restoration of the pinned WTL FormatV excerpt.
// Source/class labels are authored; the target WTL version is not identified.
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

#include "engine_string.h"
#include <stdarg.h>
#include <stdlib.h>
#include <wchar.h>
typedef char TCHAR;
typedef const char*LPCTSTR;
typedef const char*LPCSTR;
typedef wchar_t WCHAR;
typedef wchar_t*LPWSTR;
#define _T(x) x
#define ATLASSERT(x) ((void)0)
#define lstrlen lstrlenA
#define CharNext CharNextA
#define OutputDebugString OutputDebugStringA
#define _cstrtoi atoi
#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))
extern "C"{
 __declspec(dllimport) char* __stdcall CharNextA(const char*);
 __declspec(dllimport) unsigned long __stdcall GetThreadLocale();
 __declspec(dllimport) int __stdcall GetStringTypeExA(unsigned long,unsigned long,const char*,int,unsigned short*);
 __declspec(dllimport) void __stdcall OutputDebugStringA(const char*);
 __declspec(dllimport) void __stdcall DebugBreak();
 __declspec(dllimport) int __stdcall wvsprintfA(char*,const char*,char*);
}
#define wvsprintf wvsprintfA
inline int _cstrisdigit(char ch){
 unsigned short type;GetStringTypeExA(GetThreadLocale(),1,&ch,1,&type);return(type&4)==4;
}

#define ARCHIVE_BUFFER_INLINE inline
#include "engine_string_buffer.inl"
#undef ARCHIVE_BUFFER_INLINE

void ObservedString::FormatV(LPCTSTR lpszFormat, char* argList)
{
	ATLASSERT(_IsValidString(lpszFormat));

	enum _FormatModifiers
	{
		FORCE_ANSI =	0x10000,
		FORCE_UNICODE =	0x20000,
		FORCE_INT64 =	0x40000
	};

	va_list argListSave = argList;

	// make a guess at the maximum length of the resulting string
	int nMaxLen = 0;
	for (LPCTSTR lpsz = lpszFormat; *lpsz != _T('\0'); lpsz = ::CharNext(lpsz))
	{
		// handle '%' character, but watch out for '%%'
		if (*lpsz != _T('%') || *(lpsz = ::CharNext(lpsz)) == _T('%'))
		{
			nMaxLen += lstrlenA(lpsz);
			continue;
		}

		int nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *lpsz != _T('\0'); lpsz = ::CharNext(lpsz))
		{
			// check for valid flags
			if (*lpsz == _T('#'))
				nMaxLen += 2;   // for '0x'
			else if (*lpsz == _T('*'))
				nWidth = va_arg(argList, int);
			else if (*lpsz == _T('-') || *lpsz == _T('+') || *lpsz == _T('0') || *lpsz == _T(' '))
				;
			else // hit non-flag character
				break;
		}
		// get width and skip it
		if (nWidth == 0)
		{
			// width indicated by
			nWidth = _cstrtoi(lpsz);
			for (; *lpsz != _T('\0') && _cstrisdigit(*lpsz); lpsz = ::CharNext(lpsz))
				;
		}
		ATLASSERT(nWidth >= 0);

		int nPrecision = 0;
		if (*lpsz == _T('.'))
		{
			// skip past '.' separator (width.precision)
			lpsz = ::CharNext(lpsz);

			// get precision and skip it
			if (*lpsz == _T('*'))
			{
				nPrecision = va_arg(argList, int);
				lpsz = ::CharNext(lpsz);
			}
			else
			{
				nPrecision = _cstrtoi(lpsz);
				for (; *lpsz != _T('\0') && _cstrisdigit(*lpsz); lpsz = ::CharNext(lpsz))
					;
			}
			ATLASSERT(nPrecision >= 0);
		}

		// should be on type modifier or specifier
		int nModifier = 0;
		if(lpsz[0] == _T('I') && lpsz[1] == _T('6') && lpsz[2] == _T('4'))
		{
			lpsz += 3;
			nModifier = FORCE_INT64;
		}
		else
		{
			switch (*lpsz)
			{
			// modifiers that affect size
			case _T('h'):
				nModifier = FORCE_ANSI;
				lpsz = ::CharNext(lpsz);
				break;
			case _T('l'):
				nModifier = FORCE_UNICODE;
				lpsz = ::CharNext(lpsz);
				break;

			// modifiers that do not affect size
			case _T('F'):
			case _T('N'):
			case _T('L'):
				lpsz = ::CharNext(lpsz);
				break;
			}
		}

		// now should be on specifier
		switch (*lpsz | nModifier)
		{
		// single characters
		case _T('c'):
		case _T('C'):
			nItemLen = 2;
			va_arg(argList, TCHAR);
			break;
		case _T('c') | FORCE_ANSI:
		case _T('C') | FORCE_ANSI:
			nItemLen = 2;
			va_arg(argList, char);
			break;
		case _T('c') | FORCE_UNICODE:
		case _T('C') | FORCE_UNICODE:
			nItemLen = 2;
			va_arg(argList, WCHAR);
			break;

		// strings
		case _T('s'):
		{
			LPCTSTR pstrNextArg = va_arg(argList, LPCTSTR);
			if (pstrNextArg == NULL)
			{
				nItemLen = 6;  // "(null)"
			}
			else
			{
				nItemLen = lstrlen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
			break;
		}

		case _T('S'):
		{
#ifndef _UNICODE
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
			{
				nItemLen = 6;  // "(null)"
			}
			else
			{
				nItemLen = (int)wcslen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
#else // _UNICODE
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL)
			{
				nItemLen = 6; // "(null)"
			}
			else
			{
#if defined(_WIN32_WCE) && (_ATL_VER >= 0x0800)
				nItemLen = ATL::lstrlenA(pstrNextArg);
#else
				nItemLen = lstrlenA(pstrNextArg);
#endif
				nItemLen = max(1, nItemLen);
			}
#endif // _UNICODE
			break;
		}

		case _T('s') | FORCE_ANSI:
		case _T('S') | FORCE_ANSI:
		{
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL)
			{
				nItemLen = 6; // "(null)"
			}
			else
			{
#if defined(_WIN32_WCE) && (_ATL_VER >= 0x0800)
				nItemLen = ATL::lstrlenA(pstrNextArg);
#else
				nItemLen = lstrlenA(pstrNextArg);
#endif
				nItemLen = max(1, nItemLen);
			}
			break;
		}

		case _T('s') | FORCE_UNICODE:
		case _T('S') | FORCE_UNICODE:
		{
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
			{
				nItemLen = 6; // "(null)"
			}
			else
			{
				nItemLen = (int)wcslen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
			break;
		}
		}

		// adjust nItemLen for strings
		if (nItemLen != 0)
		{
			nItemLen = max(nItemLen, nWidth);
			if (nPrecision != 0)
				nItemLen = min(nItemLen, nPrecision);
		}
		else
		{
			switch (*lpsz)
			{
			// integers
			case _T('d'):
			case _T('i'):
			case _T('u'):
			case _T('x'):
			case _T('X'):
			case _T('o'):
				if (nModifier & FORCE_INT64)
					va_arg(argList, __int64);
				else
					va_arg(argList, int);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth + nPrecision);
				break;

#ifndef _ATL_USE_CSTRING_FLOAT
			case _T('e'):
			case _T('f'):
			case _T('g'):
			case _T('G'):
				ATLASSERT(!"Floating point (%%e, %%E, %%f, %%g, and %%G) is not supported by the WTL::CString class.");
#ifndef _DEBUG
				::OutputDebugString(_T("Floating point (%%e, %%f, %%g, and %%G) is not supported by the WTL::CString class."));
#ifndef _WIN32_WCE
				::DebugBreak();
#else // CE specific
				DebugBreak();
#endif // _WIN32_WCE
#endif // !_DEBUG
				break;
#else // _ATL_USE_CSTRING_FLOAT
			case _T('e'):
			case _T('g'):
			case _T('G'):
				va_arg(argList, double);
				nItemLen = 128;
				nItemLen = max(nItemLen, nWidth + nPrecision);
				break;
			case _T('f'):
				{
					double f;
					LPTSTR pszTemp;

					// 312 == strlen("-1+(309 zeroes).")
					// 309 zeroes == max precision of a double
					// 6 == adjustment in case precision is not specified,
					//   which means that the precision defaults to 6
					pszTemp = (LPTSTR)_alloca(max(nWidth, 312 + nPrecision + 6) * sizeof(TCHAR));

					f = va_arg(argList, double);
					_stprintf(pszTemp, _T("%*.*f"), nWidth, nPrecision + 6, f);
					nItemLen = _tcslen(pszTemp);
				}
				break;
#endif // _ATL_USE_CSTRING_FLOAT

			case _T('p'):
				va_arg(argList, void*);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth + nPrecision);
				break;

			// no output
			case _T('n'):
				va_arg(argList, int*);
				break;

			default:
				ATLASSERT(FALSE);  // unknown formatting option
			}
		}

		// adjust nMaxLen for output nItemLen
		nMaxLen += nItemLen;
	}

	GetBuffer(nMaxLen);
#ifndef _ATL_USE_CSTRING_FLOAT
	int nRet = ::wvsprintf(data, lpszFormat, argListSave);
#else // _ATL_USE_CSTRING_FLOAT
	int nRet = _vstprintf(data, lpszFormat, argListSave);
#endif // _ATL_USE_CSTRING_FLOAT
	nRet;   // ref
	ATLASSERT(nRet <= GetAllocLength());
	ReleaseBuffer(-1);

	va_end(argListSave);
}

