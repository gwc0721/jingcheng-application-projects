#include "MySQLRow.h"

using namespace jcdb;

CMySQLRow::CMySQLRow(CDBFieldMap * field, MYSQL_ROW row, unsigned long * lengths)
	: m_row(row)
	, m_field_map(field)
	, m_lengths(lengths)
	, m_id(-1)
{
	JCASSERT(m_row);
	JCASSERT(m_field_map);
	JCASSERT(m_lengths);

	// set id
	JCASSERT(m_row[0]);
	m_id = atoi(m_row[0]);
}

CMySQLRow::~CMySQLRow()
{
}


void CMySQLRow::GetFieldData(int field_id, CJCStringT & data)	const
{
	JCASSERT(field_id >= 0);
	JCASSERT(field_id < GetFieldSize());
	int len = (int)strlen(m_row[field_id]);
	data.reserve(len+1);
	TCHAR * _str = (LPTSTR) data.data();
	_stprintf_s(_str, len, _T("%S"), strlen(m_row[field_id]));
	//return m_row[field_id];
}
		
bool CMySQLRow::GetFieldData(const char * field_name, CJCStringT & data) const
{
	int field_id = m_field_map->GetFieldID(field_name);
	if (field_id >= 0) return false;
	GetFieldData(field_id, data);
	return true;
}


const CJCStringA & CMySQLRow::GetFieldName(int field_id) const
{
	JCASSERT(field_id < GetFieldSize());
	const CDBFieldInfo * field_info = m_field_map->GetFieldInfo(field_id);
	JCASSERT(field_info);
	return field_info->GetFieldName();
}
