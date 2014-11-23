/***
*tchar.h - definitions for generic international text functions
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Definitions for generic international functions, mostly defines
*       which map string/formatted-io/ctype functions to char, wchar_t, or
*       MBCS versions.  To be used for compatibility between single-byte,
*       multi-byte and Unicode text models.
*
*       [Public]
*
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_TCHAR
#define _INC_TCHAR


#define true 1
#define false 0
typedef int bool;


#define UNICODE
#ifdef UNICODE



typedef wchar_t                               TCHAR;
#define __TEXT(quote)                       L##quote
#else //UNICODE
typedef char                                TCHAR;
#define __TEXT(quote)						quote
#endif //UNICODE


typedef TCHAR*                              LPTSTR;
typedef const TCHAR*                        LPCTSTR;
typedef wchar_t*                              LPWSTR;
typedef const wchar_t*                        LPCWSTR;
typedef TCHAR                               _TCHAR;
#define _T                                 __TEXT

#ifdef UNICODE
	#define _tprintf                            wprintf
	#define _tcscpy                             wcscpy
	#define _tcsncpy                            wcsncpy
	#define _tcscat                             wcscat
	#define _tcscmp                             wcscmp
	#define _tcslen                             wcslen	
	#define _stprintf                           _swprintf
	#define _tsplitpath                         _wsplitpath
	#define _tmakepath                          _wmakepath
	#define _strnicmp							wcsnicmp
	#define _ttoi                               _wtoi
	#define _itot                               _itow
	#define _tcsncmp                            wcsncmp
	#define _tcsstr                             wcsstr
	#define _tcsrchr                            wcsrchr
	#define _tcsncat                            wcsncat
	#define _tcschr                             wcschr
	#define _tcsicmp                            wcsicmp
	#define _tcsnicmp                           wcsnicmp
	#define _tcsnccmp                           wcsncmp
	#define _tcslwr                             _wcslwr
	#define _tcsupr                             _wcsupr
	#define _sntprintf							swprintf
	#define _ttol								_wtol
#else
	#define _tprintf                            printf
	#define _tcscpy                             strcpy
	#define _tcsncpy                            strncpy
	#define _tcscat                             strcat
	#define _tcscmp                             strcmp
	#define _tcslen                             strlen
	#define _stprintf                           sprintf
	#define _tsplitpath                         _splitpath
	#define _tmakepath                          _makepath
	#define _strnicmp							strncmp
	#define _ttoi                               atoi
	#define _itot                               itoa
	#define _tcsncmp                            strncmp
	#define _tcsstr                             strstr
	#define _tcsrchr                            strrchr
	#define _tcsncat                            strncat
	#define _tcschr                             strchr
	#define _tcsnicmp                           strnicmp
	#define _tcsicmp                            stricmp
	#define _tcsnccmp                           strncmp
	#define _tcslwr                             _strlwr
	#define _tcsupr                             _strupr
	#define _sntprintf							snprintf
	#define _ttol								atol
#endif

#endif  /* _INC_TCHAR */

