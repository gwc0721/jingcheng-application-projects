#include "../include/UsbMassStorageHelper.h"
#include "UsbMassStorage_libusb.h"
#include <stdext.h>
#include <usb.h>

CUsbMassStorageHelper::CUsbMassStorageHelper(void)
{
}

CUsbMassStorageHelper::~CUsbMassStorageHelper(void)
{
}

bool CUsbMassStorageHelper::UsbInit()
{
	usb_init();
	return true;
}

bool CUsbMassStorageHelper::EnumDevice(CUMSList & list)
{	
	usb_bus * bus;
	struct usb_device	* dev;
	int num_bus, num_dev;

	num_bus = usb_find_busses();
	num_dev = usb_find_devices();

	for (bus = usb_busses; bus; bus = bus->next) 
	{
		for (dev = bus->devices; dev; dev = dev->next) 
		{
			if (dev) 
			{
				CUsbMassStorage * usb_dev = new CUsbMassStorage(dev);
				list.push_back(static_cast<IUsbMassStorage*>(usb_dev) );
			}
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
