#include "stdafx.h"
//#include "plugin_manager.h"
//#include "application.h"
//#include "category_comm.h"
#include "../include/feature_base.h"

LOCAL_LOGGER_ENABLE(_T("category_comm"), LOGGER_LEVEL_WARNING);

CCategoryComm::CCategoryComm(LPCTSTR name)
	: m_name(name)
{
}

CCategoryComm::~CCategoryComm(void)
{
}

void CCategoryComm::GetFeature(const CJCStringT & feature_name, jcscript::IFeature * & ptr)
{
	JCASSERT(NULL == ptr);
	const CFeatureInfo * info = m_feature_list.GetItem(feature_name);
	if (NULL == info) return;
	info->m_creator(static_cast<jcscript::IPlugin *>(this), ptr);
}

void CCategoryComm::ShowFunctionList(FILE * output) const
{
}

void CCategoryComm::RegisterFeature(LPCTSTR feature_name, FEATURE_CREATOR creator, LPCTSTR desc)
{
	CFeatureInfo * info = new CFeatureInfo(feature_name, creator, desc);
	m_feature_list.AddItem(info);
}
