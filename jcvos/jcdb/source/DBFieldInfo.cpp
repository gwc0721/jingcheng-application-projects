//#include "StdAfx.h"
#include "../include/DBFieldInfo.h"

using namespace jcdb;

CDBFieldInfo::CDBFieldInfo(LPTSTR field_name, int field_width, int field_id)
	: m_field_name(field_name)
	, m_field_width(field_width)
	, m_field_id(field_id)
	//, m_ref(1)
	//, m_offset(-1)
{
}

//CDBFieldInfo::CDBFieldInfo(LPCTSTR field_name, size_t offset)
//	: m_field_name(field_name)
//	, m_iRef(1)
//	, m_offset(offset)
//{
//}

CDBFieldInfo::~CDBFieldInfo(void)
{
}
