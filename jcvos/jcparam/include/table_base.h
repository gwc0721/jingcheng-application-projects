#pragma once

#include "table.h"
#include <vector>

#include "string_table.h"

#define BEGIN_COLUMN_TABLE()	\
	const jcparam::CColumnInfoTable jcparam::	\
	CTableRowBase<__COL_CLASS_NAME>::m_column_info(	\
	jcparam::CColumnInfoTable::RULE()

#define COLINFO(var_typ, cov, id, var_name, col_name)	\
	( new jcparam::CTypedColInfo<var_typ, cov >(id, offsetof(__COL_CLASS_NAME, \
	var_name), col_name ) )

#define COLINFO_LOC(var_typ, id, cov, var_name, col_name)	\
	( new jcparam::CTypedColInfo<__COL_CLASS_NAME::var_typ, __COL_CLASS_NAME::cov >(	\
	id, offsetof(__COL_CLASS_NAME, var_name), col_name ) )

#define COLINFO_HEX(var_typ, id, var_name, col_name)	\
	( new jcparam::CTypedColInfo<var_typ, jcparam::CHexConvertor<var_typ> >(	\
	id, offsetof(__COL_CLASS_NAME, var_name), col_name ) )

#define COLINFO_HEXL(var_typ, len, id, var_name, col_name)	\
	( new jcparam::CTypedColInfo<var_typ, jcparam::CHexConvertorL<var_typ, len> >(	\
	id, offsetof(__COL_CLASS_NAME, var_name), col_name ) )

#define COLINFO_DEC(var_typ, id, var_name, col_name)	\
	( new jcparam::CTypedColInfo<var_typ >(id, offsetof(__COL_CLASS_NAME, \
	var_name), col_name ) )

#define COLINFO_STR(id, var_name, col_name)	\
	( new jcparam::CStringColInfo(id, offsetof(__COL_CLASS_NAME, var_name), col_name ) )

#define COLINFO_TYPE(var_typ, id, var_name, col_name)	\
	( new jcparam::CTypedColInfo<var_typ, jcparam::CConvertor<var_typ> >(id, offsetof(__COL_CLASS_NAME, \
	var_name), col_name ) )

#define COLINFO_FLOAT(var_typ, id, var_name, col_name)	\
	( new jcparam::CTypedColInfo<var_typ >(id, offsetof(__COL_CLASS_NAME, \
	var_name), col_name ) )

#define END_COLUMN_TABLE()	);

namespace jcparam
{
	typedef jcparam::CStringTable<CColInfoBase, std::vector<const CColInfoBase*> > CColumnInfoTable;

	template <class ROW_BASE_TYPE>
	class CTableRowBase 
		: public ROW_BASE_TYPE, public CJCInterfaceBase
		, virtual public ITableRow
		, virtual public IValueFormat
	{
	public:
		CTableRowBase<ROW_BASE_TYPE>() {};
		CTableRowBase<ROW_BASE_TYPE>(const ROW_BASE_TYPE & row) : ROW_BASE_TYPE(row)
		{
		}

		virtual void GetSubValue(LPCTSTR name, IValue * & val)
		{
			JCASSERT(NULL == val);
			const CColInfoBase * col_info = m_column_info.GetItem(name);
			if (!col_info) THROW_ERROR(ERR_APP, _T("Column %s does`t exist."), name);
			GetColumnData(col_info, val);			
		}

		virtual void SetSubValue(LPCTSTR name, IValue * val)
		{
			THROW_ERROR(ERR_APP, _T("Table do not support add sub value"));
		}

		virtual JCSIZE GetRowID(void) const {return m_id; }
		virtual int GetColumnSize() const {return m_column_info.GetSize();}

		// 从一个通用的行中取得通用的列数据


	
		virtual void GetColumnData(int field, IValue * & val)	const
		{
			JCASSERT(NULL == val);
			const CColInfoBase * col_info = m_column_info.GetItem(field);
			GetColumnData(col_info, val);
		}

		virtual void GetColVal(int field, void * val) const
		{
			JCASSERT((JCSIZE)(field) < m_column_info.GetSize() );
			const CColInfoBase * col_info = m_column_info.GetItem(field);
			col_info->GetColVal( (BYTE*)(static_cast<const ROW_BASE_TYPE*>(this)), val );
		}

		//virtual void GetColumnData(LPCTSTR field_name, IValue * & val) const
		//{
		//	JCASSERT(NULL == val);
		//	const CColInfoBase * col_info = m_column_info.GetItem(field_name);
		//	if (!col_info) THROW_ERROR(ERR_APP, _T("Column %s does`t exist."), field_name);
		//	GetColumnData(col_info, val);
		//}

		virtual const CColInfoBase * GetColumnInfo(LPCTSTR field_name) const
		{
			const CColInfoBase * col_info = m_column_info.GetItem(field_name);
			return col_info;
		}


		virtual const CColInfoBase * GetColumnInfo(int field) const
		{
			return m_column_info.GetItem(field);
		}

		virtual bool CreateTable(ITable * & tab)
		{
			JCASSERT(NULL == tab);
			CTypedTable<ROW_BASE_TYPE> * _tab = NULL;
			CTypedTable<ROW_BASE_TYPE>::Create(5, _tab);
			tab = static_cast<ITable *>(_tab);
			return true;
		}

	public:
		virtual void GetColumnText(int field, CJCStringT & str)
		{
			JCASSERT(field < GetColumnSize() );
			const CColInfoBase * col_info = m_column_info.GetItem(field);
			ROW_BASE_TYPE * row = static_cast<ROW_BASE_TYPE*>(this);
			col_info->GetText(row, str);
		}

		virtual LPCTSTR GetColumnName(int field_id) const
		{
			JCASSERT(field_id < GetColumnSize() );
			const CColInfoBase * col_info = m_column_info.GetItem(field_id);
			return col_info->m_name.c_str();
		}

		static const CColumnInfoTable * GetColumnInfo(void) { return &m_column_info; }

		virtual void WriteHeader(FILE * file)
		{
			// output head
			JCSIZE col_size = GetColumnSize();
			for (JCSIZE ii = 0; ii < col_size; ++ii)
			{
				const CColInfoBase * col_info = GetColumnInfo(ii);
				JCASSERT(col_info);
				fprintf_s(file, "%S,", col_info->m_name.c_str());
			}
			fprintf_s(file, "\n");
		}

		virtual void Format(FILE * file, LPCTSTR format)
		{
			JCSIZE col_size = GetColumnSize();
			ROW_BASE_TYPE * row = static_cast<ROW_BASE_TYPE*>(this);
			CJCStringT str;

			for (JCSIZE ii = 0; ii < col_size; ++ii)
			{
				CJCStringT str;
				const CColInfoBase * col_info = GetColumnInfo(ii);
				col_info->GetText(row, str);
				fprintf_s(file, "%S,", str.c_str());
			}
			fprintf_s(file, "\n");
		}

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
	
	protected:
		void GetColumnData(const CColInfoBase * col, IValue * & val) const
		{
			JCASSERT(col);
			col->CreateValue((BYTE*)(static_cast<const ROW_BASE_TYPE*>(this)), val);
		}			
		
		IValue * GetColumnData(const CColInfoBase * col_info) const
		{
			//JCASSERT(col_info);
			//void * p = static_cast<const ROW_BASE_TYPE*>(this);
			//return CreateTypedValue(col_info->m_type, p + m_offset);
			return NULL;
		}

	protected:
		static const CColumnInfoTable		m_column_info;
	};



	template <class ROW_BASE_TYPE, class ROW_TYPE = CTableRowBase<ROW_BASE_TYPE> >
	class CTypedTable 
		: virtual public ITable, virtual public IValueFormat
		, public CJCInterfaceBase
	{
	public:
		static void Create(JCSIZE reserved_size, CTypedTable<ROW_BASE_TYPE, ROW_TYPE> * & table)
		{
			JCASSERT(NULL == table);
			table = new CTypedTable<ROW_BASE_TYPE, ROW_TYPE>(reserved_size);
		}

	protected:
		typedef CTypedTable<ROW_BASE_TYPE, ROW_TYPE>		THIS_TABLE_TYPE;
		typedef std::vector<ROW_BASE_TYPE>			ROW_TABLE;
		typedef typename ROW_TABLE::iterator		ROW_ITERATOR;
		
		CTypedTable(JCSIZE reserved_size) : m_table() { m_table.reserve(reserved_size); }

	public:
		virtual void GetValueText(CJCStringT & str) const {};
		virtual void SetValueText(LPCTSTR str)  {} ;

		// 列存取
		virtual void GetSubValue(LPCTSTR name, IValue * & val)
		{
			const CColumnInfoTable * col_list = ROW_TYPE::GetColumnInfo();
			const CColInfoBase * col_info = col_list->GetItem(name);

			CColumn * col = new CColumn(this, col_info);
			val = static_cast<IValue*>(col);
		}

		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val)
		{
			THROW_ERROR(ERR_APP, _T("Table do not support add sub value"));
		}

		virtual JCSIZE GetRowSize() const
		{
			return m_table.size();
		}

		virtual void GetRow(JCSIZE index, IValue * & ptr_row)
		{
			ROW_BASE_TYPE & row = m_table.at(index);
			ptr_row = static_cast<IValue*>(new ROW_TYPE( row ));
		}

		virtual JCSIZE GetColumnSize() const
		{
			const CColumnInfoTable * col_list = ROW_TYPE::GetColumnInfo();
			JCASSERT(col_list);
			return col_list->GetSize();
		}

		virtual void WriteHeader(FILE * file) 		{}

		virtual void Format(FILE * file, LPCTSTR format)
		{
			// default : csv
			// output head
			const CColumnInfoTable * col_list = ROW_TYPE::GetColumnInfo();
			JCASSERT(col_list);
			JCSIZE col_size = col_list->GetSize();
			for (JCSIZE ii = 0; ii < col_size; ++ii)
			{
				const CColInfoBase * col_info = col_list->GetItem(ii);
				JCASSERT(col_info);
				//stdext::jc_fprintf(file, _T("%s, "), col_info->m_name.c_str());
				fprintf_s(file, "%S,", col_info->m_name.c_str());
			}
			//stdext::jc_fprintf(file, _T("\n"));
			fprintf_s(file, "\n");

			ROW_ITERATOR it = m_table.begin();
			ROW_ITERATOR endit = m_table.end();
			for ( ; it!=endit; ++it)
			{
				for (JCSIZE ii = 0; ii < col_size; ++ii)
				{
					CJCStringT str;
					const CColInfoBase * col_info = col_list->GetItem(ii);
					col_info->GetText( &(*it), str);
					//stdext::jc_fprintf(file, _T("%s, "), str.c_str());
					fprintf_s(file, "%S,", str.c_str());
				}
				//stdext::jc_fprintf(file, _T("\n"));
				fprintf_s(file, "\n");
			}
		}

		bool QueryInterface(const char * if_name, IJCInterface * &if_ptr)
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

		void Append(IValue * source)
		{
			THIS_TABLE_TYPE * _source = dynamic_cast<THIS_TABLE_TYPE *>(source);
			if (!_source) THROW_ERROR(ERR_PARAMETER, _T("Table type is different"));
			m_table.insert(m_table.end(), _source->m_table.begin(), _source->m_table.end());
		}

		virtual void AddRow(ITableRow * row)
		{
			ROW_BASE_TYPE * _row = dynamic_cast<ROW_BASE_TYPE*> (row);
			if (!_row) THROW_ERROR(ERR_PARAMETER, _T("row type does not match table type"));
			push_back(*_row);
		}


	public:
		virtual void push_back(const ROW_BASE_TYPE & row_base)
		{
			m_table.push_back(row_base);
		}

	protected:
		ROW_TABLE		m_table;
		//类型验证
	};

	class CColumn : virtual public ITable, public CJCInterfaceBase
	{
	public:
		CColumn(ITable * tab, const CColInfoBase * col_info);
		~CColumn(void);

	public:
		virtual JCSIZE GetRowSize() const;
		virtual void GetRow(JCSIZE index, IValue * & row);
		virtual JCSIZE GetColumnSize() const;
		virtual void Append(IValue * source);
		virtual void GetSubValue(LPCTSTR name, IValue * & val);
		// 如果name不存在，则插入，否则修改name的值
		virtual void SetSubValue(LPCTSTR name, IValue * val);
		virtual void AddRow(ITableRow * row);

	protected:
		ITable * m_parent_tab;
		const CColInfoBase * m_col_info;
	};
};
