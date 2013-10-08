#include "StdAfx.h"
//#include "commhead.h"

#include "NetyiAccRow.h"


CLASS_SIZE(CNetyiAccRow);
CLASS_SIZE1(CTypedFieldInfo<int>, __class_size_CTypedFieldInfo_int)
CLASS_SIZE1(CTypedFieldInfo<CAtlString>, __class_size_CTypedFieldInfo_CString)

const CDBFieldMap CNetyiAccRow::m_field_map(
	6,
	new CTypedFieldInfo<int>(_T("id"), offsetof(CNetyiAccRow, m_id)),
	new CTypedFieldInfo<CAtlString>(_T("user"), offsetof(CNetyiAccRow, m_user)),
	new CTypedFieldInfo<CAtlString>(_T("password"), offsetof(CNetyiAccRow, m_password)),
	new CTypedFieldInfo<int>(_T("point"), offsetof(CNetyiAccRow, m_point)),
	new CTypedFieldInfo<int>(_T("today_clicked"), offsetof(CNetyiAccRow, m_today_clicked)),
	new CTypedFieldInfo<int>(_T("clicked"), offsetof(CNetyiAccRow, m_clicked)),
	NULL
									  );

CNetyiAccRow::CNetyiAccRow(CRecordRowImpl * row)
	: CRecordRowImpl(row)
{
	ASSERT(row);
	SetData(row);
}



CNetyiAccRow::~CNetyiAccRow(void)
{
	ASSERT(1);
}


void CNetyiAccRow::SetData(const CRecordRowImpl * row)
{
	ASSERT(row->GetFields() >= FID_MAX);
	m_user = CA2T(row->GetFieldData(FID_USER), CP_UTF8);
	m_password = CA2T(row->GetFieldData(FID_PASSWORD), CP_UTF8);
	m_point = atoi(row->GetFieldData(FID_POINT));
	m_today_clicked = atoi(row->GetFieldData(FID_TODAY_CLICKED));
	m_clicked = atoi(row->GetFieldData(FID_CLICKED));
	m_sort_key = (float)m_today_clicked;
}