#pragma once

#include <jcparam.h>
#include <SmiDevice.h>

#include "plugin_base.h"
#include <script_engine.h>

class CPluginAtaDevice :
	public CPluginBase,
	public CJCInterfaceBase
{
protected:
	CPluginAtaDevice(jcparam::IValue * param);
	virtual ~CPluginAtaDevice(void);

public:
	static bool Regist(CPluginManager * manager);
	static jcscript::IPlugin * Create(jcparam::IValue * param);
	virtual bool Reset(void) {return true;}; 
	virtual LPCTSTR name() const {return _T("ata");}

protected:
	bool IdentifyDevice(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool SetFeature(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool ReadSectors(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool WriteSectors(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool ReadDMA(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool WriteDMA(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool AtaCommand(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool Smart(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

protected:
	virtual const CCmdManager * GetCommandManager() const { return & m_cmd_man; };
	static const CCmdManager		m_cmd_man;
	ISmiDevice	* m_smi_dev;
	//IAtaDevice	* m_
};

