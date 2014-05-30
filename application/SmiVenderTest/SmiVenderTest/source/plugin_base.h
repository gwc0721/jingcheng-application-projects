#pragma once
#include <jcparam.h>
#include <script_engine.h>
//#include "cmd_def.h"


class CPluginManager;

class CPluginBase : virtual public jcscript::IPlugin
{
public:
	virtual void GetFeature(const CJCStringT & cmd_name, jcscript::IFeature * & pr);
	virtual void ShowFunctionList(FILE * output) const;

protected:
	virtual const CCmdDefBase * GetCmdDefine(const CJCStringT & cmd_name) const;
	virtual const CCmdManager * GetCommandManager() const
	{
		return NULL;
	}
};


