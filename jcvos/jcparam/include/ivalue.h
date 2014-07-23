#pragma once

#include "../../stdext/stdext.h"

namespace jcparam
{

	// VALUE_TYPE的值表示类型的宽度，用于判断拓展转换的方向
	// bit 7~4表示长度（字节），bit 3~0：类型区分
	enum VALUE_TYPE
	{
		VT_UNKNOW = 0x00,
		VT_BOOL = 1,
		VT_CHAR = 2,		VT_UCHAR = 3,
		VT_SHORT= 4,		VT_USHORT =5,
		VT_INT =  6,		VT_UINT =  7,
		VT_INT64 =8,		VT_UINT64 =9,
		VT_FLOAT =10,		VT_DOUBLE =11,
		VT_STRING =12,		VT_ENUM = 13, 
		VT_MAXNUM,
		VT_HEX, VT_OTHERS,
	};

	const TCHAR PROP_TYPE[] = _T("type");
	const TCHAR PROP_SUBTYPE[] = _T("subtype");
	const TCHAR PROP_CLASS[] = _T("class");
	const TCHAR PROP_PLUGIN[] = _T("plugin");

	enum READ_WRITE
	{
		WRITE = 0, READ = 1,
	};

///////////////////////////////////////////////////////////////////////////////
// -- stream
	class IIterator : virtual public IJCInterface
	{
	public:
		virtual IIterator& operator ++ (void) = 0;
		virtual wchar_t & operator * (void) = 0;
		virtual bool operator == (const IIterator *) = 0;
	};
	
	class IJCStream : virtual public IJCInterface
	{
	public:
		virtual void Put(wchar_t)	= 0;
		virtual wchar_t Get(void)	= 0;

		virtual void Put(const wchar_t * str, JCSIZE len) = 0;
		virtual JCSIZE Get(wchar_t * str, JCSIZE len) = 0;
		virtual void Format(LPCTSTR f, ...) = 0;
		virtual bool IsEof(void) = 0;

		virtual LPCTSTR GetName(void) const = 0;
	};

///////////////////////////////////////////////////////////////////////////////
// -- Interfaces
	class IValue : virtual public IJCInterface 
	{
	public:
		// 转换器
		virtual void GetSubValue(LPCTSTR name, IValue * & val) = 0;
		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val) = 0;
		virtual void GetValueText(CJCStringT & str) const = 0;
		virtual void SetValueText(LPCTSTR str)  = 0;
	};

	const char IF_NAME_VALUE_FORMAT[] = "IValueFormat";

	class IValueFormat/* : virtual public IValue*/
	{
	public:
		virtual void Format(FILE * file, LPCTSTR format) = 0;
		virtual void WriteHeader(FILE * file) = 0;
	};

	enum VAL_FORMAT
	{
		VF_DEFAULT = 0, 
		VF_FORMAT_MASK = 0xF0000000,
		VF_TEXT =	0x10000000, 
		VF_BINARY = 0x20000000,
		VF_HEAD =	0x08000000,
		VF_PARTIAL= 0x00800000,
	};

	class IVisibleValue : virtual public IValue
	{
	public:
		virtual void ToStream(IJCStream * stream, VAL_FORMAT fmt, DWORD param) const = 0;
		virtual void FromStream(IJCStream * str, VAL_FORMAT) = 0;
	};

