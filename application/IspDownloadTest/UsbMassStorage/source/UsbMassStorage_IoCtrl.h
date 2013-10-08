#pragma once

#include "../include/IUsbMassStorage.h"
#include <tchar.h>

class CUsbMassStorage: virtual public IUsbMassStorage
{
public:
	CUsbMassStorage(TCHAR driver_letter);
	CUsbMassStorage(HANDLE dev);
public:
	virtual ~CUsbMassStorage(void);

public:
	bool Inquiry(BYTE * buf, int buf_len);
	bool UsbScsiRead(BYTE *buf, int length, BYTE *cb, int cb_length);
	bool UsbScsiWrite(BYTE *buf, int length, BYTE *cb, int cb_length);

	virtual bool ReadSector(BYTE * buf, unsigned int LBA, int sectors);
	virtual bool WriteSector(BYTE * buf, unsigned int LBA, int sectors);

protected:
	HANDLE m_dev;
};