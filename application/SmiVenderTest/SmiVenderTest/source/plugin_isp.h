#pragma once

#include <SmiDevice.h>
#include <script_engine.h>
#include "plugin_base.h"

class CPluginManager;

class CPluginIsp :
	virtual public jcscript::IPlugin, public CPluginBase, public CJCInterfaceBase
{
protected:
	CPluginIsp(void);
	virtual ~CPluginIsp(void);
public:
	static bool Regist(CPluginManager * manager);
	static jcscript::IPlugin * Create(jcparam::IValue * param);
public:
	virtual bool Reset(void) {return true;}; 
	virtual LPCTSTR name() const {return _T("isp");}
	virtual const CCmdManager * GetCommandManager() const { return & m_cmd_man; };

protected:
	bool OnlineDownloadIsp(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool OnlineReadIsp(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool ResetCpu(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

protected:
	static const CCmdManager		m_cmd_man;
	ISmiDevice	* m_smi_dev;
};

