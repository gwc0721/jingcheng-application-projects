#pragma once

#include <jcparam.h>
#include <script_engine.h>
//#include "plugin_base.h"
//#include "feature_base.h"

class CPluginDefault :
	virtual public jcscript::IPlugin, public CJCInterfaceBase
{
public:
	CPluginDefault(void) {};
	virtual ~CPluginDefault() {};

public:
	virtual bool Reset(void) {return true;} 
	virtual void GetFeature(const CJCStringT & cmd_name, jcscript::IFeature * & pr);
	virtual void ShowFunctionList(FILE * output) const {}

public:
	virtual LPCTSTR name() const {return _T("");}
};

/*
//-- 命令实现
public:
	bool ScanDevice(jcparam::CArguSet & argu, jcparam::IValue *, jcparam::IValue * &);
	bool Exit(jcparam::CArguSet &, jcparam::IValue *, jcparam::IValue * &);
	bool Assign(jcparam::CArguSet & argu, jcparam::IValue * var_in, jcparam::IValue * &var_out);
	bool SaveVariable(jcparam::CArguSet & argu, jcparam::IValue * var_in, jcparam::IValue * &var_out);
	bool ShowVariable(jcparam::CArguSet & argu, jcparam::IValue * var_in, jcparam::IValue * &var_out);
	bool LoadBinaryData(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool MakeVector(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

*/
