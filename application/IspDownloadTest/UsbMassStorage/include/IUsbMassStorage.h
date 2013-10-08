#pragma once

#include <windows.h>

class IUsbMassStorage
{
public:
	virtual ~IUsbMassStorage(void) {};
	virtual bool Inquiry(BYTE * buf, int buf_len) = 0;
	virtual bool UsbScsiRead(BYTE *buf, int length, BYTE *cb, int cb_length) = 0;
	virtual bool UsbScsiWrite(BYTE *buf, int length, BYTE *cb, int cb_length) = 0;

	virtual bool ReadSector(BYTE * buf, unsigned int LBA, int sectors) = 0;
	virtual bool WriteSector(BYTE * buf, unsigned int LBA, int sectors) = 0;
};
