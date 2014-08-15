#pragma once
#include "storage_device_comm.h"

class CAtaStorageDevice : virtual public IAtaDevice, public CStorageDeviceComm
{
protected:
	CAtaStorageDevice(HANDLE dev)
		: CStorageDeviceComm(dev)
		, m_max_lba(0)
		, m_feature_lba48(false)
	{}

	virtual ~CAtaStorageDevice(void) {}
public:
	static const JCSIZE	ID_TABLE_LEN;

public:
	bool QueryInterface(const char * if_name, IJCInterface * &if_ptr);
	virtual FILESIZE GetCapacity(void) {return m_max_lba;}
	//virtual void ReadSmartAttribute(BYTE * buf, JCSIZE len);

	virtual bool AtaCommand(ATA_REGISTER &reg, READWRITE read_write, bool dma, BYTE * buf, JCSIZE secs);
	virtual void AtaCommand48(ATA_REGISTER &previous, ATA_REGISTER &current, READWRITE read_write, BYTE * buf, JCSIZE secs);
	virtual BYTE ReadDMA(FILESIZE lba, JCSIZE secs, BYTE &error, BYTE * buf);
	virtual BYTE ReadPIO(FILESIZE lba, BYTE secs, BYTE &error, BYTE * buf);
	virtual BYTE WritePIO(FILESIZE lba, JCSIZE secs, BYTE &error, BYTE * buf);
	virtual BYTE WriteDMA(FILESIZE lba, JCSIZE secs, BYTE &error, BYTE * buf);
	virtual BYTE FlushCache(BYTE & error);
	virtual bool Recognize(void);

	virtual bool ReadSmartData(BYTE * buf, JCSIZE len);
	virtual bool IdentifyDevice(BYTE * buf, JCSIZE len);

public:
	static void Create(HANDLE dev, IStorageDevice * &);

protected:
	FILESIZE	m_max_lba;
	bool		m_feature_lba48;
};
