#include "stdafx.h"
#include <stdext.h>
#include "plugin_base.h"
#include "cmd_def.h"

LOCAL_LOGGER_ENABLE(_T("plugin"), LOGGER_LEVEL_DEBUGINFO);

void CPluginBase::GetFeature(const CJCStringT & cmd_name, jcscript::IFeature * & pr)
{
	JCASSERT(NULL == pr);
	const CCmdDefBase *	cmd_def = GetCmdDefine(cmd_name);
	if (cmd_def) cmd_def->CreateProxy(static_cast<jcscript::IPlugin*>(this), pr);
}

const CCmdDefBase * CPluginBase::GetCmdDefine(const CJCStringT & cmd_name) const
{
	const CCmdManager* cmd_manager = GetCommandManager();
	const CCmdDefBase * cmd_def = cmd_manager->GetItem(cmd_name);
	return cmd_def;
}

void CPluginBase::ShowFunctionList(FILE * output) const
{
	const CCmdManager* cmd_manager = GetCommandManager();
	CCmdManager::CONST_ITERATOR it = cmd_manager->Begin();
	CCmdManager::CONST_ITERATOR endit = cmd_manager->End();
	for ( ; it != endit; ++it)
	{
		if ( _T('#') != it->first[0] )
		{
			const CCmdDefBase * cmd_def = it->second;
			LPCTSTR desc = cmd_def->Description();
			if (!desc) desc = _T("");
			_ftprintf(output, _T("\t%s : %s\n"), it->first.c_str(), desc);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// -- 