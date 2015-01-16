#include "stdafx.h"
#include "storage_device_comm.h"
#include <ntddscsi.h>

LOCAL_LOGGER_ENABLE(_T("CStorageDeviceComm"), LOCAL_LOG_LEV); 

#define CMD_BLOCK_SIZE	16

CStorageDeviceComm::CStorageDeviceComm(HANDLE dev)
	: m_dev(dev), m_capacity(0)
	, m_last_invoke_time(0)
{
	LOG_STACK_TRACE();
	if (!m_dev || INVALID_HANDLE_VALUE == m_dev ) THROW_ERROR(ERR_APP, _T("invalid handle") );
#ifdef _OVER_WRITE_CHECK
	m_sec_0 = NULL;
	m_sec_55aa = NULL;
	m_sec_0 = new BYTE[SECTOR_SIZE];
	m_sec_55aa = new BYTE[SECTOR_SIZE];
	try
	{
		SectorRead(m_sec_0, 0, 1);
		SectorRead(m_sec_55aa, 0x55AA, 1);
	}
	catch (std::exception &)
	{
		delete [] m_sec_0;
		delete [] m_sec_55aa;
		m_sec_0 = NULL;
		m_sec_55aa = NULL;
	}
#endif
}

CStorageDeviceComm::~CStorageDeviceComm(void)
{
	LOG_STACK_TRACE();
	std::vector<HANDLE>::iterator it = m_logical_drives.begin();
	std::vector<HANDLE>::iterator endit = m_logical_drives.end();
	for ( ; it != endit; ++ it)		CloseHandle(*it);

	if (m_dev)
	{
		DeviceUnlock();
		CloseHandle(m_dev);
	}

#ifdef _OVER_WRITE_CHECK
	delete [] m_sec_0;
	delete [] m_sec_55aa;
#endif
}

void CStorageDeviceComm::Detach(HANDLE & dev)
{
	dev = m_dev;
	m_dev = NULL;
}


bool CStorageDeviceComm::ScsiCommand(READWRITE rd_wr, BYTE *buf, JCSIZE buf_len, BYTE *cb, JCSIZE cb_length, UINT timeout)
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
	sptdwb.sptd.DataIn = (rd_wr == read)?SCSI_IOCTL_DATA_IN:SCSI_IOCTL_DATA_OUT;
    sptdwb.sptd.DataTransferLength = buf_len;
    sptdwb.sptd.TimeOutValue = timeout;
    sptdwb.sptd.DataBuffer = buf;
	// get offset
	sptdwb.sptd.SenseInfoOffset = (ULONG)offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
	memcpy(sptdwb.sptd.Cdb, cb, cb_length < CDB10GENERIC_LENGTH ? cb_length : CDB10GENERIC_LENGTH);
	
    ULONG llength = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
    ULONG	returned = 0;
    BOOL	success;

	LARGE_INTEGER t0, t1;		// 性能计算
	QueryPerformanceCounter(&t0);
	success = DeviceIoControl(
				   m_dev,
				   IOCTL_SCSI_PASS_THROUGH_DIRECT,
				   &sptdwb,
				   llength,
				   &sptdwb,
				   llength,
				   &returned,
				   FALSE );
	QueryPerformanceCounter(&t1);		// 性能计算
	if (t0.QuadPart <= t1.QuadPart)		m_last_invoke_time = t1.QuadPart - t0.QuadPart;
	else								m_last_invoke_time = 0;

	if (!success) THROW_WIN32_ERROR( _T("send scsi command failed: ") );
    FlushFileBuffers(m_dev);
	return true;
}

bool CStorageDeviceComm::DeviceLock(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dev);

	DWORD junk = 0;
    BOOL br;
  
    // 用FSCTL_LOCK_VOLUME锁卷
    br = ::DeviceIoControl(m_dev,        // 设备句柄
        FSCTL_LOCK_VOLUME,                    // 锁卷
        NULL, 0,                              // 不需要输入数据
        NULL, 0,                              // 不需要输出数据
        &junk,                          // 输出数据长度
        (LPOVERLAPPED)NULL);                  // 用同步I/O

	if (!br) LOG_ERROR(_T("Lock volume failed. Error id = %d"), GetLastError())
    return (br != FALSE);
}

