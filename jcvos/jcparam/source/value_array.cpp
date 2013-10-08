#include "../include/value.h"

using namespace  jcparam;

CValueArray::~CValueArray(void) 
{
	VALUE_ITERATOR it, end_it = m_value_vector.end();
	for (it = m_value_vector.begin(); it != end_it; ++it)
	{
		IValue * value = *it;
		JCASSERT(value);
		value->Release();
	}
}

bool CValueArray::GetValueAt(int index, IValue * &value)
{
	value = m_value_vector.at(index);
	JCASSERT(value);
	value ->AddRef();
	return true;
}

bool CValueArray::PushBack(IValue * value)
{
	JCASSERT(value);
	value->AddRef();
	m_value_vector.push_back(value);
	return true;
}


