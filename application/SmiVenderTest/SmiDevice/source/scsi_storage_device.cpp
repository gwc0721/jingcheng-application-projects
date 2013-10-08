
#include "stdafx.h"

#include "scsi_storage_device.h"


LOCAL_LOGGER_ENABLE(_T("CScsiStorageDevice"), LOGGER_LEVEL_DEBUGINFO); 

CScsiStorageDevice::CScsiStorageDevice(HANDLE dev)
	: CStorageDeviceComm(dev)
	//, m_max_lba(0)
{
}

CScsiStorageDevice::~CScsiStorageDevice(void)
{
}

void CScsiStorageDevice::Create(HANDLE dev, IStorageDevice * & i_dev)
{
	i_dev = static_cast<IStorageDevice*>(new CScsiStorageDevice(dev));
}

//FILESIZE CScsiStorageDevice::GetCapacity(void)
//{
	//JCASSERT(m_dev);
	//if (0 == m_max_lba)
	//{
	//	DISK_GEOMETRY dg;
	//	DWORD junk = 0;
	//	BOOL br = DeviceIoControl(m_dev,				// device to be queried
	//				IOCTL_DISK_GET_DRIVE_GEOMETRY,		// operation to perform
 //                   NULL, 0,							// no input buffer
 //                   &dg, sizeof(dg),					// output buffer
 //                   &junk,								// # bytes returned
 //                   (LPOVERLAPPED) NULL);				// synchronous I/O
	//	if (!br) THROW_WIN32_ERROR( _T("Get max lba ") );
	//	m_max_lba = dg.Cylinders.QuadPart * dg.TracksPerCylinder * dg.SectorsPerTrack;
	//	LOG_DEBUG(_T("C=%d, H=%d, S=%d, LBA=%d"), (int)dg.Cylinders.QuadPart,
	//		dg.TracksPerCylinder, dg.SectorsPerTrack, m_max_lba);
	//}
	//return m_max_lba;
//}

//FILESIZE CScsiStorageDevice::ReadCapacity(void)
//{
//	BYTE buf[512];
//	BYTE cb[16];
//	memset(cb, 0, 16);
//	cb[0] = 0x25;
//
//	ScsiCommand(read, buf, 8, cb, 10, 600);
//	FILESIZE max_lba = MAKELONG(MAKEWORD(), MAKEWORD());
//	return 
//}