bool CStorageDeviceComm::DeviceUnlock(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dev);

	DWORD junk = 0;
    BOOL br;
  
    // 用FSCTL_LOCK_VOLUME锁卷
    br = ::DeviceIoControl(m_dev,        // 设备句柄
        FSCTL_UNLOCK_VOLUME,                    // 锁卷
        NULL, 0,                              // 不需要输入数据
        NULL, 0,                              // 不需要输出数据
        &junk,                          // 输出数据长度
        (LPOVERLAPPED)NULL);                  // 用同步I/O

	if (!br) LOG_ERROR(_T("Unlock volume failed. Error id = %d"), GetLastError())
    return (br != FALSE);
}

bool CStorageDeviceComm::Dismount(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dev);
	DWORD junk = 0;
	BOOL br = DeviceIoControl( m_dev, FSCTL_DISMOUNT_VOLUME,
		NULL, 0, NULL, 0, &junk, NULL);

	if (!br) LOG_ERROR(_T("Dismount volume failed. Error id = %d"), GetLastError())

	return (br != FALSE);
}

bool CStorageDeviceComm::SectorRead(BYTE * buf, FILESIZE lba, JCSIZE sectors)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dev);
	JCASSERT(buf);

	DWORD data_size = sectors * SECTOR_SIZE;

#if 1
	// read sector using ReadFile api
	LARGE_INTEGER offset, new_pointer;
	offset.QuadPart = lba * SECTOR_SIZE;
	SetFilePointerEx(m_dev, offset, &new_pointer, FILE_BEGIN);
	DWORD read_size = 0;

	LARGE_INTEGER t0, t1;		// 性能计算
	QueryPerformanceCounter(&t0);
	BOOL br = ReadFile(m_dev, buf, data_size, &read_size, NULL);
	QueryPerformanceCounter(&t1);		// 性能计算
	if (t0.QuadPart <= t1.QuadPart)		m_last_invoke_time = t1.QuadPart - t0.QuadPart;
	else								m_last_invoke_time = 0;

	if (!br) THROW_WIN32_ERROR(_T("Reading LBA failed! LBA = %u, size = %d"), (UINT)lba, sectors);
#else
	// read sector using SCSI driver
	BYTE cb[10];
	memset(cb, 0, 10);
	cb[0] = 0x28;
	cb[2] = HIBYTE(HIWORD(lba));
	cb[3] = LOBYTE(HIWORD(lba));
	cb[4] = HIBYTE(LOWORD(lba));
	cb[5] = LOBYTE(LOWORD(lba));
	cb[7] = HIBYTE(sectors);
	cb[8] = LOBYTE(sectors);
	
	ScsiCommand(read, buf, data_size, cb, 10);
#endif
	return true;
}

bool CStorageDeviceComm::SectorWrite(const BYTE * buf, FILESIZE lba, JCSIZE sectors)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dev);
	JCASSERT(buf);

	LARGE_INTEGER offset, new_pointer;
	offset.QuadPart = lba * SECTOR_SIZE;
	SetFilePointerEx(m_dev, offset, &new_pointer, FILE_BEGIN);
	DWORD data_size = sectors * SECTOR_SIZE;
	DWORD written_size = 0;
	LARGE_INTEGER t0, t1;		// 性能计算
	QueryPerformanceCounter(&t0);
	BOOL br = WriteFile(m_dev, buf, data_size, &written_size, NULL);
	QueryPerformanceCounter(&t1);		// 性能计算
	if (t0.QuadPart <= t1.QuadPart)		m_last_invoke_time = t1.QuadPart - t0.QuadPart;
	else								m_last_invoke_time = 0;
	if (!br)		THROW_WIN32_ERROR(_T("Writing LBA failed! LBA = %u, size = %d"), (UINT)lba, sectors);
	return true;
}

bool CStorageDeviceComm::Inquiry(BYTE * buf, JCSIZE buf_len)
{
	LOG_STACK_TRACE();
	JCASSERT(buf);
	JCASSERT(m_dev);

	BYTE cb[16];
	memset(cb, 0, 16);
	cb[0] = 0x12;
	cb[4] = 0x24;

	if (buf_len > 0x24) buf_len = 0x24;
	ScsiCommand(read, buf, buf_len, cb, 16, 300);
	return true;
}


