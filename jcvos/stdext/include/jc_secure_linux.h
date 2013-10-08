/*
 * jc_secure_std_linux.h
 *
 *  Created on: Jun 13, 2012
 *      Author: jingcheng
 */
#pragma once



//#include <iconv.h>

#include "jchelptool.h"
#include <string.h>

namespace stdext
{
	inline int jc_vprintf(LPCTSTR format, va_list argptr)
	{
		return wprintf(format, argptr);
	}

	inline int jc_vsprintf(TCHAR * buf, JCSIZE buf_size, LPCTSTR format, va_list argptr)
	{
		return _vstprintf(buf, buf_size, format, argptr);
	}

	inline int jc_fprintf(FILE * stream, LPCTSTR format, ...)
	{
		va_list argptr;
		va_start(argptr, format);
		return _vftprintf(stream, format, argptr);
	}

	inline jcerrno jc_fopen(FILE ** pfile, LPCTSTR filename, LPCTSTR mode)
	{
		JCSIZE fn_src_len = wcslen(filename);
		JCSIZE fn_dst_len = fn_src_len * 3;
		char * fn_utf8 = new char[fn_dst_len];
		UnicodeToUtf8(fn_utf8, fn_dst_len, filename, fn_src_len);

		char mode_utf8[16];
		UnicodeToUtf8(mode_utf8, 16, mode, wcslen(mode));
		*pfile = fopen(fn_utf8, mode_utf8);
		return 0;
	}

	inline TCHAR * jc_strcat(TCHAR * dst, JCSIZE buf_size, LPCTSTR src)
	{
		return wcsncat(dst, src, buf_size);
	}

	inline JCSIZE jc_strftime(TCHAR * dst, JCSIZE buf_size, LPCTSTR format, const struct tm * timeptr)
	{
		return wcsftime(dst, buf_size, format, timeptr);
	}

	inline jcerrno jc_memcpy(void *dst, JCSIZE buf_size, const void * src, JCSIZE count)
	{
		memcpy(dst, src, count);
		return 0;
	}

	inline double jc_str2f(LPCTSTR str)
	{
		TCHAR * str_end = NULL;
		return wcstof(str, &str_end);
	}

	inline INT64 jc_str2ll(LPCTSTR str)
	{
		TCHAR * str_end = NULL;
		return wcstoll(str, &str_end, 10);
	}

	inline int jc_str2l(LPCTSTR str, LPTSTR * str_end, int base)		{	return wcstol(str, str_end, base);	}
	inline int jc_str2ul(LPCTSTR str, LPTSTR * str_end, int base)		{	return wcstoul(str, str_end, base);}

	inline jcerrno jc_int2str(INT64 val, LPTSTR str, JCSIZE buf_size, int base)
	{
		swprintf(str, buf_size, _T("%lld"), val);
		return 0;
	}

	inline jcerrno jc_uint2str(UINT64 val, LPTSTR str, JCSIZE buf_size, int base)
	{
		swprintf(str, buf_size, _T("%llu"), val);
		return 0;
	}

}

//#ifndef JC_SECURE_STD_LINUX_H_
//#define JC_SECURE_STD_LINUX_H_


//#endif /* JC_SECURE_STD_LINUX_H_ */
