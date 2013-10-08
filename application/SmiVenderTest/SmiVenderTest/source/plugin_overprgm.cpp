#include "stdafx.h"
#include <stdext.h>
#include "plugin_manager.h"
#include "plugin_overprgm.h"
#include "application.h"

LOCAL_LOGGER_ENABLE(_T("overprgm_pi"), LOGGER_LEVEL_ERROR);

#define __CLASS_NAME__	CPluginOverPrgm
BEGIN_COMMAND_DEFINATION
	COMMAND_DEFINE(_T("enable"),	Enable, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		,	_T("Enable or disable over program."),
		PARDEF(_T("disable"),	d, VT_BOOL,		_T("Disable if select, otherwise enable.") )
	)

	COMMAND_DEFINE(_T("range"),		SetRange, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		,	_T("Set over program range."), 
		PARDEF(_T("start_1"),	a, VT_UINT,		_T("Start LBA of range 1") )
		PARDEF(_T("sectors_1"),	m, VT_UINT,		_T("Length of range 1 in sector") )
		PARDEF(_T("start_2"),	b, VT_UINT,		_T("Start LBA of range 2") )
		PARDEF(_T("sectors_2"),	n, VT_UINT,		_T("Length of range 2 in sector") )
	)

	COMMAND_DEFINE(_T("blocksize"),	SetBlockSize, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		,	_T("Set block size. Only if cannot recognize device"),
		PARDEF(FN_SECTORS,	n, VT_UINT,		_T("Sectors per block") )
	)
	COMMAND_DEFINE(_T("clean"),		CleanCache, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		,	_T("Force clean cache blocks.")
	)

END_COMMAND_DEFINATION
#undef __CLASS_NAME__

CPluginOverPrgm::CPluginOverPrgm(void)
	: m_smi_dev(NULL)
{
	CSvtApplication * app = CSvtApplication::GetApp();
	app->GetDevice(m_smi_dev);
	JCASSERT(m_smi_dev);

	m_sec_per_block = 1024;	// default for 651G1
}

CPluginOverPrgm::~CPluginOverPrgm(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}

bool CPluginOverPrgm::Regist(CPluginManager * manager)
{
	LOG_STACK_TRACE();
	//manager->RegistPlugin(
	//	_T("overprgm"), NULL, CPluginManager::CPluginInfo::PIP_SINGLETONE, 
	//	CPluginOverPrgm::Create);
	return true;
}

jcscript::IPlugin * CPluginOverPrgm::Create(jcparam::IValue * param)
{
	return static_cast<jcscript::IPlugin*>(new CPluginOverPrgm());
}

bool CPluginOverPrgm::Enable(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(m_smi_dev);

	stdext::auto_cif<IAtaDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not connect to ATA interface."));

	BYTE enable = 1;
	if ( argu.Exist(_T("disable")) ) enable = 0;

	// write one segment
	ATA_REGISTER reg;
	memset(&reg, 0, sizeof(reg));
	reg.command = 0xB0;
	reg.feature = 0xF3;
	reg.lba_low = enable;
	storage->AtaCommand(reg, read, false, NULL, 0);
	if ( (reg.status & 0x01) != 0) THROW_ERROR(ERR_APP, 
		_T("Failure on writing data, status=0x%02X, err=0x%02X"), reg.status, reg.error);

	stdext::jc_printf(_T("Over program %s.\n"), enable?_T("enabled"):_T("disabled"));
	return true;
}

bool CPluginOverPrgm::SetRange(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

	// Verify device
	//CCardInfo info;
	//m_smi_dev->GetCardInfo(info);
	//if (info.m_controller_name != _T("LT2244") )	THROW_ERROR(ERR_UNSUPPORT, 
	//	_T("This feature only supported bu SM2244LT."));

	stdext::auto_cif<IAtaDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not connect to ATA interface."));


	stdext::auto_array<BYTE> buf(SECTOR_SIZE);
	memset(buf, 0, SECTOR_SIZE);

	static LPCTSTR param_name_start[2] = {_T("start_1"), _T("start_2")};	
	static LPCTSTR param_name_secs[2] = {_T("sectors_1"), _T("sectors_2")};	
	static const JCSIZE offset[2] = {0, 16};

	for (int ii = 0; ii < 2; ++ii)
	{
		JCSIZE start = 0, secs = 0xFFFFFFFF;
		argu.GetValT(param_name_secs[ii], secs);
		argu.GetValT(param_name_start[ii], start);

		LOG_DEBUG(_T("range %d, start=0x%X, sectors=0x%X"), ii, start, secs);
		if (0xFFFFFFFF == secs) continue;

		JCSIZE start_block = start / m_sec_per_block;
		JCSIZE blocks = 0;
		if (0 != secs)
		{
			JCSIZE end = start + secs;
			JCSIZE end_block = (end-1) / m_sec_per_block +1;
			blocks = end_block - start_block;
		}
		LOG_DEBUG(_T("range %d, start_block=0x%X, blocks=0x%X"), ii, start_block, blocks);

		BYTE * _buf = buf + offset[ii];
		_buf[0] = 1;		// valid
		_buf[1] = HIBYTE(LOWORD(start_block));
		_buf[2] = LOBYTE(LOWORD(start_block));
		_buf[3] = HIBYTE(LOWORD(blocks));
		_buf[4] = LOBYTE(LOWORD(blocks));
		//stdext::jc_printf
	}

	// write ata command
	ATA_REGISTER reg;
	memset(&reg, 0, sizeof(reg));
	reg.command = 0xB0;
	reg.feature = 0xF4;
	reg.sec_count = 1;
	storage->AtaCommand(reg, write, false, buf, 1);
	if ( (reg.status & 0x01) != 0) THROW_ERROR(ERR_APP, 
			_T("Failure on writing data, status=0x%02X, err=0x%02X"), reg.status, reg.error);
	stdext::jc_printf(_T("Over program range was set.\n"));
	return true;
}

bool CPluginOverPrgm::SetBlockSize(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	argu.GetValT(FN_SECTORS, m_sec_per_block);
	stdext::jc_printf(_T("Block size = %d sectors.\n"), m_sec_per_block);
	return true;
}

bool CPluginOverPrgm::CleanCache(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

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
	reg.feature = 0xF5;

	stdext::jc_printf(_T("Cleaning cache blocks...\n"));
	storage->AtaCommand(reg, read, false, NULL, 0);
	if ( (reg.status & 0x01) != 0) THROW_ERROR(ERR_APP, 
		_T("Failure on writing data, status=0x%02X, err=0x%02X"), reg.status, reg.error);
	stdext::jc_printf(_T("Finished.\n"));
	return true;
}