void CStorageDeviceComm::UnmountAllLogical(void)
{
	LOG_STACK_TRACE();

	static const TCHAR PHYSICAL_DRIVE[] = _T("PhysicalDrive");
	static const JCSIZE PHDRV_LEN = _tcslen(PHYSICAL_DRIVE);
	TCHAR lgdrv_name[] = _T("\\\\.\\x:");

	// Check physical drive number
	LPCTSTR str_drv_no = _tcsstr(m_device_name.c_str(), PHYSICAL_DRIVE);
	if (NULL == str_drv_no)
	{	// If it is a logical drive, just dismount it.
		LOG_ERROR(_T("Current drive is logical: %s"), m_device_name.c_str())
		Dismount();
		return;
	}

	int phdrv_no = str_drv_no[PHDRV_LEN] - _T('0');
	LOG_DEBUG(_T("Dismount all logical drive for physical drive %d"), phdrv_no)

	DWORD logical_drives = GetLogicalDrives();
	
	TCHAR drive_letter = _T('A');
	for (DWORD mask = 1; mask < 0x10000000; mask <<= 1, ++drive_letter)
	{
		if ( !(mask & logical_drives) ) continue;
		lgdrv_name[4] = drive_letter;
		LOG_DEBUG(_T("Check logical drive %c:"), drive_letter)

		HANDLE hlgdrv = CreateFile(lgdrv_name, 
						GENERIC_READ|GENERIC_WRITE, 
			  			FILE_SHARE_READ|FILE_SHARE_WRITE, 
						NULL, 
						OPEN_EXISTING, 
						0 | FILE_FLAG_NO_BUFFERING, 
						NULL );
		if( INVALID_HANDLE_VALUE == hlgdrv)
		{
			LOG_ERROR(_T("Openning logical drive %c: failed"), drive_letter);
			continue;
		}
		stdext::auto_handle<HANDLE> logic_drv(hlgdrv);

		VOLUME_DISK_EXTENTS		ext;
		DWORD read_size = 0;
		BOOL br = DeviceIoControl(logic_drv, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0,
			&ext, sizeof(ext), &read_size, NULL);
		//if (!br) THROW_WIN32_ERROR(_T("Getting disk extents %c:"), drive_letter );
		if (!br)
		{
			LOG_ERROR(_T("Failure on getting disk extents of %c:"), drive_letter);
			continue;
		}
		if (ext.NumberOfDiskExtents <= 0) continue;
		if (phdrv_no == ext.Extents[0].DiskNumber)
		{
			LOG_DEBUG(_T("Logical drive %c: is a partition of physicla drive %d"), drive_letter, phdrv_no)
			br = DeviceIoControl( logic_drv, FSCTL_DISMOUNT_VOLUME,
				NULL, 0, NULL, 0, &read_size, NULL);
			logic_drv.detatch(hlgdrv);
			m_logical_drives.push_back(hlgdrv);
		}
	}
}

bool CStorageDeviceComm::ScsiRead(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout)
{
	// read sector using SCSI driver
	LOG_STACK_TRACE();
	JCASSERT(m_dev);
	JCASSERT(buf);

	DWORD data_size = secs * SECTOR_SIZE;
	BYTE cb[10];
	memset(cb, 0, 10);
	cb[0] = 0x28;
	cb[2] = HIBYTE(HIWORD(lba));
	cb[3] = LOBYTE(HIWORD(lba));
	cb[4] = HIBYTE(LOWORD(lba));
	cb[5] = LOBYTE(LOWORD(lba));
	cb[7] = HIBYTE(secs);
	cb[8] = LOBYTE(secs);
	
	return ScsiCommand(read, buf, data_size, cb, 10, timeout);
}


bool CStorageDeviceComm::ScsiWrite(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dev);
	JCASSERT(buf);

	DWORD data_size = secs * SECTOR_SIZE;
	BYTE cb[10];
	memset(cb, 0, 10);
	cb[0] = 0x2A;
	cb[2] = HIBYTE(HIWORD(lba));
	cb[3] = LOBYTE(HIWORD(lba));
	cb[4] = HIBYTE(LOWORD(lba));
	cb[5] = LOBYTE(LOWORD(lba));
	cb[7] = HIBYTE(secs);
	cb[8] = LOBYTE(secs);
	
	return ScsiCommand(write, buf, data_size, cb, 10, timeout);
}

FILESIZE CStorageDeviceComm::GetCapacity(void)
{
	LOG_STACK_TRACE();
	if (0 == m_capacity)
	{
		BYTE buf[12];
		BYTE cb[16];
		memset(cb, 0, 16);
		cb[0] = 0x25;

		ScsiCommand(read, buf, 8, cb, 10, 600);
		LOG_DEBUG(_T("%02X, %02X, %02X, %02X, "), buf[0], buf[1], buf[2], buf[3]);
		m_capacity = MAKELONG(MAKEWORD(buf[3], buf[2]), MAKEWORD(buf[1], buf[0]));
		++ m_capacity;
	}
	return m_capacity;
}

