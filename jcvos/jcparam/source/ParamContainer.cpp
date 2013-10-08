#include "../include/parameter_impl.h"

using namespace jcparam;

IValue * CParamContainerMap::GetSubValue(LPCTSTR param_name)
{
	LPCTSTR sep = _tcschr(param_name, _T('.'));

	CJCStringT main_name;
	if (sep)	main_name = CJCStringT(param_name, sep - param_name);
	else		main_name = param_name;

	PARAM_ITERATOR it = m_param_map.find(main_name);
	if ( it == m_param_map.end() ) return NULL;

	if (sep)	return it->second->GetSubValue(sep +1);
	else		return it->second;

}

void jcparam::CParamContainerMap::GetValueText(CJCStringT & str) const
{
}

void jcparam::CParamContainerMap::SetValueText(LPCTSTR str)
{
}