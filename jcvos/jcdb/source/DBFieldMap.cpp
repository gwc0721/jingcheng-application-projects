//#include "StdAfx.h"
#include "../include/field_info.h"
#include <stdio.h>
#include <stdarg.h>


using namespace jcdb;

static const int ARRAY_INCREASE = 5;

CDBFieldMap::CDBFieldMap()
	: m_field_array(NULL)
	, m_array_capacit(0)
	, m_field_size(0)
{
	//m_field_array = new (CDBFieldInfo*)[ARRAY_INCREASE];
	//m_array_capacit = ARRAY_INCREASE;
	//memset(m_field_array, 0, sizeof (CDBFieldInfo*) * ARRAY_INCREASE);
}

CDBFieldMap::CDBFieldMap(int size, ...)
	: m_field_array(NULL)
	, m_array_capacit(size)
	, m_field_size(size)
{
	m_field_array = new CDBFieldInfo* [size];
	memset(m_field_array, 0, size * sizeof(CDBFieldInfo*) );

	va_list fields;
	va_start(fields, size);

	for (int ii = 0; ii < size; ++ii)
	{
		CDBFieldInfo * field_info = va_arg(fields, CDBFieldInfo *);
		if (!field_info) break;

		int id = field_info->GetFieldId();
		JCASSERT( id < m_field_size);

		typedef std::pair<CJCStringA, const CDBFieldInfo*> _FIELD_PAIR;
		m_field_map.insert( _FIELD_PAIR(field_info->GetFieldName(), field_info) );
		m_field_array[id] = field_info;
	}
	va_end(fields);
}

CDBFieldMap::~CDBFieldMap(void)
{
	//int icount = GetSize();
	//for (int i=0; i< icount; i++)
	//{
	//	const CDBFieldInfo * field_info = GetValueAt(i);
	//	if (field_info) delete field_info;
	//}
	//RemoveAll();
	for (int ii=0; ii<m_field_size; ii++)
	{
		delete m_field_array[ii];
		m_field_array[ii] = NULL;
	}
	delete [] m_field_array;
}

//void CDBFieldMap::InsertField(int field_num, const CDBFieldInfo * field)
//{
//	SetAtIndex(field_num, field->GetFieldName(), field);
//}

void CDBFieldMap::InsertField(int id, const char * name, int width)
{
	typedef std::pair<CJCStringA, const CDBFieldInfo*> _FIELD_PAIR;
	CDBFieldInfo * field_info = new CDBFieldInfo(name, width, id);
	m_field_map.insert( _FIELD_PAIR(name, field_info) );

	//
	if (m_array_capacit <= id)
	{
		CDBFieldInfo ** temp = new CDBFieldInfo* [m_array_capacit + ARRAY_INCREASE];
		memset(	(void*)(temp + m_array_capacit), 0, ARRAY_INCREASE * sizeof(CDBFieldInfo*) );
		if (m_field_array)
		{
			memcpy(temp, m_field_array, m_array_capacit * sizeof(CDBFieldInfo*) );
			delete [] m_field_array;
		}
		m_array_capacit += ARRAY_INCREASE;
	}
	m_field_array[id] = field_info;
}

void CDBFieldMap::PushField(int id, const char * name, int width)
{
	JCASSERT(id < m_field_size);
	typedef std::pair<CJCStringA, const CDBFieldInfo*> _FIELD_PAIR;
	CDBFieldInfo * field_info = new CDBFieldInfo(name, width, id);
	m_field_map.insert( _FIELD_PAIR(name, field_info) );

	m_field_array[id] = field_info;
}

void CDBFieldMap::SetFieldSize(int size)
{
	JCASSERT(m_field_array == NULL);
	m_field_size = m_array_capacit = size;
	m_field_array = new CDBFieldInfo* [size];
	memset(m_field_array, 0, size * sizeof(CDBFieldInfo*) );
}

int CDBFieldMap::GetFieldID(const char * name)
{
	FIELD_MAP_ITERATOR it = m_field_map.find(name);
	if (it == m_field_map.end() ) return -1;
	JCASSERT(it->second);
	return it->second->GetFieldId();
}
