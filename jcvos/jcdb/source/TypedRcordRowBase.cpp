#include "../include/typed_record_row.h"

using namespace jcdb;


CTypedRecordRowBase::CTypedRecordRowBase(/*IResultSet * res, */CDBFieldMap *field_map)
: m_field_map(field_map)
{
	JCASSERT(field_map);
}

CTypedRecordRowBase::~CTypedRecordRowBase(void)
{
}

void CTypedRecordRowBase::GetFieldData(int field_id, CJCStringT & data) const 
{
	JCASSERT(field_id >= 0);
	JCASSERT(field_id < GetFieldSize());

	const CDBFieldInfo * field = m_field_map->GetFieldInfo(field_id);
	field->GetDataText((void*)this, data);
}

bool CTypedRecordRowBase::GetFieldData(const char * field_name, CJCStringT & data) const
{
	int field_id = m_field_map->GetFieldID(field_name);
	if (field_id >= 0) return false;
	GetFieldData(field_id, data);
	return true;
}

const CJCStringA & CTypedRecordRowBase::GetFieldName(int field_id) const
{
	JCASSERT(field_id < GetFieldSize());
	const CDBFieldInfo * field_info = m_field_map->GetFieldInfo(field_id);
	JCASSERT(field_info);
	return field_info->GetFieldName();
}

void CTypedRecordRowBase::SetData(const IRecordRow * row)
{
	JCASSERT(row);
	int field_size = row->GetFieldSize();
	for (int ii=0; ii<field_size; ++ii)
	{
		const CDBFieldInfo * field_info = m_field_map->GetFieldInfo(ii);
		JCASSERT(field_info);

		CJCStringT data;
		bool br = row->GetFieldData(field_info->GetFieldName().c_str(), data);
		if (br) 
		{
			field_info->SetDataText((void*)this, data.c_str() );
		}
	}
}
