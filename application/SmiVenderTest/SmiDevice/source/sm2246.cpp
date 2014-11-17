#include "stdafx.h"
#include "sm2246.h"

LOCAL_LOGGER_ENABLE(_T("device_sm2246"), LOGGER_LEVEL_ERROR);

CSM2246::CSM2246(IStorageDevice * dev)
	: CSmiDeviceComm(dev)
{
	//Init2244();
}

bool CSM2246::CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device)
{
	LOG_STACK_TRACE();
	JCASSERT(storage);
	smi_device =  static_cast<ISmiDevice*>(new CSM2246(storage));
	return true;
}


bool CSM2246::Recognize(IStorageDevice * storage, BYTE * inquery)
{
    LOG_STACK_TRACE();
	char vendor[16];
	memset(vendor, 0, 16);
	memcpy_s(vendor, 16, inquery + 8, 16);
	LOG_DEBUG(_T("Checking inquery = %S"), vendor);

	LOG_DEBUG(_T("Checking vender command..."));
	bool br = false;
	try
	{
		storage->DeviceLock();
		// try vendor command
		stdext::auto_array<BYTE> buf0(SECTOR_SIZE);
		storage->ScsiRead(buf0, 0x00AA, 1, 60);
		storage->ScsiRead(buf0, 0xAA00, 1, 60);
		storage->ScsiRead(buf0, 0x0055, 1, 60);
		storage->ScsiRead(buf0, 0x5500, 1, 60);
		storage->ScsiRead(buf0, 0x55AA, 1, 60);

		br = CSM2246::LocalRecognize(buf0);
		// Stop vender command
		storage->ScsiRead(buf0, 0x55AA, 1, 60);
	}
	catch (...)
	{
	}

	return br;
}

bool CSM2246::LocalRecognize(BYTE * data)
{
    LOG_STACK_TRACE();
	char * ic_ver = (char*)(data + 0x2E);	
	LOG_DEBUG(_T("%S"), ic_ver);
	return (NULL != strstr(ic_ver, "SM2246A") );
}