///////////////////////////////////////////////////////////////////////////////
// -- tables
	struct COLUMN_INFO_BASE
	{
	public:
		COLUMN_INFO_BASE(JCSIZE id, VALUE_TYPE type, JCSIZE offset, LPCTSTR name)
			: m_id(id), m_type(type), m_offset(offset),
			m_name(name)
		{};

		void GetText(void * row, CJCStringT & str) const;

		virtual void ToStream(void * row, IJCStream * stream, jcparam::VAL_FORMAT fmt) const {};
		virtual void CreateValue(BYTE * src, IValue * & val) const {};
		virtual void GetColVal(BYTE * src, void *) const {};
		LPCTSTR name(void) const {return m_name.c_str(); }

	public:
		JCSIZE		m_id;
		VALUE_TYPE	m_type;
		JCSIZE		m_offset;
		CJCStringT	m_name;
	};

	class IColInfoList	: virtual public IJCInterface
	{
	public:
		virtual void AddInfo(const COLUMN_INFO_BASE* info) = 0;
		virtual const COLUMN_INFO_BASE * GetInfo(const CJCStringT & key) const = 0;
		virtual const COLUMN_INFO_BASE * GetInfo(JCSIZE index) const = 0;
		virtual JCSIZE GetColNum(void) const =0;
		virtual void OutputHead(IJCStream * stream) const = 0;
	};

	class IVector : /*virtual*/ public IValue
	{
	public:
		virtual void PushBack(IValue * val) = 0;
		virtual void GetRow(JCSIZE index, IValue * & val) = 0;
		virtual JCSIZE GetRowSize() const = 0;
	};

	class ITable : virtual public IVector, virtual public IVisibleValue
	{
		// for column access
	public:
		virtual JCSIZE GetColumnSize() const = 0;
	};

	class ITableRow : public IVisibleValue
	{
		// for column(field) access
	public:
		virtual int GetColumnSize() const = 0;
		virtual const COLUMN_INFO_BASE * GetColumnInfo(LPCTSTR field_name) const = 0;
		virtual const COLUMN_INFO_BASE * GetColumnInfo(int field) const = 0;
		// 从一个通用的行中取得通用的列数据
		virtual void GetColumnData(int field, IValue * &)	const = 0;

	public:
		// 从row的类型创建一个表格
		virtual bool CreateTable(ITable * & tab) = 0;
	};

///////////////////////////////////////////////////////////////////////////////
// -- Factory
	void CreateColumnInfoList(IColInfoList * & list);
	void CreateGeneralTable(IColInfoList * info, ITable * & table);

///////////////////////////////////////////////////////////////////////////////////////////////////////
// CConvertor<T>
//	类型转换器
//	
	template <typename T>
	class CConvertor
	{
	public:
		static void T2S(const T &t, CJCStringT &v);
		static void S2T(LPCTSTR, T &t);
	};

	template <typename T, UINT L>
	class CHexConvertorL
	{
	public:
		static void T2S(const T &typ, CJCStringT &str)
		{
			TCHAR _str[20];
			JCASSERT(L < 20);
			JCASSERT(L > 0);
			T _t = typ;
			wmemset(_str, _T('0'), L);
			_str[L] = 0;
			for (int ii = L-1; (ii >= 0) && (_t != 0); --ii, _t >>= 4)
			{
				BYTE v = (_t & 0xF);
				if ( v < 10) 	_str[ii] = _T('0') + v;
				else			_str[ii] = _T('A') + v - 10;
			}
			str = _str;
		}

		static void S2T(LPCTSTR str, T &typ)
		{
			LPTSTR end = NULL;
			typ = (T)stdext::jc_str2l(str, &end, (int)10);
		}
	};

	template <typename T>
	class CHexConvertor
	{
	public:
		static void T2S(const T &typ, CJCStringT &str)
		{
			TCHAR _str[20];
			stdext::jc_sprintf(_str, _T("%X"),  typ);
			str = _str;
		}

		static void S2T(LPCTSTR str, T &typ)
		{
			LPTSTR end = NULL;
			typ = (T)stdext::jc_str2l(str, &end, (int)10);
		}
	};

// 一些基本实现
	class CTypedValueBase : virtual public IValue
	{
	public:
		virtual VALUE_TYPE GetType(void) const = 0;
		virtual void GetSubValue(LPCTSTR name, IValue * & val);
		virtual void SetSubValue(LPCTSTR name, IValue * val);
		virtual const void * GetData(void) const = 0;
	};
};


typedef stdext::auto_interface<jcparam::IValue> AUTO_IVAL;