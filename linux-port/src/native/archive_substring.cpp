// Native-evidenced legacy restoration of pinned WTL string helpers.
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

// Native-evidenced older WTL helper behavior, not a fixed replacement.
#include "engine_string.h"
extern "C" __declspec(dllimport) char*__stdcall CharNextA(const char*);
extern "C" int __cdecl memcmp(const void*,const void*,unsigned);
#pragma intrinsic(memcmp)
inline const char*FindArchiveChar(const char*p,char ch){
 while(*p!=0){if(*p==ch)break;p=CharNextA(p);}
 return(*p==ch)?p:0;
}
const char*__cdecl FindArchiveSubstring(const char*str,const char*set){
 int length=lstrlenA(set);if(length==0)return str;
 const char*result=0;const char*cur=str;
 // The native implementation discards the found pointer, and tests/copies
 // at cur, advancing it one character after each mismatch. Preserve it.
 while(FindArchiveChar(cur,*set)!=0){
  if(memcmp(cur,set,length)==0){result=cur;break;}
  cur=CharNextA(cur);
 }
 return result;
}
