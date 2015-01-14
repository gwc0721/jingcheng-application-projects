#pragma once

#include <stdext.h>


const char IF_NAME_STORAGE_DEVICE[] = "IStorageDevice";
const char IF_NAME_ATA_DEVICE[] = "IAtaDevice";

enum READWRITE
{
	write,
	read,
};

typedef struct _ATA_REGISTER
{
	union
	{
		BYTE feature;
		BYTE error;
	};
	BYTE sec_count;
	BYTE lba_low;
	BYTE lba_mid;
	BYTE lba_hi;
	BYTE device;
	union
	{
		BYTE command;
		BYTE status;
	};
	BYTE dummy;
} ATA_REGISTER;

class IStorageDevice : public virtual IJCInterface
{
public:

public:
	virtual bool Inquiry(BYTE * buf, JCSIZE buf_len) = 0;
	// rd_wr: Read (true) or Write (false)
	virtual bool SectorRead(BYTE * buf, FILESIZE lba, JCSIZE sectors) = 0;
	virtual bool SectorWrite(const BYTE * buf, FILESIZE lba, JCSIZE sectors) = 0;

	virtual bool ScsiRead(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout) = 0;
	virtual bool ScsiWrite(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout) = 0;

	// 用于性能测试，单位us。
	virtual UINT GetLastInvokeTime(void) = 0;

	virtual FILESIZE GetCapacity(void) = 0;
	virtual bool DeviceLock(void) = 0;
	virtual bool DeviceUnlock(void) = 0;
	virtual bool Dismount(void) = 0;

	virtual void SetDeviceName(LPCTSTR name) = 0;

	virtual void UnmountAllLogical(void) = 0;
	// time out in second
	virtual bool ScsiCommand(READWRITE rd_wr, BYTE *buf, JCSIZE length, BYTE *cb, JCSIZE cb_length, UINT timeout) = 0;
	//virtual void FlushCache() = 0;
	//virtual UINT GetTimeOut(void) = 0; 
	//virtual void SetTimeOut(UINT) = 0;

	virtual bool Recognize() = 0;
	virtual void Detach(HANDLE & dev) = 0;


#ifdef _DEBUG
	virtual HANDLE GetHandle() = 0;
#endif

#ifdef _OVER_WRITE_CHECK
	virtual bool SectorVerify(void) = 0; 
#endif
};

class IAtaDevice : public virtual IStorageDevice
{
public:
	//virtual void ReadSmartAttribute(BYTE * buf, JCSIZE len) = 0;
	virtual bool AtaCommand(ATA_REGISTER &reg, READWRITE read_write, bool dma, BYTE * buf, JCSIZE secs) = 0;
	virtual void AtaCommand48(ATA_REGISTER &previous, ATA_REGISTER &current, READWRITE read_write, BYTE * buf, JCSIZE secs) = 0;
	// Return error regist in lowest byte
	virtual BYTE ReadDMA(FILESIZE lba, JCSIZE secs, BYTE &error, BYTE * buf) =0;
	// Return status
	virtual BYTE ReadPIO(FILESIZE lba, BYTE secs, BYTE &error, BYTE * buf) = 0;

	virtual BYTE WritePIO(FILESIZE lba, JCSIZE secs, BYTE &error, BYTE * buf) = 0;
	virtual BYTE WriteDMA(FILESIZE lba, JCSIZE secs, BYTE &error, BYTE * buf) = 0;
	virtual BYTE FlushCache(BYTE & error) = 0;

	virtual bool ReadSmartData(BYTE * buf, JCSIZE len) = 0;
	virtual bool IdentifyDevice(BYTE * buf, JCSIZE len) = 0;
};

typedef void (* STORAGE_CREATOR)(HANDLE, IStorageDevice * &);
