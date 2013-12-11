#pragma once

#include "plugin_manager.h"
#include <SmiDevice.h>
#include <jcparam.h>

#include "cmd_def.h"
#include "plugin_base.h"

class CSvtApplication;

///////////////////////////////////////////////////////////////////////////////
//--CSvtApplication
class CSvtApplication : virtual public jcscript::LSyntaxErrorHandler
{
//-- Application 实现
public:
	CSvtApplication(void);
	virtual ~CSvtApplication(void);

	virtual int Initialize(LPCTSTR cmd);
	virtual int Cleanup(void);
	virtual int Run(void);
	virtual bool ProcessCommandLine(LPCTSTR cmd);
	static CSvtApplication* GetApp(void)  { return m_app; }

protected:
	// 命令行解释
	static const jcparam::CParameterDefinition m_cmd_line_parser;	// parser for process command line
	static CSvtApplication * m_app;

//-- Plugin 管理
public:
	CPluginManager * GetPluginManager(void) { return m_plugin_manager; }

protected:
	CPluginManager	* m_plugin_manager;

//-- Device Manager
public:
	void GetDevice(ISmiDevice * & dev);
	// Default variable: 和系统或者device有关的一些变量/常量，如:max lba, pages per block等
	bool GetDefaultVariable(const CJCStringT & name, jcparam::IValue * & var);
	void SetDevice(const CJCStringT & name, ISmiDevice * dev);

protected:
	ISmiDevice * m_dev;
	CJCStringT m_device_name;


//--命令解析与执行
protected:
	// 解析并执行一条命令
	bool ParseCommand(LPCTSTR first, LPCTSTR last);
	bool RunScript(LPCTSTR file_name);
	void ShowHelpMessage() const;
	virtual void OnError(JCSIZE line, JCSIZE column, LPCTSTR msg);
	void GetVersionInfo(void);

	UINT m_main_ver, m_sub_ver;


public:
	void ReportProgress(LPCTSTR msg, int percent);

protected:
	bool ScanDevice(jcparam::CArguSet & argu, jcparam::IValue *, jcparam::IValue * &);

//-- jcscript::IPlugin实现
public:
	virtual bool Reset(void) {return false;}; 
	virtual LPCTSTR name() const {return _T("default");}

protected:
	jcscript::IAtomOperate	* m_cur_script;


protected:
	bool OutputVariable(FILE * file, jcparam::IValue * var_val, LPCTSTR type);


//-- 命令行
protected:
	// 命令行参数
	jcparam::CArguSet		m_arg_set;		// argument for process command line
	
	// 输出编译的中间结果
	CJCStringT	m_compile_log_fn;
	bool		m_compile_only;
	void OutCompileLog(jcscript::IAtomOperate * script);

	static const TCHAR INDENTATION[];


	// for command tokenize and invoke
protected:
	static const TCHAR CMD_KEY_EXIT[];
	static const TCHAR CMD_KEY_SCANDEVICE[];
	static const TCHAR CMD_KEY_ASSIGN[];
	static const TCHAR CMD_KEY_SAVE[];
	static const TCHAR CMD_KEY_DUMMY_DEVICE[];
};