UINT CStorageDeviceComm::GetLastInvokeTime(void)
{
	return ( (UINT)(m_last_invoke_time / CJCLogger::Instance()->GetTimeStampCycle()) ); 
}

bool CStorageDeviceComm::StartStopUnit(bool stop)
{
	BYTE cmd_block[CMD_BLOCK_SIZE];
	stdext::auto_array<BYTE>	_buf(SECTOR_SIZE);

	bool br;
	if (stop)
	{	// power off
		memset(cmd_block, 0, sizeof(BYTE) * CMD_BLOCK_SIZE);
		cmd_block[0] = 0x1B, cmd_block[1] = 0, cmd_block[4] = 0x02;
		br = ScsiCommand(read, _buf, 0, cmd_block, CMD_BLOCK_SIZE, 60);
	}
	else
	{	// power on
		memset(cmd_block, 0, sizeof(BYTE) * CMD_BLOCK_SIZE);
		cmd_block[0] = 0x1B, cmd_block[1] = 0, cmd_block[4] = 0;
		br = ScsiCommand(read, _buf, 0, cmd_block, CMD_BLOCK_SIZE, 60);
		if (!br) return false;

		memset(cmd_block, 0, sizeof(BYTE) * CMD_BLOCK_SIZE);
		cmd_block[0] = 0x0, cmd_block[1] = 0, cmd_block[4] = 0;
		br = ScsiCommand(read, _buf, 0, cmd_block, CMD_BLOCK_SIZE, 60);
	}
	return br;
}

/*
void CStorageDeviceComm::GetTimeOut(UINT &rd, UINT &wr)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dev);

	COMMTIMEOUTS time_out;
	memset(&time_out, 0, sizeof(COMMTIMEOUTS) );
	BOOL br = GetCommTimeouts(m_dev, &time_out);
	if (!br) THROW_WIN32_ERROR(_T("Get time out failed."));
	LOG_DEBUG(_T("RI = %d, RM = %d, RC = %d, WM = %d, WC = %d"), 
		time_out.ReadIntervalTimeout, time_out.ReadTotalTimeoutMultiplier,
		time_out.ReadTotalTimeoutConstant, time_out.WriteTotalTimeoutMultiplier, 
		time_out.WriteTotalTimeoutConstant);

	rd = time_out.ReadIntervalTimeout;
	wr = time_out.WriteTotalTimeoutConstant;
}


void CStorageDeviceComm::SetTimeOut(UINT rd, UINT wr)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dev);

	COMMTIMEOUTS time_out;
	memset(&time_out, 0, sizeof(COMMTIMEOUTS) );
	BOOL br = GetCommTimeouts(m_dev, &time_out);
	if (!br) THROW_WIN32_ERROR(_T("Get time out failed."));
	LOG_DEBUG(_T("RI = %d, RM = %d, RC = %d, WM = %d, WC = %d"), 
		time_out.ReadIntervalTimeout, time_out.ReadTotalTimeoutMultiplier,
		time_out.ReadTotalTimeoutConstant, time_out.WriteTotalTimeoutMultiplier, 
		time_out.WriteTotalTimeoutConstant);

	time_out.ReadIntervalTimeout = rd;
	time_out.WriteTotalTimeoutConstant = wr;
	br = SetCommTimeouts(m_dev, &time_out);
	if (!br) THROW_WIN32_ERROR(_T("Set time out failed."));
}
*/

#ifdef _OVER_WRITE_CHECK
bool CStorageDeviceComm::SectorVerify(void)
{
	bool br = true;
	BYTE buf[SECTOR_SIZE];
	if (m_sec_0)
	{
		SectorRead(buf, 0, 1);
		if ( memcmp(buf, m_sec_0, SECTOR_SIZE) != 0)
		{
			br = false;
			LOG_ERROR(_T("Verify sector 0 failed")); 
		}
	}

	if (m_sec_55aa)
	{
		SectorRead(buf, 0x55AA, 1);
		if ( memcmp(buf, m_sec_55aa, SECTOR_SIZE) != 0)
		{
			br = false;
			LOG_ERROR(_T("Verify sector 0x55AA failed")); 
		}
	}

	return br;
}
#endif