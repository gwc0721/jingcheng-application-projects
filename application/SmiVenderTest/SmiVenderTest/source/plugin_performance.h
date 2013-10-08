#pragma once


#include <SmiDevice.h>
#include <script_engine.h>
#include "plugin_base.h"
#include "smi_plugin_base.h"

class CPluginManager;


class CPluginPerformance :
	virtual public jcscript::IPlugin, public CPluginBase, public CJCInterfaceBase, public CSmiPluginBase
{
protected:
	CPluginPerformance(void);
	virtual ~CPluginPerformance(void);
public:
	static bool Regist(CPluginManager * manager);
	static jcscript::IPlugin * Create(jcparam::IValue * param);
public:
	virtual bool Reset(void) {return true;}; 
	virtual LPCTSTR name() const {return PLUGIN_NAME;}

// Command handlings
protected:
	bool RandomWrite(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool MakeTestList(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

protected:
	float WriteTime(IStorageDevice * storage, FILESIZE start_lba, JCSIZE sectors, BYTE * buf);

protected:
	static LPCTSTR PLUGIN_NAME;

protected:
	virtual const CCmdManager * GetCommandManager() const { return & m_cmd_man; };
	static const CCmdManager		m_cmd_man;
	float	m_freq;		// system frequency for time test;
};


