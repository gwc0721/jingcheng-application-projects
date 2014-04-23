#include "stdafx.h"
#include "../include/dynamic_table.h"

using namespace jcparam;
///////////////////////////////////////////////////////////////////////////////
//-- 
LOCAL_LOGGER_ENABLE(_T("dynamic_table"), LOGGER_LEVEL_DEBUGINFO);

LOG_CLASS_SIZE_T( CTypedColumn<int>, 1 )


///////////////////////////////////////////////////////////////////////////////
//-- CDynamicRow
CDynamicRow::CDynamicRow(JCSIZE id, CDynamicTable * tab) 
	: m_id(id)
	, m_table(tab)
{
	JCASSERT(m_table);
	m_table->AddRef();
}

CDynamicRow::~CDynamicRow(void)
{
	m_table->Release();
}

void CDynamicRow::GetColumnData(LPCTSTR field_name, IValue * & val) const
{
	JCASSERT(m_table);
	JCASSERT(NULL == val);

	stdext::auto_cif<CTypedColumnBase, IValue>	col;
	m_table->GetSubValue(field_name, col);
	JCASSERT(col.valid());
	col->GetRow(m_id, val);
}

int CDynamicRow::GetColumnSize() const 
{
	return m_table->GetColumnSize();
}


///////////////////////////////////////////////////////////////////////////////
//-- CDynamicTable

CDynamicTable::~CDynamicTable(void)
{
	COLUMN_MAP::iterator it = m_column_map.begin();
	COLUMN_MAP::iterator endit = m_column_map.end();
	for ( ; it != endit; ++it)	(it->second)->Release();
}

void CDynamicTable::GetRow(JCSIZE index, IValue * & row)
{
	// TODO
}

void CDynamicTable::Append(IValue * source)
{
	// TODO: 扩展各列前，先要检查整个表格的匹配性

	CDynamicTable * table = dynamic_cast<CDynamicTable *>(source);
	if (!table) THROW_ERROR(ERR_PARAMETER, _T("Different table type") );
	
	COLUMN_MAP::iterator it = m_column_map.begin();
	COLUMN_MAP::iterator endit = m_column_map.end();

	COLUMN_MAP::iterator src_it = table->m_column_map.begin();
	COLUMN_MAP::iterator src_end = table->m_column_map.end();

	for ( ; (it != endit) && (src_it != src_end); ++it, ++src_it)
	{
		if (it->first != src_it->first)
			THROW_ERROR(ERR_PARAMETER, _T("Column does't match. %s != %s"), it->first.c_str(), src_it->first.c_str());
		CTypedColumnBase * col = (it->second);
		JCASSERT(col);
		CTypedColumnBase * src_col = (src_it->second);
		JCASSERT(src_col);
		
		col->Append(src_col);
		m_row_size = col->GetRowSize();
	}
}

void CDynamicTable::GetSubValue(LPCTSTR name, IValue * & val)
{
	JCASSERT(name);
	JCASSERT(NULL == val);

	COLUMN_MAP::iterator it = m_column_map.find(name);
	if ( it == m_column_map.end() ) return;
	val = it->second;
	JCASSERT(val);
	val->AddRef();
}


///////////////////////////////////////////////////////////////////////////////
//--
CDynaTab::CDynaTab(void)
	: m_col_info( CColumnInfoTable::RULE() )
{
}

CDynaTab::~CDynaTab(void)
{
	ROW_LIST::iterator	endit = m_rows.end();
	for (ROW_LIST::iterator it = m_rows.begin(); it != endit; ++it)
	{
		if (*it) (*it)->Release();
	}
}

void CDynaTab::AddColumn(LPCTSTR name)
{
	COLUMN_INFO_BASE * col_info = new COLUMN_INFO_BASE(
		m_col_info.GetSize(), VT_UNKNOW, 0, name);
	m_col_info.AddItem(col_info);
}

bool CDynaTab::CreateRow(CDynaRow * & row)
{
	JCASSERT(NULL == row);
	row = new CDynaRow(&m_col_info, m_rows.size());
	return true;
}

void CDynaTab::PushBack(IValue * row)
{
	CDynaRow * _row = dynamic_cast<CDynaRow*>(row);
	if (NULL == _row)	THROW_ERROR(ERR_PARAMETER, _T("row type does not matche table type") );
	m_rows.push_back(_row);
	row->AddRef();
}


void CDynaTab::GetRow(JCSIZE index, IValue * & row)
{
	JCASSERT(NULL == row);
	if (index < m_rows.size() ) row = static_cast<IValue*>(m_rows[index]);
	if (row) row->AddRef();
}

