#pragma once
#include "storage_device_comm.h"

class CI2CStorageDevice : public CStorageDeviceComm
{
protected:
	CI2CStorageDevice(HANDLE dev);
	virtual ~CI2CStorageDevice(void);

public:
	virtual bool Recognize(void);
	static void Create(HANDLE dev, IStorageDevice * &);

public:
	virtual bool ScsiRead(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout);
	virtual bool ScsiWrite(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout);
	//FILESIZE GetCapacity(void) {return 0;};

protected:
	bool I2CCommand(WORD cmd_id, READWRITE rd_wr, BYTE * buf, JCSIZE buf_len);
	bool SendAtaCommand(ATA_REGISTER & reg, BYTE * buf);
	bool ReadStatus(BYTE * buf);
	bool WriteData(BYTE * buf, BYTE * data_buf);
	bool ReadData(BYTE * buf, BYTE * data_buf);
};
