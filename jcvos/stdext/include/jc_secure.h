#pragma once

#ifdef WIN32

typedef errno_t jcerrno;

#else

typedef int jcerrno;

#endif

#include <stdio.h>
#include <stdarg.h>


namespace stdext
{
	inline int jc_vprintf(LPCTSTR format, va_list argptr);

	inline int jc_printf(LPCTSTR format, ...)
	{
		va_list argptr;
		va_start(argptr, format);
		return jc_vprintf(format, argptr);
	}


	inline int jc_vsprintf(TCHAR * buf, JCSIZE buf_size, LPCTSTR format, va_list argptr);

	inline int jc_sprintf(TCHAR * buffer, JCSIZE buf_size, LPCTSTR format, ... )
	{
		va_list argptr;
		va_start(argptr, format);
		return jc_vsprintf(buffer, buf_size, format, argptr);
	};

	template <JCSIZE buf_size>
	inline int jc_sprintf(TCHAR (&buffer)[buf_size], LPCTSTR format, ...)
	{
		va_list argptr;
		va_start(argptr, format);
		return jc_vsprintf(buffer, buf_size, format, argptr);
	};

	inline int jc_fprintf(FILE * stream, LPCTSTR format, ...);
	inline jcerrno jc_fopen(FILE ** pfile, LPCTSTR filename, LPCTSTR mode);

	inline TCHAR * jc_strcat(TCHAR * dst, JCSIZE buf_size, LPCTSTR src);

	inline JCSIZE jc_strftime(TCHAR * dst, JCSIZE buf_size, LPCTSTR format, const struct tm * timeptr);

	inline jcerrno jc_memcpy(void *dst, JCSIZE buf_size, const void * src, JCSIZE count);

	inline double jc_str2f(LPCTSTR str);
	inline INT64 jc_str2ll(LPCTSTR str);

	inline int jc_str2l(LPCTSTR str, LPTSTR * str_end, int base = 10);
	inline int jc_str2l(LPCTSTR str, int base = 10)
	{
		LPTSTR str_end = NULL;
		return jc_str2l(str, &str_end, base);
	}

	inline int jc_str2ul(LPCTSTR str, LPTSTR * str_end, int base = 10);
	inline int jc_str2ul(LPCTSTR str, int base = 10)
	{
		LPTSTR str_end = NULL;
		return jc_str2ul(str, &str_end, base);
	}


	inline jcerrno jc_int2str(INT64 val, LPTSTR str, JCSIZE buf_size, int base = 10);

	template <JCSIZE buf_size>
	inline jcerrno jc_int2str(INT64 val, TCHAR (&str)[buf_size], int base = 10)
	{
		return jc_int2str(val, str, buf_size, base);
	}

	inline jcerrno jc_uint2str(UINT64 val, LPTSTR str, JCSIZE buf_size, int base = 10);

	template <JCSIZE buf_size>
	inline jcerrno jc_uint2str(UINT64 val, TCHAR (&str)[buf_size],int base = 10)
	{
		return jc_uint2str(val, str, buf_size, base);
	}
//	inline jcerrno

};

#ifdef __linux__
#include "jc_secure_linux.h"
#endif

#ifdef WIN32
#include "jc_secure_windows.h"
#endif