void CDynaTab::Append(IValue * source)
{
	CDynaTab * tab = dynamic_cast<CDynaTab*>(source);
	if (NULL == tab)	THROW_ERROR(ERR_USER, _T("Not a table") );
	if (tab->GetColumnSize() != m_col_info.GetSize() )	
		THROW_ERROR(ERR_USER, _T("Column size not match") );
	JCSIZE row_size = tab->GetRowSize();

	for (JCSIZE ii = 0; ii < row_size; ++ ii)
	{
		IValue * _row = NULL;
		tab->GetRow(ii, _row);
		if (_row)
		{
			CDynaRow * row = dynamic_cast<CDynaRow *>(_row);
			if (row)	m_rows.push_back(row);
			else		_row->Release();
		}
	}
}

void CDynaTab::Format(FILE * file, LPCTSTR format)
{
	LOG_STACK_TRACE();
	// output head
	JCSIZE col_size = m_col_info.GetSize();
	for (JCSIZE ii = 0; ii < col_size; ++ii)
	{
		const COLUMN_INFO_BASE * col_info = m_col_info.GetItem(ii);
		JCASSERT(col_info);
		stdext::jc_fprintf(file, _T("%s, "), col_info->m_name.c_str());
	}
	stdext::jc_fprintf(file, _T("\n"));

	// output data
	ROW_LIST::iterator	endit = m_rows.end();
	for (ROW_LIST::iterator it = m_rows.begin(); it != endit; ++it)
	{
		JCASSERT(*it);
		(*it)->Format(file, format);
		stdext::jc_fprintf(file, _T("\n"));
	}
}

bool CDynaTab::QueryInterface(const char * if_name, IJCInterface * &if_ptr)
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


///////////////////////////////////////////////////////////////////////////////
//--
CDynaRow::CDynaRow(CColumnInfoTable * col_info, JCSIZE id)
	: m_col_info(col_info), m_cols(NULL), m_id(id)
{
	JCASSERT(m_col_info);
	m_col_size = m_col_info->GetSize();
	m_cols = new PIVALUE[m_col_size];
	memset(m_cols, 0, sizeof(PIVALUE) * m_col_size);
}

CDynaRow::~CDynaRow()
{
	for (JCSIZE ii = 0; ii<m_col_size; ++ii) if (m_cols[ii]) m_cols[ii]->Release();
	delete [] m_cols;
}

void CDynaRow::SetColumnVal(JCSIZE col_id, IValue * val)
{
	JCASSERT(col_id < m_col_size);
	if (m_cols[col_id])		m_cols[col_id]->Release();
	m_cols[col_id] = val;
	if (val) val->AddRef();
}

void CDynaRow::SetSubValue(LPCTSTR name, IValue * val)
{
	JCASSERT(m_col_info);
	const COLUMN_INFO_BASE * col_info = m_col_info->GetItem(name);
	if ( (!col_info) || (col_info->m_id >= m_col_size) )	
		THROW_ERROR(ERR_PARAMETER, _T("Unknow column name %s"), name);
	JCSIZE col_id = col_info->m_id;
	if (m_cols[col_id])		m_cols[col_id]->Release();
	m_cols[col_id] = val;
	if (val) val->AddRef();
}

void CDynaRow::GetSubValue(LPCTSTR name, IValue * & val)
{
	JCASSERT(m_col_info);
	JCASSERT(NULL == val);
	const COLUMN_INFO_BASE * col_info = m_col_info->GetItem(name);
	if ( (!col_info) || (col_info->m_id >= m_col_size) )	
		THROW_ERROR(ERR_PARAMETER, _T("Unknow column name %s"), name);
	val = m_cols[col_info->m_id];
	if (val) val->AddRef();
}

int CDynaRow::GetColumnSize() const
{
	JCASSERT(m_col_info);
	return m_col_info->GetSize();
}

void CDynaRow::GetColumnData(int field, IValue * &val)	const
{
	JCASSERT(NULL == val);
	JCASSERT((JCSIZE)field < m_col_size);
	val = m_cols[field];
	if (val) val->AddRef();
}

const COLUMN_INFO_BASE * CDynaRow::GetColumnInfo(int field) const
{
	JCASSERT((JCSIZE)field < m_col_size);
	return m_col_info->GetItem(field);
}

LPCTSTR CDynaRow::GetColumnName(int field_id) const
{
	JCASSERT(m_col_info);
	const COLUMN_INFO_BASE * col_info = m_col_info->GetItem(field_id);
	return (col_info)?(col_info->m_name.c_str()):NULL;
}

void CDynaRow::Format(FILE * file, LPCTSTR format)
{
	// output head
	for (JCSIZE ii = 0; ii < m_col_size; ++ii)
	{
		IValue * val = m_cols[ii];
		stdext::auto_cif<IValueFormat> fmt;
		val->QueryInterface(IF_NAME_VALUE_FORMAT, fmt);
		if ( fmt.valid() )	fmt->Format(file, format);
		stdext::jc_fprintf(file, _T(","));
	}
}

bool CDynaRow::CreateTable(ITable * & tab)
{
	JCASSERT(NULL == tab);
	CDynaTab * _tab = NULL;
	CDynaTab::Create(_tab);
	tab = static_cast<ITable*>(_tab);
	return true;
}
