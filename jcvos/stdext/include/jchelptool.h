#pragma once

#include "jcexception.h"
namespace stdext
{
	class CJCHelpTool
	{
		// 将绝对路径转换成相对路径，
		//	[IN] cur_path : 作为参考的当前路径。绝对路径表示。Win32下，Volumn必须和abs_path一致。
		//	[IN] abs_path : 绝对路径。Win32下必须以"x:"开头。Linux下必须以"/"开头。
		//	[OUT] rel_path : 转换后的相对路径
		static int PathAbs2Rel(LPCTSTR cur_path, LPCTSTR abs_path, CJCStringT & rel_path);

		static int PathRel2Abs(LPCTSTR cur_path, LPCTSTR rel_path, CJCStringT & abs_path);
	};


	int UnicodeToUtf8(char *strDest, JCSIZE nDestLen, const wchar_t *szSrc, JCSIZE nSrcLen);
	void UnicodeToUtf8(CJCStringA & dest, const wchar_t *szSrc, JCSIZE nSrcLen = 0);
	JCSIZE Utf8ToUnicode(wchar_t *strDest, JCSIZE nDestLen, const char *szSrc, JCSIZE nSrcLen);

	// Convert an integer to string in hex
	//	[OUT]	str :	Output buffer. Caller shall insure buffer size >= len +1
	//	[IN]	len :	Digitals to convert
	//	[IN]	d :		Data to be converted
	void itohex(LPTSTR str, JCSIZE dig, UINT d);
	UINT64 str2hex(LPCTSTR str, JCSIZE dig = 0xFFFFFFFF);

	inline UINT char2hex(TCHAR ch)
	{
		if ( ( _T('0') <= ch ) && ( ch <= _T('9')) ) return ch - _T('0');
		else if ( ( _T('A') <= ch ) && ( ch <= _T('F')) ) return ch - _T('A') + 0xA;
		else if ( ( _T('a') <= ch ) && ( ch <= _T('f')) ) return ch - _T('a') + 0xA;
		else THROW_ERROR(ERR_PARAMETER, _T("Illegal hex charactor %c"), ch);
	}

	inline TCHAR hex2char(BYTE d)
	{
		if ( d < 10 ) return _T('0') + d;
		else return _T('A') + (d-10);
	}

	inline UINT64 MAKEUINT64(DWORD lo, DWORD hi)
	{
		return ( (UINT64)(hi)<<32 | (UINT64)(lo) );
	}


};
