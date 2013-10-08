#pragma once

#include <SmiDevice.h>
#include <script_engine.h>
#include "plugin_base.h"
#include "smi_plugin_base.h"

class CPluginManager;

class CPluginDevice :
	virtual public jcscript::IPlugin, public CPluginBase, public CJCInterfaceBase, public CSmiPluginBase
{
protected:
	CPluginDevice(void);
	virtual ~CPluginDevice(void);

public:
	static bool Regist(CPluginManager * manager);
	static jcscript::IPlugin * Create(jcparam::IValue * param);

public:
	virtual bool Reset(void) {return true;}; 
	virtual LPCTSTR name() const {return PLUGIN_NAME;}

protected:
	bool ShowCardMode(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool ForceCardMode(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

	bool CleanCacheBlock(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool CpuReset(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool ReadFlash(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool ReadSpareData(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool WriteFlash(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool EraseFlash(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

	bool ReadSectors(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool WriteSectors(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool ReadControllerData(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	//--
	bool SmartHistory(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool VenderCommand(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool BadBlock(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool DownloadMPISP(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);



	bool DumpISP(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool ModifyCID(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool CheckCID(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool LinkInfo(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

	//bool Layout(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

	bool SetMaxAddress(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	//bool IdentifyDevice(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

	bool GetDeviceInfo(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	bool GetSpare(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

	//bool ErrorBit(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);

	//bool OnlineDownloadIsp(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);
	//bool OnlineReadIsp(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout);


protected:
	void ClearCID();
	bool RetrieveBuffer(jcparam::CArguSet & argu, jcparam::IValue * varin, CBinaryBuffer * & buf);

protected:
	static LPCTSTR PLUGIN_NAME;
	ICidTab * m_cid;

protected:
	virtual const CCmdManager * GetCommandManager() const { return & m_cmd_man; };
	static const CCmdManager		m_cmd_man;
};
