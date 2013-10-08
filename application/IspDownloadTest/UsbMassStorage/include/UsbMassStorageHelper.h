#pragma once

#include "IUsbMassStorage.h"
#include <list>

typedef std::list<IUsbMassStorage*> CUMSList;


class CUsbMassStorageHelper
{
public:
	CUsbMassStorageHelper(void);
public:
	~CUsbMassStorageHelper(void);

public:
	static bool UsbInit();
	static bool EnumDevice(CUMSList & list);
	static bool ReleaseDeviceList(CUMSList & list);
	static IUsbMassStorage * CreateDevice(HANDLE dev);
};
