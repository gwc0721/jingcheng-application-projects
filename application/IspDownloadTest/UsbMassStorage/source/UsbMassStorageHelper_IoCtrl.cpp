#include "../include/UsbMassStorageHelper.h"
#include "UsbMassStorage_IoCtrl.h"
//#include <stdext.h>

CUsbMassStorageHelper::CUsbMassStorageHelper(void)
{
}

CUsbMassStorageHelper::~CUsbMassStorageHelper(void)
{
}

bool CUsbMassStorageHelper::UsbInit()
{
	return true;
}

bool CUsbMassStorageHelper::EnumDevice(CUMSList & list)
{
	TCHAR DiskName[32];
	for(TCHAR dev_letter = _T('D'); dev_letter <= _T('Z'); dev_letter++)
	{
		_stprintf(DiskName, _T("\\\\.\\%c:"), dev_letter);
		HANDLE dev = CreateFile(DiskName, 
					GENERIC_READ|GENERIC_WRITE, 
			  		FILE_SHARE_READ|FILE_SHARE_WRITE, 
					NULL, 
					OPEN_EXISTING, 
					FILE_FLAG_NO_BUFFERING, 
					NULL );
		if( INVALID_HANDLE_VALUE != dev)
		{
			CUsbMassStorage * usb_dev = new CUsbMassStorage(dev);
			list.push_back(usb_dev);
		}
	}
	return true;
}

bool CUsbMassStorageHelper::ReleaseDeviceList(CUMSList & list)
{
	CUMSList::iterator end_it = list.end();
	for (CUMSList::iterator it = list.begin(); it != end_it; it++)
	{
		delete (*it);
	}
	return true;
}

IUsbMassStorage * CUsbMassStorageHelper::CreateDevice(HANDLE dev)
{
	return static_cast<IUsbMassStorage*>(new CUsbMassStorage(dev));
}
