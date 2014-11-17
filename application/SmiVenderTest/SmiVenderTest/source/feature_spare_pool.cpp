#include "stdafx.h"

#include "feature_spare_pool.h"
#include "application.h"

LPCTSTR CFeatureSparePool::_BASE_TYPE::m_feature_name = _T("sparepool");

CParamDefTab CFeatureSparePool::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CFeatureSparePool, m_file_name) ) )
	);

//const UINT CFeatureSparePool::_BASE_TYPE::m_property = jcscript::OPP_LOOP_SOURCE;

const TCHAR CFeatureSparePool::m_desc[] = _T("retriev read count");

CFeatureSparePool::CFeatureSparePool(void)
	: m_smi_dev(NULL)
{
}

CFeatureSparePool::~CFeatureSparePool(void)
{
}

bool CFeatureSparePool::Invoke(jcparam::IValue * , jcscript::IOutPort * outport)
{
	return false;
}

bool CFeatureSparePool::Init(void)
{
	CSvtApplication::GetApp()->GetDevice(m_smi_dev);
	JCASSERT(m_smi_dev);
	return true;
}
