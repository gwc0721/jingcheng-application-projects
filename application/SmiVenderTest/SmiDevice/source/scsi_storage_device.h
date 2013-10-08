#pragma once

#include "storage_device_comm.h"
#include <tchar.h>

class CScsiStorageDevice: public CStorageDeviceComm
{
public:
	//virtual FILESIZE GetCapacity(void);

protected:
	CScsiStorageDevice(HANDLE dev);
	virtual ~CScsiStorageDevice(void);
public:
	virtual bool Recognize(void) {return true;}
	static void Create(HANDLE dev, IStorageDevice * &);

	FILESIZE ReadCapacity(void);

protected:
	static CStorageDeviceInfo	m_register;
};