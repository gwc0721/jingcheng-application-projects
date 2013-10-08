#pragma once
#include "../include/istorage_device.h"
#include "../include/smi_recognizer.h"
#include "../include/device_base.h"


#include "ntddscsi.h"
#include <vector>

#define SPT_SENSE_LENGTH	32
#define SPTWB_DATA_LENGTH	4*512
#define CDB6GENERIC_LENGTH	6
#define CDB10GENERIC_LENGTH	16

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER 
{
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS 
{
    SCSI_PASS_THROUGH spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
    UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

class CStorageDeviceComm 
	: virtual public IStorageDevice
	, public CStorageDeviceBase
	, public CJCInterfaceBase
{
public:
	CStorageDeviceComm(HANDLE dev);
	virtual ~CStorageDeviceComm(void);

public:
	virtual bool DeviceLock(void);
	virtual bool DeviceUnlock(void);
	virtual bool Dismount(void);

	virtual bool Inquiry(BYTE * buf, JCSIZE buf_len);
	virtual bool SectorRead(BYTE * buf, FILESIZE lba, JCSIZE sectors);
	virtual bool SectorWrite(const BYTE * buf, FILESIZE lba, JCSIZE sectors);
	virtual bool ScsiRead(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout);
	virtual bool ScsiWrite(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout);

	virtual void SetDeviceName(LPCTSTR name)
	{
		m_device_name = name;
	}

	virtual void UnmountAllLogical(void);
	virtual bool ScsiCommand(READWRITE rd_wr, BYTE *buf, JCSIZE length, BYTE *cb, JCSIZE cb_length, UINT timeout);
	virtual void Detach(HANDLE & dev);
	virtual FILESIZE GetCapacity(void);

	//virtual void GetTimeOut(UINT &rd, UINT &wr);
	//virtual void SetTimeOut(UINT rd, UINT wr);
	//virtual UINT GetTimeOut(void);
	//virtual void SetTimeOut(UINT);



protected:
	HANDLE		m_dev;
	std::vector<HANDLE>	m_logical_drives;
	CJCStringT	m_device_name;
	FILESIZE	m_capacity;		// capacitance in sector

#ifdef _DEBUG
	virtual HANDLE GetHandle() {return m_dev;};
#endif

#ifdef _OVER_WRITE_CHECK
public:
	virtual bool SectorVerify(void); 
protected:
	// backup sectors to verify
	BYTE * m_sec_0;
	BYTE * m_sec_55aa;
#endif

};