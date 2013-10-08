
#include "UsbMassStorage_IoCtrl.h"
#include <stdext.h>
#include "ntddscsi.h"


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


CUsbMassStorage::CUsbMassStorage(TCHAR driver_letter)
	: m_dev(NULL)
{
	TCHAR DiskName[32];
	_stprintf(DiskName, _T("\\\\.\\%c:"), driver_letter);
	HANDLE dev = CreateFile(DiskName, 
					GENERIC_READ|GENERIC_WRITE, 
			  		FILE_SHARE_READ|FILE_SHARE_WRITE, 
					NULL, 
					OPEN_EXISTING, 
					FILE_FLAG_NO_BUFFERING, 
					NULL );
	if( INVALID_HANDLE_VALUE == dev) THROW_WIN32_ERROR("open driver error!")
	m_dev = dev;
}

CUsbMassStorage::CUsbMassStorage(HANDLE dev)
	: m_dev(dev)
{
	if (!m_dev || INVALID_HANDLE_VALUE == m_dev ) THROW_ERROR(ERR_APP, "invalid handle")
}


CUsbMassStorage::~CUsbMassStorage(void)
{
	if (m_dev) CloseHandle(m_dev);
}

bool CUsbMassStorage::Inquiry(BYTE * buf, int buf_len)
{
	JCASSERT(buf);
	JCASSERT(m_dev);

	BYTE cb[16];
	memset(cb, 0, 16);
	cb[0] = 0x12;
	cb[4] = 0x24;

	UsbScsiRead(buf, buf_len, cb, 16);
	return true;
}

bool CUsbMassStorage::UsbScsiRead(unsigned char *buf, int length, unsigned char *cb, int cb_length)
{
	JCASSERT(m_dev);
	JCASSERT(buf);
	JCASSERT(cb);

	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
    ZeroMemory(&sptdwb, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
    sptdwb.sptd.Length= sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = cb_length;
    sptdwb.sptd.SenseInfoLength = 0x00;
    sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
    sptdwb.sptd.DataTransferLength = length;
    sptdwb.sptd.TimeOutValue = 300;
    sptdwb.sptd.DataBuffer = buf;
//        sptdwb.sptd.SenseInfoOffset = offsetof(struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
	// get offset
	sptdwb.sptd.SenseInfoOffset = (char*)(&(sptdwb.ucSenseBuf)) - (char*)(&(sptdwb));
	memcpy(sptdwb.sptd.Cdb, cb, cb_length < CDB10GENERIC_LENGTH ? cb_length : CDB10GENERIC_LENGTH);
	
    ULONG llength = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
    ULONG	returned = 0;
    BOOL	success;

	success = DeviceIoControl(
				   m_dev,
				   IOCTL_SCSI_PASS_THROUGH_DIRECT,
				   &sptdwb,
				   llength,
				   &sptdwb,
				   llength,
				   &returned,
				   FALSE );
	if (!success) THROW_WIN32_ERROR("send scsi command failed");
    FlushFileBuffers(m_dev);
	return true;
}

bool CUsbMassStorage::UsbScsiWrite(BYTE *buf, int length, BYTE *cb, int cb_length)
{
	int i;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
	ZeroMemory(&sptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
	sptdwb.sptd.Length= sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptdwb.sptd.PathId = 0;
	sptdwb.sptd.TargetId = 1;
	sptdwb.sptd.Lun = 0;
	sptdwb.sptd.CdbLength = cb_length;//CDB10GENERIC_LENGTH;
	sptdwb.sptd.SenseInfoLength = 0x00;
	sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
	sptdwb.sptd.DataTransferLength = length;
	sptdwb.sptd.TimeOutValue = 300;
	sptdwb.sptd.DataBuffer = buf;
	sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
	memcpy(sptdwb.sptd.Cdb, cb, cb_length < CDB10GENERIC_LENGTH ? cb_length : CDB10GENERIC_LENGTH);
	
	ULONG	llength = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	ULONG	returned = 0;
	BOOL	success;
	
	success = DeviceIoControl(
					m_dev,
					IOCTL_SCSI_PASS_THROUGH_DIRECT,
					&sptdwb,
					llength,
					&sptdwb,
					llength,
					&returned,
					FALSE );
	if (!success) THROW_WIN32_ERROR("send scsi command failed");
	FlushFileBuffers(m_dev);
	return true;
}

bool CUsbMassStorage::ReadSector(BYTE * buf, unsigned int LBA, int sectors)
{
	JCASSERT(buf);

	BYTE cmd[16];
	memset(cmd, 0 , sizeof(cmd));
	cmd[0x00] = 0x28;
//	cmd[0x01] = ;

	cmd[0x05] = LBA && 0xFF, LBA >>= 8;
	cmd[0x04] = LBA && 0xFF, LBA >>= 8;
	cmd[0x03] = LBA && 0xFF, LBA >>= 8;
	cmd[0x02] = LBA && 0xFF, LBA >>= 8;

	cmd[0x08] = sectors && 0xFF;
	cmd[0x07] = (sectors >> 8) && 0xFF;

	int buf_len = sectors * 256;
	return UsbScsiRead(buf, buf_len, cmd, 0x10);		
}

bool CUsbMassStorage::WriteSector(BYTE * buf, unsigned int LBA, int sectors)
{
	JCASSERT(buf);

	BYTE cmd[16];
	memset(cmd, 0 , sizeof(cmd));
	cmd[0x00] = 0x2A;
//	cmd[0x01] = ;

	cmd[0x05] = LBA && 0xFF, LBA >>= 8;
	cmd[0x04] = LBA && 0xFF, LBA >>= 8;
	cmd[0x03] = LBA && 0xFF, LBA >>= 8;
	cmd[0x02] = LBA && 0xFF, LBA >>= 8;

	cmd[0x08] = sectors && 0xFF;
	cmd[0x07] = (sectors >> 8) && 0xFF;

	int buf_len = sectors * 256;
	return UsbScsiRead(buf, buf_len, cmd, 0x10);		

}
