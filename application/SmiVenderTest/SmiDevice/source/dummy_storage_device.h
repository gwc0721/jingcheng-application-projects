#pragma once

#include "../include/istorage_device.h"
#include "../include/device_base.h"

class CDummyStorageDevice 
	: public CStorageDeviceBase
{
public:
	static bool Recognize(HANDLE dev, IStorageDevice * & i_dev)
	{
		JCASSERT(NULL == i_dev);
		i_dev = new CDummyStorageDevice;
		return true;	
	}
};