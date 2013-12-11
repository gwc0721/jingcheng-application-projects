#include "stdafx.h"
#include "../include/value.h"

using namespace jcparam;
LOCAL_LOGGER_ENABLE(_T("parameter"), LOGGER_LEVEL_DEBUGINFO);


///////////////////////////////////////////////////////////////////////////////

LOG_CLASS_SIZE( CTypedValueBase )

LOG_CLASS_SIZE_T1(CTypedValue, INT64)
LOG_CLASS_SIZE_T1(CTypedValue, int)
LOG_CLASS_SIZE_T1(CTypedValue, CJCStringT)



///////////////////////////////////////////////////////////////////////////////

jcparam::IValue * jcparam::CreateTypedValue(jcparam::VALUE_TYPE vt, void * data)
{
	IValue * val = NULL;
	switch (vt)
	{
	case jcparam::VT_CHAR:
		val = CTypedValue<char>::Create( (data)?*((const char *)data):(0) );					
		break;
	case jcparam::VT_UCHAR:		val = CTypedValue<unsigned char>::Create();		break;
	case jcparam::VT_SHORT:		val = CTypedValue<short>::Create();				break;
	case jcparam::VT_USHORT:	val = CTypedValue<unsigned short>::Create();	break;
	case jcparam::VT_INT:		val = CTypedValue<int>::Create();				break;
	case jcparam::VT_UINT:		val = CTypedValue<unsigned int>::Create();		break;
	case jcparam::VT_INT64:		val = CTypedValue<INT64>::Create();				break;
	case jcparam::VT_UINT64:	val = CTypedValue<UINT64>::Create();			break;
	case jcparam::VT_FLOAT:		val = CTypedValue<float>::Create();				break;
	case jcparam::VT_DOUBLE:	val = CTypedValue<double>::Create();			break;
	case jcparam::VT_STRING:	val = CTypedValue<CJCStringT>::Create();		break;
	case jcparam::VT_BOOL:		val = CTypedValue<bool>::Create();				break;
	default:
		THROW_ERROR(ERR_PARAMETER, _T("Unknow type %d"), (int)vt);
		break;
	}
	return val;
}

typedef std::pair<LPCTSTR, VALUE_TYPE>	VALUE_TYPE_PAIR;

static const VALUE_TYPE_PAIR value_type_index[] = {
	VALUE_TYPE_PAIR(_T("bool"), jcparam::VT_BOOL),
	VALUE_TYPE_PAIR(_T("char"), jcparam::VT_CHAR),
	VALUE_TYPE_PAIR(_T("uchar"), jcparam::VT_UCHAR),
	VALUE_TYPE_PAIR(_T("short"), jcparam::VT_SHORT),
	VALUE_TYPE_PAIR(_T("ushort"), jcparam::VT_USHORT),
	VALUE_TYPE_PAIR(_T("int"), jcparam::VT_INT),
	VALUE_TYPE_PAIR(_T("uint"), jcparam::VT_UINT),
	VALUE_TYPE_PAIR(_T("int64"), jcparam::VT_INT64),
	VALUE_TYPE_PAIR(_T("uint64"), jcparam::VT_UINT64),
	VALUE_TYPE_PAIR(_T("float"), jcparam::VT_FLOAT),
	VALUE_TYPE_PAIR(_T("double"), jcparam::VT_DOUBLE),
	VALUE_TYPE_PAIR(_T("string"), jcparam::VT_STRING),
};

const JCSIZE value_type_number = sizeof (value_type_index) / sizeof(VALUE_TYPE_PAIR);

VALUE_TYPE jcparam::StringToType(LPCTSTR str)
{
	for (JCSIZE ii = 0; ii < value_type_number; ++ii)
		if ( _tcscmp(str, value_type_index[ii].first) == 0) return value_type_index[ii].second;
	return jcparam::VT_UNKNOW;
}


///////////////////////////////////////////////////////////////////////////////

void jcparam::CTypedValueBase::GetSubValue(LPCTSTR name, IValue * & val)
{
	JCASSERT(val == NULL);
	if (_tcscmp(name, _T("") ) == 0 ) val = static_cast<IValue*>(this);
}

void jcparam::CTypedValueBase::SetSubValue(LPCTSTR name, IValue * val)
{
	THROW_ERROR(ERR_APP, _T("Do not support sub value!") );
}

///////////////////////////////////////////////////////////////////////////////


CParamSet::CParamSet(void)
{
}

CParamSet::~CParamSet(void)
{
	ITERATOR endit = m_param_map.end();
	for (ITERATOR it = m_param_map.begin(); it != endit; ++it)
	{
		if (it->second) it->second->Release();
	}
}

void CParamSet::GetSubValue(LPCTSTR param_name, IValue * & value)
{
	JCASSERT(param_name);
	JCASSERT(NULL == value);

	ITERATOR it = m_param_map.find(param_name);
	if ( it == m_param_map.end() ) return;
	value = it->second;
	JCASSERT(value);
	value->AddRef();
}

void CParamSet::SetSubValue(LPCTSTR name, IValue * val)
{
	JCASSERT(name);
	JCASSERT(val);

	ITERATOR it = m_param_map.find(name);
	if ( it != m_param_map.end() )
	{
		IValue * tmp = it->second;
		it->second = val;
		tmp->Release();
	}
	else
	{
		m_param_map.insert( KVP(name, val) );
	}
	val->AddRef();
}


bool CParamSet::InsertValue(const CJCStringT & param_name, IValue* value)
{
	std::pair<ITERATOR, bool> rs = m_param_map.insert( KVP(param_name, value) );
	if (!rs.second) THROW_ERROR(ERR_APP, _T("Parameter %s has already existed"), param_name.c_str());
	value->AddRef();
	return true;
}

bool CParamSet::RemoveValue(LPCTSTR param_name)
{
	ITERATOR it = m_param_map.find(param_name);
	if ( it != m_param_map.end() )
	{
		IValue * tmp = it->second;
		tmp->Release();
		m_param_map.erase(it);
		return true;
	}
	else	return false;
	//JCSIZE ii = m_param_map.erase(param_name);
	//if (0 == ii)	THROW_ERROR(ERR_PARAMETER, _T(""))
}
