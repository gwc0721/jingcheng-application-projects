#include "stdafx.h"
#include "../include/value.h"
#include "../include/table_base.h"
#include "../include/jcstream.h"

using namespace jcparam;

// ColumnInfo --

// CColumn --

CColumn::CColumn(ITable * tab, const COLUMN_INFO_BASE * col_info)
	: m_parent_tab(tab)
	, m_col_info(col_info)
{
	JCASSERT(m_parent_tab);
	JCASSERT(m_col_info);
	m_parent_tab->AddRef();
}

CColumn::~CColumn(void)
{
	m_parent_tab->Release();
	m_parent_tab = NULL;
}

JCSIZE CColumn::GetRowSize() const
{
	JCASSERT(m_parent_tab);
	return m_parent_tab->GetRowSize();
}

void CColumn::GetRow(JCSIZE index, IValue * & val)
{
	JCASSERT(NULL == val);
	stdext::auto_interface<IValue> row;
	m_parent_tab->GetRow(index, row);
	ITableRow * _row = dynamic_cast<ITableRow*>((IValue*)row);
	JCASSERT(_row);
	_row->GetColumnData(m_col_info->m_id, val);
}

///////////////////////////////////////////////////////////////////////////////
// -- 
void COLUMN_INFO_BASE::GetText(void *row, CJCStringT &str) const
{
	IJCStream * stream = NULL;
	CreateStreamString(&str, stream);
	ToStream(row, stream, jcparam::VF_TEXT);
	stream->Release();
}