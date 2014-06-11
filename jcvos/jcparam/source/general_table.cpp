#include "stdafx.h"

#include "../include/general_table.h"

LOCAL_LOGGER_ENABLE(_T("jcparam.gtable"), LOGGER_LEVEL_WARNING);

using namespace jcparam;
///////////////////////////////////////////////////////////////////////////////
// -- CColInfoList
void CreateColumnInfoList(IColInfoList * & list)
{
	JCASSERT(NULL == list);
	list = new CColInfoList;
}

void CColInfoList::AddInfo(const COLUMN_INFO_BASE* info)
{
	LIST_BASE::AddItem(info);
}

const COLUMN_INFO_BASE * CColInfoList::GetInfo(const CJCStringT & key) const
{
	return LIST_BASE::GetItem(key);
}

const COLUMN_INFO_BASE * CColInfoList::GetInfo(JCSIZE index) const
{
	return LIST_BASE::GetItem(index);
}

void CColInfoList::OutputHead(IJCStream * stream) const
{
	CONST_ITERATOR it = m_item_map->begin();
	CONST_ITERATOR endit = m_item_map->end();

	for ( ; it!=endit; ++it)
	{
		const CJCStringT & name = it->first;
		stream->Put(name.c_str(), name.length() );
		stream->Put(_T(','));
	}
}


///////////////////////////////////////////////////////////////////////////////
// -- CGeneralRow

void CGeneralRow::CreateGeneralRow(IColInfoList * info, CGeneralRow * &row)
{
	JCASSERT(NULL == row);
	row = new CGeneralRow(info);
}

CGeneralRow::CGeneralRow(IColInfoList * info)
	: m_col_info(info)
	, m_fields(NULL)
	, m_data(NULL)
{
	JCASSERT(m_col_info);
	m_col_info->AddRef();
	m_col_num = m_col_info->GetColNum();

	m_fields = new FIELDS[m_col_num];
	JCASSERT(m_fields);
	memset(m_fields, 0, sizeof(FIELDS) * m_col_num);
}

CGeneralRow::~CGeneralRow(void)
{
	if (m_col_info) m_col_info->Release();
	delete [] m_fields;
	delete [] m_data;
}

void CGeneralRow::GetColumnData(const COLUMN_INFO_BASE *info, IValue * &val)	const
{
	JCASSERT(NULL == val);
	CJCStringT tmp(m_data + m_fields[info->m_id].offset, m_fields[info->m_id].len);
	val = static_cast<IValue*>(CTypedValue<CJCStringT>::Create(tmp) );
}


void CGeneralRow::GetSubValue(LPCTSTR name, IValue * & val)
{
	const COLUMN_INFO_BASE * info = m_col_info->GetInfo(name);
	if (!info) return;
	GetColumnData(info, val);
}

void CGeneralRow::SetSubValue(LPCTSTR name, IValue * val)
{	// update data
}

void CGeneralRow::GetValueText(CJCStringT & str) const
{
	str = CJCStringT(m_data);
}

void CGeneralRow::SetValueText(LPCTSTR str)
{
	// <TODO> clear m_data
	m_data_len = (JCSIZE)_tcslen(str);
	m_data = new TCHAR[m_data_len +1];
	_tcscpy_s(m_data, m_data_len +1, str);
	// <TODO> calculate fields
}

int CGeneralRow::GetColumnSize() const
{
	return m_col_info->GetColNum();
}

const COLUMN_INFO_BASE * CGeneralRow::GetColumnInfo(LPCTSTR field_name) const
{
	return m_col_info->GetInfo(field_name);
}

const COLUMN_INFO_BASE * CGeneralRow::GetColumnInfo(int field) const
{
	return m_col_info->GetInfo(field);
}
		
// 从一个通用的行中取得通用的列数据
void CGeneralRow::GetColumnData(int field, IValue * & val)	const
{
	if ((JCSIZE)field >= m_col_num) return;
	const COLUMN_INFO_BASE * info = m_col_info->GetInfo(field);
	GetColumnData(info, val);
}

//JCSIZE CGeneralRow::GetRowID(void) const
//{
//	return 0;
//}
		// 从row的类型创建一个表格
bool CGeneralRow::CreateTable(ITable * & tab)
{
	jcparam::CreateGeneralTable(m_col_info, tab);
	return true;
}

void CGeneralRow::ToStream(jcparam::IJCStream * str, VAL_FORMAT) const
{
	str->Put(m_data, m_data_len);
}

void CGeneralRow::FromStream(jcparam::IJCStream * str, VAL_FORMAT)
{
}

///////////////////////////////////////////////////////////////////////////////
// -- CGeneralColumn
/*
CGeneralColumn::CGeneralColumn(CGeneralTable * table, const COLUMN_INFO_BASE *info)
	: m_table(table), m_col_info(info)
{
}

CGeneralColumn::~CGeneralColumn(void)
{
}

void CGeneralColumn::GetRow(JCSIZE index, IValue * & val)
{
	JCASSERT(m_table);
	stdext::auto_cif<ITableRow, IValue> row;
	m_table->GetRow(index, row);
	row->GetColumnData(m_col_info->m_id, val);
}

JCSIZE CGeneralColumn::GetRowSize() const
{
	JCASSERT(m_table);
	return m_table->GetRowSize();
}
*/

///////////////////////////////////////////////////////////////////////////////
// -- CGeneralTable
void jcparam::CreateGeneralTable(IColInfoList * info, ITable * & table)
{
	JCASSERT(NULL == table);
	table = static_cast<ITable*>(new CGeneralTable(info));
}

CGeneralTable::CGeneralTable(IColInfoList * info)
	: m_col_info(info)
{
	JCASSERT(m_col_info);
	m_col_info->AddRef();
}

CGeneralTable::~CGeneralTable(void)
{
	if (m_col_info) m_col_info->Release();
	ROWS::iterator it=m_rows.begin();
	ROWS::iterator endit = m_rows.end();
	for (; it != endit; ++it)
	{
		JCASSERT(*it);
		(*it)->Release();
	}
}

void CGeneralTable::GetSubValue(LPCTSTR name, IValue * & val)
{
	JCASSERT(NULL == val);
	JCASSERT(m_col_info);

	const COLUMN_INFO_BASE * info = m_col_info->GetInfo(name);
	if (!info) return;

	val = static_cast<IValue*>(new CColumn(static_cast<ITable*>(this), info));
}

void CGeneralTable::PushBack(IValue * val)
{
	CGeneralRow * row = dynamic_cast<CGeneralRow*>(val);
	if (!row) THROW_ERROR(ERR_PARAMETER, _T("general_table:push_back row type do not match"));
	m_rows.push_back(row);
	row->AddRef();
}

void CGeneralTable::GetRow(JCSIZE index, IValue * & val)
{
	JCASSERT(index < m_rows.size() );
	val = dynamic_cast<IValue*>( m_rows.at(index) );
}

JCSIZE CGeneralTable::GetRowSize() const
{
	return (JCSIZE) m_rows.size();
}

// ITable
JCSIZE CGeneralTable::GetColumnSize() const
{
	JCASSERT(m_col_info);
	return m_col_info->GetColNum();
}

void CGeneralTable::ToStream(IJCStream * stream, VAL_FORMAT fmt) const
{
	JCASSERT(m_col_info);
	m_col_info->OutputHead(stream);
	ROWS::const_iterator it = m_rows.begin();
	ROWS::const_iterator endit = m_rows.end();

	for ( ; it != endit; ++it)
	{
		(*it)->ToStream(stream, fmt);
	}

}

void CGeneralTable::FromStream(IJCStream * str, VAL_FORMAT)
{
}


