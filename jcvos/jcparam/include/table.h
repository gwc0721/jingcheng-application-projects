#pragma once

#include "ivalue.h"

namespace jcparam
{
	class CColInfoBase
	{
	public:
		CColInfoBase(JCSIZE id, VALUE_TYPE type, JCSIZE offset, LPCTSTR name)
			: m_id(id), m_type(type), /*m_width(width),*/ m_offset(offset),
			m_name(name)
		{};

		//CColInfoBase(JCSIZE id, JCSIZE width, JCSIZE offset, LPCTSTR name)
		//	: m_id(id), m_width(width), m_offset(offset),
		//	m_name(name)
		//{};

		virtual void GetText(void * row, CJCStringT & str) const {};
		virtual void CreateValue(BYTE * src, IValue * & val) const {};
		virtual void GetColVal(BYTE * src, void *) const {};
		LPCTSTR name(void) const {return m_name.c_str(); }

	public:
		JCSIZE		m_id;
		VALUE_TYPE	m_type;
		//JCSIZE		m_width;
		JCSIZE		m_offset;
		CJCStringT	m_name;
	};

	template <typename VAL_TYPE, class CONVERTOR = CConvertor<VAL_TYPE> >
	class CTypedColInfo : public CColInfoBase
	{
	public:
		//CTypedColInfo (int id, JCSIZE offset, LPCTSTR name)
		//	: CColInfoBase(id, sizeof(VAL_TYPE), offset, name)
		//{};
		CTypedColInfo (int id, JCSIZE offset, LPCTSTR name)
			: CColInfoBase(id, type_id<VAL_TYPE>::id(), offset, name)
		{	JCASSERT(1);
		};
		virtual void GetText(void * row, CJCStringT & str) const
		{
			VAL_TYPE * val = reinterpret_cast<VAL_TYPE*>((BYTE*)row + m_offset);
			CONVERTOR::T2S(*val, str);
		}
		virtual void CreateValue(BYTE * src, IValue * & val) const
		{
			JCASSERT(NULL == val);
			BYTE * p = src + m_offset;
			VAL_TYPE & vsrc= *((VAL_TYPE*)p);
			val = CTypedValue<VAL_TYPE>::Create(vsrc);
		}
		virtual void GetColVal(BYTE * src, void * val) const
		{
			JCASSERT(src);
			JCASSERT(val);

			BYTE * p = src + m_offset;
			VAL_TYPE * _v = (VAL_TYPE *)(val);
			*_v = *((VAL_TYPE *)p);
		}
	};

	class CStringColInfo : public CColInfoBase
	{
	public:
		CStringColInfo (int id, JCSIZE offset, LPCTSTR name)
			: CColInfoBase(id, VT_STRING, offset, name)
		{};
		virtual void GetText(void * row, CJCStringT & str) const
		{
			str = *(reinterpret_cast<CJCStringT*>((BYTE*)row + m_offset));
		}
		virtual void CreateValue(BYTE * src, IValue * & val) const
		{
			JCASSERT(NULL == val);
			BYTE * p = src + m_offset;
			val = CTypedValue<CJCStringT>::Create(*(reinterpret_cast<CJCStringT*>(p)));
		}
	};	
	
	class ITableRow;
	class ITableColumn;

	class ITable : virtual public IValue
	{
	public:
		virtual JCSIZE GetRowSize() const = 0;
		virtual void GetRow(JCSIZE index, IValue * & row) = 0;
		//virtual void NextRow(IValue * & row) = 0;
		virtual JCSIZE GetColumnSize() const = 0;
		virtual void Append(IValue * source) = 0;
		virtual void AddRow(ITableRow * row) = 0;
	};

	class ITableRow : virtual public IValue
	{
	public:
		virtual JCSIZE GetRowID(void) const = 0;
		virtual int GetColumnSize() const = 0;

		// 从一个通用的行中取得通用的列数据
		//virtual void GetColumnData(const CColInfoBase * col, IValue * & val) const = 0;

		virtual void GetColumnData(int field, IValue * &)	const = 0;
		virtual const CColInfoBase * GetColumnInfo(LPCTSTR field_name) const = 0;
		virtual void GetColVal(int field, void *) const = 0;
		//virtual void GetColumnData(LPCTSTR field_name, IValue * &) const = 0;

		//virtual LPCTSTR GetColumnName(int field_id) const = 0;
		virtual const CColInfoBase * GetColumnInfo(int field) const = 0;
		// 从row的类型创建一个表格
		virtual bool CreateTable(ITable * & tab) = 0;
	};

};