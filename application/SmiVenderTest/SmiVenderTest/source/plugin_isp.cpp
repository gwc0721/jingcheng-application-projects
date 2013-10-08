#include "stdafx.h"
#include <stdext.h>
#include "plugin_manager.h"
#include "plugin_isp.h"
#include "application.h"

LOCAL_LOGGER_ENABLE(_T("isp_pi"), LOGGER_LEVEL_ERROR);

#define __CLASS_NAME__	CPluginIsp
BEGIN_COMMAND_DEFINATION
	COMMAND_DEFINE(_T("download"),			OnlineDownloadIsp, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		,	_T("Download input ISP data to device. Using online update isp feature")
	)

	COMMAND_DEFINE(_T("read"),			OnlineReadIsp, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		,	_T("Read ISP data from device. Using online update isp feature"), 
		PARDEF(FN_SECTORS,	n, VT_UINT,		_T("Sectors for read") )
		PARDEF(_T("isp"),		i, VT_UCHAR,	_T("ISP block id, default: 0.") )
	)

	COMMAND_DEFINE(_T("reset"),			ResetCpu, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		,	_T("Reset CPU")
	)

END_COMMAND_DEFINATION
#undef __CLASS_NAME__

CPluginIsp::CPluginIsp(void)
	: m_smi_dev(NULL)
{
	CSvtApplication * app = CSvtApplication::GetApp();
	app->GetDevice(m_smi_dev);
	JCASSERT(m_smi_dev);
}

CPluginIsp::~CPluginIsp(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}

bool CPluginIsp::Regist(CPluginManager * manager)
{
	LOG_STACK_TRACE();
	//manager->RegistPlugin(
	//	_T("isp"), NULL, CPluginManager::CPluginInfo::PIP_SINGLETONE, 
	//	CPluginIsp::Create);
	return true;
}

jcscript::IPlugin * CPluginIsp::Create(jcparam::IValue * param)
{
	return static_cast<jcscript::IPlugin*>(new CPluginIsp());
}

bool CPluginIsp::OnlineDownloadIsp(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(m_smi_dev);

	// Verify device
	//CCardInfo info;
	//m_smi_dev->GetCardInfo(info);
	//if (info.m_controller_name != _T("LT2244") )	THROW_ERROR(ERR_UNSUPPORT, 
	//	_T("This feature only supported bu SM2244LT."));

	stdext::auto_cif<IAtaDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not connect to ATA interface."));

	CBinaryBuffer * data = dynamic_cast<CBinaryBuffer*>(varin);
	if (!data) THROW_ERROR(ERR_PARAMETER, _T("Missing input data or type unmatch."));
	JCSIZE len = data->GetSize();
	if ( len % SECTOR_SIZE != 0 ) THROW_ERROR(ERR_PARAMETER, 
		_T("Data length must be in sector size."));
	JCSIZE remain_secs = len / SECTOR_SIZE;

	JCSIZE segment = 0;
	BYTE * buf = data->Lock();
	while ( remain_secs > 0 )
	{
		JCSIZE download_secs = min(remain_secs, 256);
		stdext::jc_printf(_T("Downloading segment %d.\n"), segment);
		// write one segment
		ATA_REGISTER reg;
		memset(&reg, 0, sizeof(reg));
		reg.command = 0xB0;
		reg.feature = 0xF0;
//		reg.device = 0xA0;
		reg.sec_count = (BYTE)(download_secs & 0xFF);
		reg.lba_mid = (BYTE)(segment);
		storage->AtaCommand(reg, write, false, buf, download_secs);
		if ( (reg.status & 0x01) != 0) THROW_ERROR(ERR_APP, 
			_T("Failure on writing data, status=0x%02X, err=0x%02X, seg=%d"), reg.status, reg.error, segment);
		remain_secs -= download_secs;
		segment ++;
		buf += download_secs * SECTOR_SIZE;
		Sleep(1000);
	}
	data->Unlock();
	stdext::jc_printf(_T("Succeeded downloading ISP.\n"));
	return true;
}

bool CPluginIsp::OnlineReadIsp(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(m_smi_dev);
	JCASSERT(NULL == varout);

	// Verify device
	//CCardInfo info;
	//m_smi_dev->GetCardInfo(info);
	//if (info.m_controller_name != _T("LT2244") )	THROW_ERROR(ERR_UNSUPPORT, 
	//	_T("This feature only supported bu SM2244LT."));

	stdext::auto_cif<IAtaDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not connect to ATA interface."));

	BYTE isp_id = 0;
	argu.GetValT(_T("isp"), isp_id);

	JCSIZE secs = 0;
	argu.GetValT(FN_SECTORS, secs);
	if (0 == secs) THROW_ERROR(ERR_PARAMETER, _T("Missing ISP length (in sectors)."));

	stdext::auto_interface<CBinaryBuffer> data;
	CBinaryBuffer::Create(secs * SECTOR_SIZE, data);

	//JCSIZE len = data->GetSize();

	JCSIZE remain_secs = secs;

	JCSIZE segment = 0;
	BYTE * buf = data->Lock();
	while ( remain_secs > 0 )
	{
		JCSIZE read_secs = min(remain_secs, 256);

		// write one segment
		ATA_REGISTER reg;
		memset(&reg, 0, sizeof(reg));
		reg.command = 0xB0;
		reg.feature = 0xF1;
		reg.lba_low = isp_id;
		reg.sec_count = (BYTE)(read_secs & 0xFF);
		reg.lba_mid = (BYTE)(segment);

		storage->AtaCommand(reg, read, false, buf, read_secs);
		if ( (reg.status & 0x01) != 0) THROW_ERROR(ERR_APP, 
			_T("Failure on writing data, status=0x%02X, err=0x%02X, seg=%d"), reg.status, reg.error, segment);

		//
		remain_secs -= read_secs;
		segment ++;
		buf += read_secs * SECTOR_SIZE;
	}
	data->Unlock();
	data.detach(varout);
	stdext::jc_printf(_T("Succeeded reading ISP.\n"));
	return true;
}

bool CPluginIsp::ResetCpu(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();

	JCASSERT(m_smi_dev);
	JCASSERT(NULL == varout);

	// Verify device
	//CCardInfo info;
	//m_smi_dev->GetCardInfo(info);
	//if (info.m_controller_name != _T("LT2244") )	THROW_ERROR(ERR_UNSUPPORT, 
	//	_T("This feature only supported bu SM2244LT."));

	stdext::auto_cif<IAtaDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not connect to ATA interface."));

	// reset cpu
	ATA_REGISTER reg;
	memset(&reg, 0, sizeof(reg));
	reg.command = 0xB0;
	reg.feature = 0xF2;

	storage->AtaCommand(reg, read, false, NULL, 0);
	if ( (reg.status & 0x01) != 0) THROW_ERROR(ERR_APP, 
		_T("Failure on writing data, status=0x%02X, err=0x%02X"), reg.status, reg.error);
	stdext::jc_printf(_T("Device reseted.\n"));

	//
	return true;
}
