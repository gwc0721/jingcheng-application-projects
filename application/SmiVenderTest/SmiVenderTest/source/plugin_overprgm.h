#pragma once

#include <SmiDevice.h>
#include <script_engine.h>
#include "plugin_base.h"

class CPluginManager;

class CPluginOverPrgm :
	virtual public jcscript::IPlugin, public CPluginBase, public CJCInterfaceBase
{
protected:
	CPluginOverPrgm(void);
	virtual ~CPluginOverPrgm(void);
public:
	static bool Regist(CPluginManager * manager);
	static jcscript::IPlugin * Create(jcparam::IValue * param);
public:
	virtual bool Reset(void) {return true;}; 
	virtual LPCTSTR name() const {return _T("overprgm");}
	virtual const CCmdManager * GetCommandManager() const { return & m_cmd_man; };

protected:
	bool Enable(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool SetRange(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool SetBlockSize(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool CleanCache(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

protected:
	UINT	m_sec_per_block;
	static const CCmdManager		m_cmd_man;
	ISmiDevice	* m_smi_dev;
};

