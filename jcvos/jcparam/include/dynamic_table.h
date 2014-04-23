#pragma once

// Dynamic table：
//   ITable的一个实现。在这个实现中，application可以动态的创建和删除列(Column)。
//	然后可以向table中添加行。

#include "value.h"
#include "table.h"
#include "table_base.h"
#include <vector>
#include <map>

namespace jcparam
{
	class CTypedColumnBase
		: virtual public ITable
		, virtual public IValueFormat
		, public CJCInterfaceBase	
	{
	public:
		virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr)
		{
			JCASSERT(NULL == if_ptr);
			bool br = false;
			if ( FastCmpA(IF_NAME_VALUE_FORMAT, if_name) )
			{
				if_ptr = static_cast<IJCInterface*>(this);
				if_ptr->AddRef();
				br = true;
			}
			else br = __super::QueryInterface(if_name, if_ptr);
			return br;
		}
	};

	template <typename DATATYPE>
	class CTypedColumn
		: public CTypedColumnBase
		, public std::vector<DATATYPE>, public CTypedValueBase
	{
	protected:
		typedef typename std::vector<DATATYPE> VECTOR_BASE_CLASS;
		CTypedColumn(JCSIZE count)
		{
			if (count) VECTOR_BASE_CLASS::reserve(count);
		};
		~CTypedColumn(void) {};

	public:
		static void Create(JCSIZE size, CTypedColumn<DATATYPE> * & col)
		{
			JCASSERT(NULL == col);
			col = new CTypedColumn<DATATYPE>(size);
		}

	public:
		virtual JCSIZE GetRowSize() const {return (JCSIZE)(VECTOR_BASE_CLASS::size()); };

		virtual void GetRow(JCSIZE index, IValue * & row)
		{
			JCASSERT(NULL == row);
			JCASSERT(index < GetRowSize() );

			DATATYPE val = VECTOR_BASE_CLASS::at(index);
			row = static_cast<IValue*>(CTypedValue<DATATYPE>::Create(val));
		}

		virtual JCSIZE GetColumnSize() const {return 1;}

		virtual void Append(IValue * source)
		{
			CTypedColumn<DATATYPE> * col = dynamic_cast<CTypedColumn<DATATYPE> *>(source);
			if (!col) THROW_ERROR(ERR_PARAMETER, _T("Type dismatched."));
			VECTOR_BASE_CLASS::insert(VECTOR_BASE_CLASS::end(), col->begin(), col->end());
		}

		virtual void Format(FILE * file, LPCTSTR format)
		{
			CJCStringT str;
			VECTOR_BASE_CLASS::iterator it = VECTOR_BASE_CLASS::begin();
			VECTOR_BASE_CLASS::iterator endit = VECTOR_BASE_CLASS::end();
			for ( ; it != endit; ++it)
			{
				CConvertor<DATATYPE>::T2S((*it), str);
				stdext::jc_fprintf(file, str.c_str());
				stdext::jc_fprintf(file, _T(", \n"));
			}
		}
		virtual void WriteHeader(FILE * file)
		{
		}


		virtual void PushBack(IValue * row)
		{
			JCASSERT(0);
			//DATATYPE * _row = dynamic_cast<DATATYPE*> (row);
			//if (!_row) THROW_ERROR(ERR_PARAMETER, _T("row type does not match table type"));
			//push_back(&_row);
		}

	protected:
	};

	class CDynamicTable;

	class CDynamicRow
		: virtual public ITableRow
		, public CJCInterfaceBase
	{
	protected:
		CDynamicRow(JCSIZE id, CDynamicTable * tab);
		~CDynamicRow(void);

	public:
		virtual JCSIZE GetRowID(void) const {return m_id;}
		virtual int GetColumnSize() const;
		virtual void GetColumnData(LPCTSTR field_name, IValue * &) const = 0;

		virtual LPCTSTR GetColumnName(int field_id) const = 0;	
		virtual void GetSubValue(LPCTSTR name, IValue * & val) {};
		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val){};

	protected:
		JCSIZE			m_id;
		CDynamicTable * m_table;
	};

	class CDynamicTable
		: virtual public ITable
		, virtual public IValueFormat
		, public CJCInterfaceBase
	{
	protected:
		CDynamicTable(void) : m_row_size(0) {};
		~CDynamicTable(void);

	public:
		virtual JCSIZE GetRowSize() const { return m_row_size; }
		virtual void GetRow(JCSIZE index, IValue * & row);
		virtual JCSIZE GetColumnSize() const { return m_column_map.size(); }
		virtual void Append(IValue * source);

		virtual void GetValueText(CJCStringT & str) const {};
		virtual void SetValueText(LPCTSTR str) {};
		virtual void GetSubValue(LPCTSTR name, IValue * & val);
		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val){};

	public:
		template <typename DATATYPE>
		void AddColumn(CJCStringT & col_name)
		{
			CTypedColumnBase * column;
			CTypedColumn<DATATYPE>::Create(m_row_size, column);
			m_columns.push_back(column);
		}

		void push_back();

	protected:
		typedef std::vector<CTypedColumnBase *>	COLUMN_LIST;
		typedef std::map<CJCStringT, CTypedColumnBase *>	COLUMN_MAP;

	protected:
		JCSIZE	m_row_size;
		COLUMN_MAP	m_column_map;
	};

