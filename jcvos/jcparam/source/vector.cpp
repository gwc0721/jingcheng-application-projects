#include "stdafx.h"
#include "../include/value.h"


LOCAL_LOGGER_ENABLE(_T("jcparam.vector"), LOGGER_LEVEL_DEBUGINFO);
using namespace jcparam;

LOG_CLASS_SIZE( CVector )

CVector::CVector(void)
{
}

CVector::~CVector(void)
{
	VALUE_ITERATOR it = m_vector.begin();
	VALUE_ITERATOR endit = m_vector.end();

	for ( ; it != endit; ++it)
	{
		if (*it) (*it)->Release();
	}
	m_vector.clear();
}

void CVector::PushBack(IValue * val)
{
	m_vector.push_back(val);
	val->AddRef();
}

void CVector::GetRow(JCSIZE index, IValue * & val)
{
	JCASSERT(NULL == val);
	JCASSERT(index < m_vector.size());
	val = m_vector.at(index);
	val->AddRef();
}

JCSIZE CVector::GetRowSize() const
{
	return (JCSIZE)m_vector.size();
}

//void CVector::Format(FILE * file, LPCTSTR format)
//{
//	VALUE_ITERATOR it = m_vector.begin();
//	VALUE_ITERATOR endit = m_vector.end();
//	for ( ; it!=endit; ++it)
//	{
//		IValue *val = (*it);
//		if (val)
//		{
//			CJCStringT str;
//			val->GetValueText(str);
//			fprintf_s(file, "%S", str.c_str() );
//		}
//		fprintf_s(file, "\n");
//	}
//}
//
//void CVector::WriteHeader(FILE * file)
//{
//}
