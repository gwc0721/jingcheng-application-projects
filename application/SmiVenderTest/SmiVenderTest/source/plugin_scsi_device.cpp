#include "stdafx.h"

#include "plugin_scsi_device.h"
#include "plugin_manager.h"
#include "application.h"

LOCAL_LOGGER_ENABLE(_T("ata_pi"), LOGGER_LEVEL_ERROR);

using namespace jcparam;

#define __CLASS_NAME__	CPluginScsiDevice
BEGIN_COMMAND_DEFINATION
	COMMAND_DEFINE(_T("inquiry"),	Inquiry,		0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Inquiry.")
		//, PARDEF(_T("raw"),		r, VT_BOOL,		_T("Return raw data.") )
	)

END_COMMAND_DEFINATION
#undef __CLASS_NAME__

CPluginScsiDevice::CPluginScsiDevice(jcparam::IValue * param)
	: m_smi_dev(NULL)
{
	CSvtApplication * app = CSvtApplication::GetApp();
	app->GetDevice(m_smi_dev);

}


CPluginScsiDevice::~CPluginScsiDevice(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}


bool CPluginScsiDevice::Regist(CPluginManager * manager)
{
	LOG_STACK_TRACE();
	//manager->RegistPlugin(_T("scsi"), NULL, CPluginManager::CPluginInfo::PIP_SINGLETONE, CPluginScsiDevice::Create);
	return true;
}

jcscript::IPlugin * CPluginScsiDevice::Create(jcparam::IValue * param)
{
	return static_cast<jcscript::IPlugin*>(new CPluginScsiDevice(param));
}


bool CPluginScsiDevice::Inquiry(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(varout == NULL);
	JCASSERT(m_smi_dev);

	stdext::auto_cif<IStorageDevice>	storage;
	m_smi_dev->QueryInterface(IF_NAME_STORAGE_DEVICE, storage);
	if (!storage.valid())	THROW_ERROR(ERR_UNSUPPORT, _T("Device is not an ATA device"));


	stdext::auto_interface<CBinaryBuffer>	data;
	CBinaryBuffer::Create(SECTOR_SIZE, data);

	storage->Inquiry(data->Lock(), SECTOR_SIZE);
	data->Unlock();
	
	data.detach(varout);
	return true;
}


