#pragma once

#include <stdio.h>
#include <stdarg.h>

namespace stdext
{
	inline int jc_vprintf(LPCTSTR format, va_list argptr)
	{
		return _vtprintf_s(format, argptr);
	}

	inline int jc_vsprintf(TCHAR * buf, JCSIZE buf_size, LPCTSTR format, va_list argptr)
	{
		return _vstprintf_s(buf, buf_size, format, argptr);
	};

	inline int jc_fprintf(FILE * stream, LPCTSTR format, ...)
	{
		va_list argptr;
		va_start(argptr, format);
		return _vftprintf_s(stream, format, argptr);
	};

	inline jcerrno jc_fopen(FILE ** pfile, LPCTSTR filename, LPCTSTR mode)
	{
		return _tfopen_s(pfile, filename, mode);
	};

	inline TCHAR * jc_strcat(TCHAR * dst, JCSIZE buf_size, LPCTSTR src)
	{
		_tcscat_s(dst, buf_size, src);
		return dst;
	}

	inline JCSIZE jc_strftime(TCHAR * dst, JCSIZE buf_size, LPCTSTR format, const struct tm * timeptr)
	{
		return (JCSIZE)(_tcsftime(dst, buf_size, format, timeptr));
	}

	inline jcerrno jc_memcpy(void *dst, JCSIZE buf_size, const void * src, JCSIZE count)
	{
		return memcpy_s(dst, buf_size, src, count);
	}

	inline double jc_str2f(LPCTSTR str)		{	return _tstof(str);	}

	inline INT64 jc_str2ll(LPCTSTR str)		{	return _tstoi64(str);	}

	inline int jc_str2l(LPCTSTR str, LPTSTR * str_end, int base)		{	return _tcstol(str, str_end, base);	}
	inline int jc_str2ul(LPCTSTR str, LPTSTR * str_end, int base)		{	return _tcstoul(str, str_end, base);}


	inline jcerrno jc_int2str(INT64 val, LPTSTR str, JCSIZE buf_size, int base)
	{
		return _i64tot_s(val, str, buf_size, base);
	}
	inline jcerrno jc_uint2str(UINT64 val, LPTSTR str, JCSIZE buf_size, int base)
	{
		return _ui64tot_s(val, str, buf_size, base);
	}
};
