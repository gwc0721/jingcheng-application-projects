#pragma once

#include <jcparam.h>
#include "plugin_base.h"

class CPluginManager;

class CSvtApplication;


class CDefaultPlugin :
	virtual public jcscript::IPlugin, public CPluginBase, public CJCInterfaceBase
{
protected:
	CDefaultPlugin();
	~CDefaultPlugin();

public:
	static CDefaultPlugin * Create(jcparam::IValue * param)
	{
		return new CDefaultPlugin();
	}
	
	void SetApp(CSvtApplication * app) { m_app = app; }
	virtual bool Reset(void) {return false;}; 




// Command processes
public:
	bool ScanDevice(jcparam::CArguSet & argu, jcparam::IValue *, jcparam::IValue * &);

protected:
	bool Exit(jcparam::CArguSet &, jcparam::IValue *, jcparam::IValue * &);

protected:
	virtual const CCmdManager * GetCommandManager() const { return & m_cmd_man; };
	static const CCmdManager		m_cmd_man;

	CSvtApplication *	m_app;
};