// IspDownloadTest.cpp : Defines the entry point for the console application.
//

#include "config.h"
#include "stdafx.h"

#include <windows.h>

#include <UsbMassStorage.h>
#include <stdext.h>
#include "SM3257EN.h"

LOCAL_LOGGER_ENABLE(_T("IspDwonload"), LOGGER_LEVEL_DEBUGINFO);

/*
 * Search for a specific USB device by vendor and device ID
 */
IUsbMassStorage * find_usb_dev()
{
	LOG_STACK_TRACE()
	IUsbMassStorage * dev = NULL;
	CUMSList list;

	CUsbMassStorageHelper::EnumDevice(list);
	BYTE szInquiry[0x25];

	CUMSList::iterator end_it = list.end();
	for (CUMSList::iterator it = list.begin(); it != end_it; it++)
	{
		dev = (*it);
		if ( dev )
		{
			memset(szInquiry, 0, 0x25);
			dev->Inquiry(szInquiry, 0x24);
			char * str = (char*)(szInquiry+5);
			if (strstr(str, "smi") != 0)
			{
				list.erase(it);
				CUsbMassStorageHelper::ReleaseDeviceList(list);
				break;
			}
			dev = NULL;
		}
	}
	return dev;
}

/*
 * Read the FlashIDBuffer for Checking some information
 */
int SMI_DoReadFlashIDBuffer(IUsbMassStorage * dev, BYTE * buf)
{
	BYTE cmd[16];
	memset(cmd, 0 , sizeof(cmd));
	cmd[0x00] = 0xF0;
	cmd[0x01] = 0x06;
	cmd[0x0B] = 0x01;
	return dev->UsbScsiRead(buf ,512, cmd, 0x10);
}

int _tmain(int argc, _TCHAR* argv[])
{
	IUsbMassStorage * usb_dev = NULL;
	// Open log 
	LOGGER_TO_FILE(O, _T("sm3257.log") );
	CJCLogger::Instance()->SetColumnSelect( CJCLogger::COL_COMPNENT_NAME | CJCLogger::COL_FUNCTION_NAME);

	LOG_DEBUG(_T("isp download"));
	try
	{
		CUsbMassStorageHelper::UsbInit();
		usb_dev = find_usb_dev();
		if (usb_dev)
		{
			CSM3257EN dev(usb_dev);
			BYTE FlashIDBuf[512];
			int len = 512;
			dev.ReadFlashIDBuffer(FlashIDBuf, len);

			CStringA ver;
			dev.GetISPVer(ver);
			_tprintf(_T("ISP ver: %S \r\n"), ver.c_str() );
		}
	}
	catch (stdext::CJCException & err)
	{
		err.what();
	}

	getchar();
	
	return 0;
}