#include "stdafx.h"


#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("SmiRecognizer"), LOGGER_LEVEL_ERROR);

#include "../include/smi_recognizer.h"

#include "ata_storage_device.h"
#include "scsi_storage_device.h"
#include "i2c_storage_device.h"
#include "dummy_storage_device.h"

#include "sm2242.h"
#include "sm2244lt.h"
#include "sm2232.h"
#include "dummy_smi_device.h"
#include "sm2246.h"


CSmiRecognizer::STORAGE_MAP CSmiRecognizer::m_storage_map;
CSmiRecognizer::DEVICE_MAP CSmiRecognizer::m_device_map;

CStorageDeviceInfo::CStorageDeviceInfo(LPCTSTR name, STORAGE_CREATOR creator)
	: m_name(name), m_creator(creator)
{
}


bool CSmiRecognizer::RegisterStorageDevice(const CStorageDeviceInfo & info)
{
	std::pair<STORAGE_ITERATOR, bool> rs = 
		m_storage_map.insert( STORAGE_PAIR(info.m_name, info) );
	if ( ! rs.second) THROW_ERROR(ERR_PARAMETER, 
		_T("Storage device %s has already been registered"), info.m_name);
	return true;
}

bool CSmiRecognizer::RegisterSmiDevice(const CSmiDeviceCreator & info)
{
	std::pair<DEVICE_ITERATOR, bool> rs = 
		m_device_map.insert( DEVICE_PAIR(info.m_name, info) );
	if ( ! rs.second) THROW_ERROR(ERR_PARAMETER, 
		_T("Smi device %s has already been registered"), info.m_name);
	return true;
}

void CSmiRecognizer::Register(void)
{
	RegisterStorageDevice( CStorageDeviceInfo(_T("ATA_DEVICE"), &CAtaStorageDevice::Create) );
	RegisterStorageDevice( CStorageDeviceInfo(_T("I2C_DEVICE"), &CI2CStorageDevice::Create) );
	RegisterStorageDevice( CStorageDeviceInfo(_T("SCSI_DEVICE"), &CScsiStorageDevice::Create) );
	//RegisterStorageDevice( CStorageDeviceInfo(_T("DUMMY"), &CDummyStorageDevice::Recognize, &CDummyStorageDevice::Recognize) );

	RegisterSmiDevice( CSmiDeviceCreator(_T("LT2244"), &CLT2244::Recognize, &CLT2244::CreateDevice) );
	RegisterSmiDevice( CSmiDeviceCreator(_T("SM2246"), &CSM2246::Recognize, &CSM2246::CreateDevice) );
	RegisterSmiDevice( CSmiDeviceCreator(_T("SM2242"), &CSM2242::Recognize, &CSM2242::CreateDevice) );
	RegisterSmiDevice( CSmiDeviceCreator(_T("SM2232"), &CSM2232::Recognize, &CSM2232::CreateDevice) );
	//RegisterSmiDevice( CSmiDeviceCreator(_T("DUMMY"), &CDummySmiDevice::Recognize, &CDummySmiDevice::CreateDevice) );
}

CSmiDeviceCreator CSmiRecognizer::m_smi_dummy_creator( _T("DUMMY"), &CDummySmiDevice::Recognize, &CDummySmiDevice::CreateDevice );

bool CSmiRecognizer::CreateDummyDevice(ISmiDevice * & smi_dev)
{
	IStorageDevice * storage= NULL;
	CDummyStorageDevice::Recognize(NULL, storage);
	CDummySmiDevice::CreateDevice(storage, smi_dev);
	storage->Release();
	return true;
}

