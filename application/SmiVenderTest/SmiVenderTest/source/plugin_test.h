#pragma once

#include <jcparam.h>
#include <SmiDevice.h>
#include <script_engine.h>

#include "plugin_base.h"
#include "smi_plugin_base.h"

class CPluginTest :
	public CPluginBase,
	public CJCInterfaceBase,
	public CSmiPluginBase
{
protected:
	CPluginTest(jcparam::IValue * param);
	virtual ~CPluginTest(void);

public:
	static bool Regist(CPluginManager * manager);
	static jcscript::IPlugin * Create(jcparam::IValue * param);

// For jcscript::IPlugin
public:
	virtual bool Reset(void) {return true;}; 
	virtual LPCTSTR name() const {return PLUGIN_NAME;}

// Commands
protected:
	bool MakePattern(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool RandomPattern(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool VerifyRandomPattern(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool Verify(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool FindData(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);


	//bool CommandDefineTest(jcparam::CArguSet & argu, jcparam::IValue * in, jcparam::IValue * & out);
	bool Endurance(jcparam::CArguSet & argu, jcparam::IValue * in, jcparam::IValue * & out);
	bool SequentialWrite(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool RandomWrite(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool CleanTime(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

	bool CopyCompare(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool SectorWrite(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	//bool ReadTest(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);


protected:
	//bool RandomWriteTest(ISmiDevice * smi_dev, UINT block_size, FILESIZE blocks, 
	//				 FILESIZE range, CTimeLogArray * time_log_arr);
	bool RandomCopyCompareTest(ISmiDevice * smi_dev, UINT block_size, FILESIZE blocks, CJCStringT & pattern, UINT loop);

	// Calculate performace from time_log
	bool Performance(UINT resolution, jcparam::IValue * var_in, jcparam::IValue * & var_out);
	bool VerifyData(ISmiDevice * smi_dev/*, IValue * & val*/);

	bool FileRepair(ISmiDevice * smi_dev);
	void GenerateRandomList(FILESIZE blocks, FILESIZE alignment, FILESIZE* list);

	// measure write time for one "block".
	

// Test functions
protected:

	template <typename VAL_TYPE, int BYTES>
	VAL_TYPE GetRawValue(BYTE * buf)
	{
		VAL_TYPE val = 0;
		for ( JCSIZE jj=0; jj<BYTES; ++jj)	val |= (VAL_TYPE)(buf[jj]) << (jj * 8);
		return val;
	}

protected:
	virtual const CCmdManager * GetCommandManager() const { return & m_cmd_man; };
	static const CCmdManager		m_cmd_man;

	//ISmiDevice	* m_smi_dev;


protected:
	static const TCHAR PNAME_BLOCK_SIZE[];
	static const TCHAR PNAME_BLOCKS[];
	static const TCHAR PNAME_RANGE[];
	static const TCHAR PNAME_RESOLUTION[];
	static const TCHAR PNAME_PATTERN[];
	static const TCHAR PNAME_LOOP[];

	//static const char * PLUGIN_NAME;
	static const TCHAR PLUGIN_NAME[];
	static const TCHAR VTS_TIMELOG[];

};
