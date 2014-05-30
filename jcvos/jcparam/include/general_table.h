#pragma once

#include "ivalue.h"
#include "value.h"
#include "table_base.h"
#include <vector>

namespace jcparam
{
	//void CreateColumnInfoList(IColInfoList * & list);
	class CColInfoList : public IColInfoList
		, public CJCInterfaceBase
		, CStringTable<COLUMN_INFO_BASE, std::vector<const COLUMN_INFO_BASE*> >
	{
	public:
		CColInfoList(void) {};
		virtual ~CColInfoList(void) {};
		typedef CStringTable<COLUMN_INFO_BASE, std::vector<const COLUMN_INFO_BASE*> >	LIST_BASE;
	public:
		//friend void CreateColumnInfoList(IColInfoList * & list);
	public:
		virtual void AddInfo(const COLUMN_INFO_BASE* info);
		virtual const COLUMN_INFO_BASE * GetInfo(const CJCStringT & key) const;
		virtual const COLUMN_INFO_BASE * GetInfo(JCSIZE index) const;
		virtual JCSIZE GetColNum(void) const {return LIST_BASE::GetSize();}
	};

	// CGeneralRow: 用逗号分开的字符串存储行。CGeneralRow管理一个动态的COLUMN_INFO数组。
	//	并且通过COLUMN_INFO来识别各个字段。
	//  客户可以使用printf等语句直接产生行的数据。
	class CGeneralTable;

	class CGeneralRow : public ITableRow
		, public virtual IVisibalValue
		, public CJCInterfaceBase
	{
	protected:
		CGeneralRow(IColInfoList * info);
		virtual ~CGeneralRow(void);
	public:
		static void CreateGeneralRow(IColInfoList * info, CGeneralRow * &row);
	public:
		// IValue
		virtual void GetSubValue(LPCTSTR name, IValue * & val);
		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val);
		virtual void GetValueText(CJCStringT & str) const;
		virtual void SetValueText(LPCTSTR str);

		// ITableRow
		virtual int GetColumnSize() const;
		virtual const COLUMN_INFO_BASE * GetColumnInfo(LPCTSTR field_name) const;
		virtual const COLUMN_INFO_BASE * GetColumnInfo(int field) const;
		// 从一个通用的行中取得通用的列数据
		virtual void GetColumnData(int field, IValue * &)	const;
		//virtual JCSIZE GetRowID(void) const = 0;
		// 从row的类型创建一个表格
		virtual bool CreateTable(ITable * & tab);

		virtual void ToStream(IStream * str, VAL_FORMAT) const;
		virtual void FromStream(IStream * str, VAL_FORMAT);

	protected:
		void GetColumnData(const COLUMN_INFO_BASE *info, IValue * &)	const;

	protected:
		JCSIZE	m_col_num;
		TCHAR	* m_data;
		JCSIZE	m_data_len;
		//每个字段在行中的偏移量，行的长度不超过64K字符
		struct FIELDS
		{
			WORD	offset;
			WORD	len;
		};
		FIELDS	*m_fields;

		IColInfoList	* m_col_info;

		//CJCStringT	m_data;
		//CGeneralTable * m_table;
	};

	class CGeneralTable : virtual public ITable
				, public CJCInterfaceBase
	{
	public:
		CGeneralTable(IColInfoList * info);
		virtual ~CGeneralTable(void);

	public:
		//static void CreateGeneralTable(IColInfoList * info, ITable * & table);

	public:
		// IValue
		virtual void GetSubValue(LPCTSTR name, IValue * & val);
		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val) {};	// do not support
		virtual void GetValueText(CJCStringT & str) const {};		// do not support now
		virtual void SetValueText(LPCTSTR str)  {};					// do not support now

		// IVector
		virtual void PushBack(IValue * val);
		virtual void GetRow(JCSIZE index, IValue * & val);
		virtual JCSIZE GetRowSize() const;
		// ITable
		virtual JCSIZE GetColumnSize() const;
		//virtual bool GetColumn(int filed, IVector * &) const	= 0;
		//virtual const COLUMN_INFO_BASE * GetColumnInfo(LPCTSTR field_name) const = 0;
		//virtual const COLUMN_INFO_BASE * GetColumnInfo(int field) const = 0;

		//virtual void Append(IValue * source) {};
	public:
		bool AddColumnInfo(COLUMN_INFO_BASE * col);
		bool CloseColumnInfo(void);
		bool CreateRow(CGeneralRow * & row);

	protected:
		IColInfoList * m_col_info;
		
		typedef std::vector<CGeneralRow *> ROWS;
		ROWS	m_rows;
	};

	class CGeneralColumn : virtual public IVector
				, public CJCInterfaceBase
	{
	public:
		CGeneralColumn(CGeneralTable *, const COLUMN_INFO_BASE *);
		~CGeneralColumn(void);

	public:
		virtual void GetSubValue(LPCTSTR name, IValue * & val) {};
		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val) {};
		virtual void GetValueText(CJCStringT & str) const{};
		virtual void SetValueText(LPCTSTR str) {};
		// read only
		virtual void PushBack(IValue * val){};
		virtual void GetRow(JCSIZE index, IValue * & val);
		virtual JCSIZE GetRowSize() const;

	protected:
		CGeneralTable * m_table;
		const COLUMN_INFO_BASE * m_col_info;
	};
};