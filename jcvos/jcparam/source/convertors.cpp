//#include "StdAfx.h"
#include "../include/ivalue.h"

///////////////////////////////////////////////////////////////////////////////////////////////
// CVariantConvert
//using namespace jcparam;
// 数值 ------------------------------------------------------------------------------------


#include "../include/value.h"



//-- bool --
namespace jcparam
{
	template <>	VALUE_TYPE jcparam::type_id<bool>::id(void)		{return VT_BOOL;}
	template <> VALUE_TYPE jcparam::type_id<char>::id(void)		{return VT_CHAR;}
	template <>	VALUE_TYPE jcparam::type_id<BYTE>::id(void)		{return VT_UCHAR;}
	template <>	VALUE_TYPE jcparam::type_id<wchar_t>::id(void)	{return VT_SHORT;}
	template <>	VALUE_TYPE jcparam::type_id<short>::id(void)	{return VT_SHORT;}
	template <>	VALUE_TYPE jcparam::type_id<WORD>::id(void)		{return VT_USHORT;}
	template <>	VALUE_TYPE jcparam::type_id<int>::id(void)		{return VT_INT;}
	template <>	VALUE_TYPE jcparam::type_id<DWORD>::id(void)	{return VT_UINT;}
	template <>	VALUE_TYPE jcparam::type_id<unsigned int>::id(void)	{return VT_UINT;}	
	template <>	VALUE_TYPE jcparam::type_id<INT64>::id(void)	{return VT_INT64;}
	template <>	VALUE_TYPE jcparam::type_id<UINT64>::id(void)	{return VT_UINT64;}
	template <>	VALUE_TYPE jcparam::type_id<float>::id(void)	{return VT_FLOAT;}
	template <>	VALUE_TYPE jcparam::type_id<double>::id(void)	{return VT_DOUBLE;}
	template <> VALUE_TYPE jcparam::type_id<CJCStringT>::id(void)	{return VT_STRING;}
	template <> VALUE_TYPE jcparam::type_id<wchar_t const *>::id(void)	{return VT_STRING;}

template <> void jcparam::CConvertor<bool>::T2S(const bool & typ, CJCStringT & str)
{
	str =  typ?_T("1"):_T("0");
}

template <> void jcparam::CConvertor<bool>::S2T(LPCTSTR str, bool & typ)
{
	if ( _tcscmp(str, _T("0"))==0 || _tcscmp(str, _T("false")) == 0  )	typ = false;
	else	typ = true;
}

//-- char --
template <> void CConvertor<char>::T2S(const char & typ, CJCStringT & str)
{
	TCHAR _str[4];
	stdext::jc_int2str( typ, _str, 10);
	str = _str;
}

template <> void CConvertor<char>::S2T(LPCTSTR str, char &  typ)
{
	LPTSTR end = NULL;
	 typ = (char)stdext::jc_str2l(str, &end);
}

//-- u char --
template <> void CConvertor<unsigned char>::T2S(const unsigned char & typ, CJCStringT & str)
{
	TCHAR _str[4];
	stdext::jc_int2str( typ, _str, 10);
	str = _str;
}

template <> void CConvertor<unsigned char>::S2T(LPCTSTR str, unsigned char & typ)
{
	LPTSTR end = NULL;
	if ( _T('0') == str[0] && (_T('x') == str[1] || _T('X') == str[1]) )
		typ = (unsigned char)stdext::str2hex(str+2);
	else  typ = (unsigned char)stdext::jc_str2l(str, &end);
}

//-- wchar --
template <> void CConvertor<wchar_t>::T2S(const wchar_t & typ, CJCStringT & str)
{
	str = typ;
}

template <> void CConvertor<wchar_t>::S2T(LPCTSTR str, wchar_t & typ)
{
	typ = str[0];
}

//-- short --
template <> void CConvertor<short>::T2S(const short & typ, CJCStringT & str)
{
	TCHAR _str[8];
	stdext::jc_int2str( typ, _str, 10);
	str = _str;
}

template <> void CConvertor<short>::S2T(LPCTSTR str, short & typ)
{
	 typ = (short)stdext::jc_str2l(str);
}

//-- u short -- 
template <> void CConvertor<unsigned short>::T2S(const unsigned short & typ, CJCStringT & str)
{
	TCHAR _str[8];
	stdext::jc_int2str( typ, _str, 10);
	str = _str;
}

template <> void CConvertor<unsigned short>::S2T(LPCTSTR str, unsigned short & typ)
{
	if ( _T('0') == str[0] && (_T('x') == str[1] || _T('X') == str[1]) )
		typ = (unsigned short)stdext::str2hex(str+2);
	else  typ = (unsigned short)stdext::jc_str2l(str);
}

//-- int -- 
template <> void CConvertor<int>::T2S(const int & typ, CJCStringT & str)
{
	TCHAR _str[16];
	stdext::jc_int2str( typ, _str, 10);
	str = _str;
}

template <> void CConvertor<int>::S2T(LPCTSTR str, int & typ)
{
	 typ = (int)stdext::jc_str2l(str);
}

//-- uint --
template <> void CConvertor<unsigned int>::T2S(const unsigned int & typ, CJCStringT & str)
{
	TCHAR _str[16];
	stdext::jc_uint2str( typ, _str, 10);
	str = _str;
}

template <> void CConvertor<unsigned int>::S2T(LPCTSTR str, unsigned int & typ)
{
	if ( _T('0') == str[0] && (_T('x') == str[1] || _T('X') == str[1]) )
		typ = (unsigned int)stdext::str2hex(str+2);
	else  typ = (unsigned int)stdext::jc_str2ul(str);
	//else  typ = _tcstoul(str, NULL, 10);
}

template <> void CConvertor<unsigned long>::T2S(const unsigned long & typ, CJCStringT & str)
{
	TCHAR _str[16];
	stdext::jc_uint2str( typ, _str, 10);
	str = _str;
}

template <> void CConvertor<unsigned long>::S2T(LPCTSTR str, unsigned long & typ)
{
	if ( _T('0') == str[0] && (_T('x') == str[1] || _T('X') == str[1]) )
		typ = (unsigned long)stdext::str2hex(str+2);
	else  typ = (unsigned long)stdext::jc_str2ul(str);
}


// -- int 64 --
template <> void CConvertor<INT64>::T2S(const INT64 & typ, CJCStringT & str)
{
	TCHAR _str[32];
	stdext::jc_int2str( typ, _str, 32, 10);
	str = _str;
}

template <> void CConvertor<INT64>::S2T(LPCTSTR str, INT64 & typ)
{
	 typ = stdext::jc_str2l(str);
}

//-- uint64 --
template <> void CConvertor<UINT64>::T2S(const UINT64 & typ, CJCStringT & str)
{
	TCHAR _str[32];
	stdext::jc_uint2str( typ, _str, 10);
	str = _str;
}

template <> void CConvertor<UINT64>::S2T(LPCTSTR str, UINT64 & typ)
{
	if ( _T('0') == str[0] && (_T('x') == str[1] || _T('X') == str[1]) )
		typ = stdext::str2hex(str + 2);
	else  typ = (UINT64)stdext::jc_str2l(str);
}

//--double--
template <> void CConvertor<double>::T2S(const double & typ, CJCStringT & str)
{
	TCHAR _str[32];
	stdext::jc_sprintf(_str, _T("%g"),  typ);
	str = _str;
}

template <> void CConvertor<double>::S2T(LPCTSTR str, double & typ)
{
	 typ = stdext::jc_str2f(str);
}

//-- float --
template <> void CConvertor<float>::T2S(const float & typ, CJCStringT & str)
{
	TCHAR _str[32];
	stdext::jc_sprintf(_str, _T("%g"),  typ);
	str = _str;
}

template <> void CConvertor<float>::S2T(LPCTSTR str, float & typ)
{
	 typ = (float)(stdext::jc_str2f(str));
}

// 字符串 -------------------------------------------------------------------------------------
template <> void CConvertor<CJCStringT>::T2S(const CJCStringT & typ, CJCStringT & str)
{
	str =  typ;
}

template <> void CConvertor<CJCStringT>::S2T(LPCTSTR str, CJCStringT & typ)
{
	 typ = str;
}

template <> void CConvertor<LPCTSTR>::T2S(const LPCTSTR & typ, CJCStringT & str)
{
	str =  typ;
}

template <> void CConvertor<LPCTSTR>::S2T(LPCTSTR str, LPCTSTR & typ)
{
	 typ = str;
}

}

// 其他 --------------------------------------------------------------------------------------
// these convertors are only support for win32
#if 0

template <> void CConvertor<COleVariant>::T2S(const COleVariant &t, CJCStringT & str)	
{
	str = t.bstrVal;
}

template <> void CConvertor<COleVariant>::S2T(LPCTSTR str, COleVariant &t)
{
	t = str;
}

template <> void CConvertor<CPoint>::T2S(const CPoint &t, CJCStringT &str)
{
	str.Format(_T("%d, %d"), t.x, t.y);
}

template <> void CConvertor<CPoint>::S2T(LPCTSTR str, CPoint &t)
{
	_stscanf_s(str, _T("%d, %d"), &t.x, &t.y);
}

template <> void CConvertor<CSize>::T2S(const CSize &t, CJCStringT &str)
{
	str.Format(_T("%d, %d"), t.cx, t.cy);
}

template <> void CConvertor<CSize>::S2T(LPCTSTR str, CSize &t)
{
	_stscanf_s(str, _T("%d, %d"), &t.cx, &t.cy);
}

#endif







