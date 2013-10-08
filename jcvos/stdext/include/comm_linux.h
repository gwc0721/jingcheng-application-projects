#pragma once
/*
 * comm_linux.h
 *
 *  Created on: Jun 7, 2012
 *      Author: jingcheng
 */

#ifndef COMM_LINUX_H_
#define COMM_LINUX_H_

// 关于整数的定义

#include <inttypes.h>

typedef unsigned char 	BYTE;
typedef unsigned long		DWORD;

typedef unsigned int UINT;


typedef int64_t INT64;
typedef uint64_t UINT64;



// 关于字符串和UNICODE的定义


#ifdef _UNICODE

#include <wchar.h>
typedef wchar_t TCHAR;
#define _T(x)	L ## x
#define _vstprintf vswprintf
#define _vftprintf vfwprintf
#define _tcschr		wcschr
#define _tcslen		wcslen
#define _tcscmp		wcscmp

#else

typedef char TCHAR;
#define _T(x)	x
#define _vstprintf vsprintf
#define _vftprintf vfprintf
#define _tcschr		strchr
#define _tcslen		strlen
#define _tcscmp		strcmp

#endif

#define		LockedIncrement(ii)		(++ii)
#define 	LockedDecrement(ii)		(--ii)

#include <assert.h>

#define		jcbreak		assert(false)

#endif /* COMM_LINUX_H_ */