bool CSmiRecognizer::RecognizeDevice(
		LPCTSTR device_name, ISmiDevice * &smi_dev, 
		const CJCStringT & force_storage, const CJCStringT & force_device)
{
	LOG_STACK_TRACE();
	LOG_TRACE(_T("Recogizing driver %s"), device_name);

	if ( 0  == m_storage_map.size() ) Register();
	JCASSERT(smi_dev == NULL);
	JCASSERT(device_name)

	bool found = false;

	LOG_DEBUG(_T("Opening driver %s"), device_name);
	HANDLE hdev = CreateFile(device_name, 
					GENERIC_READ|GENERIC_WRITE, 
			  		FILE_SHARE_READ|FILE_SHARE_WRITE, 
					NULL, 
					OPEN_EXISTING, 
					0 | FILE_FLAG_NO_BUFFERING, 
					NULL );
	if( INVALID_HANDLE_VALUE == hdev) return false;

	DWORD junk = 0;
	stdext::auto_interface<IStorageDevice> storage_device;

	if ( !force_storage.empty() )
	{
		LOG_DEBUG(_T("Force storage %s"), force_storage.c_str() );
		STORAGE_ITERATOR it = m_storage_map.find(force_storage);
		if ( it == m_storage_map.end() )
			THROW_ERROR(ERR_PARAMETER, _T("Unknow storage device %s"), force_storage.c_str() );
		CStorageDeviceInfo & info = it->second;
		if (NULL == info.m_creator) THROW_ERROR(
			ERR_UNSUPPORT, _T("Storage device %s doesn't support force create"), force_storage.c_str() );
		(*info.m_creator)(hdev, storage_device);
		JCASSERT(storage_device);
		bool br = storage_device->Recognize();
		if (!br ) 
		{
			LOG_DEBUG(_T("Force storage %s failed"), force_storage.c_str() );
			return false;
		}
	}
	else
	{
		STORAGE_ITERATOR it = m_storage_map.begin();
		STORAGE_ITERATOR endit = m_storage_map.end();
		for ( ; it != endit; ++ it)
		{
			CStorageDeviceInfo & info = it->second;
			LOG_DEBUG(_T("Trying storage %s"), info.m_name );
			
			JCASSERT(info.m_creator);
			(*info.m_creator)(hdev, storage_device);
			bool br = storage_device->Recognize();
			if (br )
			{
				LOG_DEBUG(_T("Storage %s is match for %s"), info.m_name, device_name);
				break;
			}
			storage_device->Detach(hdev);
			storage_device->Release();
		}
	}

	if (!storage_device) 
	{
		CloseHandle(hdev);
		return false;
	}
	storage_device->SetDeviceName(device_name);

	// Get Inquery buffer
	stdext::auto_array<BYTE>	inquery_buf(4* SECTOR_SIZE);
	memset((BYTE*)inquery_buf, 0, SECTOR_SIZE);
	storage_device->Inquiry(inquery_buf, SECTOR_SIZE);

	// Check if there is a tester

	stdext::auto_interface<ISmiDevice>	smi_device;
	CSmiDeviceCreator * creator = NULL;

	if ( !force_device.empty() )
	{
		LOG_DEBUG(_T("Force SMI device %s"), force_device.c_str() );
		if ( force_device == _T("DUMMY") )
		{
			creator = & m_smi_dummy_creator;
		}
		else
		{
			DEVICE_ITERATOR it = m_device_map.find(force_device);
			if ( it == m_device_map.end() )
				THROW_ERROR(ERR_PARAMETER, _T("Unknow SMI device %s"), force_device.c_str() );
			bool br = (it->second.m_recognizer)(storage_device, inquery_buf);
			if ( !br)  { LOG_DEBUG(_T("Device %s does not match drive %s"), it->second.m_name, device_name) }
			else creator = &(it->second);
		}
	}
	else
	{
		DEVICE_ITERATOR it = m_device_map.begin();
		DEVICE_ITERATOR endit = m_device_map.end();
		for ( ; it != endit; ++ it)
		{
			creator = &(it->second);
			LOG_DEBUG(_T("Trying SMI device %s"), creator->m_name );
			JCASSERT(creator->m_recognizer);
			if ( (*(creator->m_recognizer))(storage_device, inquery_buf) ) break;
			creator = NULL;
		}
	}

#if _OVER_WRITE_CHECK
	storage_device->SectorVerify();
#endif

	if ( !creator )	return false;

	// Do not unmount logical drive if using DUMMY controller
	if ( _tcscmp(creator->m_name, _T("DUMMY")) != 0 )	storage_device->UnmountAllLogical();

	JCASSERT(creator->m_creator);
	bool br = (*creator->m_creator)(storage_device, smi_device);
	if (!br || !smi_device) 
	{
		LOG_ERROR(_T("Failure on creating SMI device %s"), creator->m_name );
		return false;
	}
	smi_device->Initialize();
	smi_device.detach(smi_dev);
	return true;
}

SMI_DEVICE_TYPE CSmiRecognizer::AchieveDeviceType(IStorageDevice * dev)
{
	JCASSERT(dev);
	stdext::auto_array<BYTE> buf0(SECTOR_SIZE);
	stdext::auto_array<BYTE> buf1(SECTOR_SIZE);

	dev->SectorRead(buf0, 0x00AA, 1);
	dev->SectorRead(buf0, 0xAA00, 1);
	dev->SectorRead(buf0, 0x0055, 1);
	dev->SectorRead(buf0, 0x5500, 1);
	dev->SectorRead(buf0, 0x55AA, 1);

	// Dummy的执行Check Flash Assembly内部命令。
	// 一旦进入Vendor Command模式，必须完整地执行整个Vendor Command。
	// 即Read(0x00AA), Read(0xAA00), Read(0x0055), Read(0x5500), Read(0x55AA), 
	// Write(0x55AA) for Command, Read/Write(0x55AA) for data output/input
	memset(buf1, 0, SECTOR_SIZE);
	buf1[0] = 0xF0;
	buf1[1] = 0x20;
	buf1[0x0B] = 0x01;
	dev->SectorWrite(buf1, 0x55AA, 1);
	dev->SectorRead(buf1, 0x55AA, 1);

	SMI_DEVICE_TYPE dev_type = DEV_UNKNOWN;

	buf0[0x34] = 0;
	char * ic_ver = (char*)(buf0 + 0x2E);
	LOG_DEBUG(_T("Device type = %S"), ic_ver);

	if ( strcmp(ic_ver, "SM2241") == 0 )			dev_type = DEV_SM2241;
	else if ( strcmp(ic_ver, "SM2242") == 0 )		dev_type = DEV_SM2242;
	else
	{
		buf0[0x33] = 0;
		if ( strcmp(ic_ver, "SM224") == 0 )		dev_type = DEV_SM224;
		else if ( strcmp(ic_ver, "SM223") == 0 )	dev_type = DEV_SM223;
	}
	return dev_type;
}