///////////////////////////////////////////////////////////////////////////////
//--
	class CDynaTab;

	class CDynaRow
		: virtual public ITableRow
		, public CJCInterfaceBase
	{
		friend class CDynaTab;

	protected:
		CDynaRow(CColumnInfoTable * col_info, JCSIZE m_id);
		~CDynaRow();

	public:
		virtual void GetValueText(CJCStringT & str) const {};
		virtual void SetValueText(LPCTSTR str)  {};

		virtual JCSIZE GetRowID(void) const {return m_id;}
		virtual int GetColumnSize() const;
		//virtual void GetColumnData(LPCTSTR field_name, IValue * &) const;
		virtual void GetColumnData(int field, IValue * &)	const;
		virtual const COLUMN_INFO_BASE * GetColumnInfo(LPCTSTR field_name) const
		{
			return m_col_info->GetItem(field_name);
		}
		virtual void GetColVal(int field, void *val) const
		{
			JCASSERT((JCSIZE)field < m_col_info->GetSize() );
			const COLUMN_INFO_BASE * col_info = m_col_info->GetItem(field);
			col_info->GetColVal(  (BYTE*)(static_cast<const ITableRow*>(this)), val );
		}
		virtual LPCTSTR GetColumnName(int field_id) const;	
		virtual void GetSubValue(LPCTSTR name, IValue * & val);
		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val);
		//
		virtual const COLUMN_INFO_BASE * GetColumnInfo(int field) const;
		virtual bool CreateTable(ITable * & tab);

	public:
		void SetColumnVal(JCSIZE col_id, IValue * val);
		void Format(FILE * file, LPCTSTR format);

	protected:
		typedef IValue *	PIVALUE;
		CColumnInfoTable *	m_col_info;
		PIVALUE	* m_cols;
		JCSIZE		m_col_size;
		JCSIZE		m_id;
	};

	class CDynaTab
		: virtual public ITable
		, virtual public IValueFormat
		, public CJCInterfaceBase
	{
	protected:
		CDynaTab(void);
		~CDynaTab(void);

	public:
		static void Create(CDynaTab * & tab) { JCASSERT(NULL == tab); tab = new CDynaTab();};
		virtual void GetValueText(CJCStringT & str) const {};
		virtual void SetValueText(LPCTSTR str)  {};

	public:
		virtual JCSIZE GetRowSize() const { return m_rows.size(); }
		virtual void GetRow(JCSIZE index, IValue * & row);
		virtual JCSIZE GetColumnSize() const { return m_col_info.GetSize(); }
		virtual void Append(IValue * source);

		virtual void GetSubValue(LPCTSTR name, IValue * & val) {};

		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val){};
		virtual void Format(FILE * file, LPCTSTR format);
		virtual void WriteHeader(FILE *) {};
		virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr);

		// 添加一行数据到表格
		virtual void PushBack(IValue * row);

	public:
		void AddColumn(LPCTSTR name);
		// 从table的类型生成row
		bool CreateRow(CDynaRow *& row);

	protected:
		typedef std::vector<CDynaRow *>		ROW_LIST;

		CColumnInfoTable	m_col_info;
		ROW_LIST			m_rows;
	};


}