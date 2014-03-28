
#include "stdafx.h"
#include <stdext.h>
#include "config.h"

#include <windows.h> 
#include <winioctl.h>
#include <ntddscsi.h>

LOCAL_LOGGER_ENABLE(_T("update_fw"), LOGGER_LEVEL_DEBUGINFO);

#define SPT_SENSE_LENGTH	32
#define SPTWB_DATA_LENGTH	4*512
#define CDB6GENERIC_LENGTH	6
#define CDB10GENERIC_LENGTH	10
#define CDB16GENERIC_LENGTH	16
#define Option_Read         0
#define Option_Write        1


typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS 
{
    SCSI_PASS_THROUGH spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
    UCHAR             ucDataBuf[SPTWB_DATA_LENGTH];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER 
{
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[SPT_SENSE_LENGTH];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

// return true means win7, else means win xp 
bool CheckWinVista_7OS()
{
	LOG_STACK_TRACE();

    OSVERSIONINFO   WinVerInfo;

	bool win7 = false;

	ZeroMemory(&WinVerInfo, sizeof(OSVERSIONINFO));
	WinVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	bool success = (GetVersionEx((OSVERSIONINFO*) &WinVerInfo) != FALSE);
    if(!success)	THROW_WIN32_ERROR(_T("failure on getting windows ver."));

	if(WinVerInfo.dwMajorVersion==6)
	{
		LOG_DEBUG(_T("windows ver = win7 or vista"));
		win7 = true;
	}
	else
	{
		LOG_DEBUG(_T("windows ver = win xp"));
		win7 = false;
	}
	return win7;
}

/*******************************************************************************
 *
 * FUNCTION:    SMIReadWriteSector
 *
 * PARAMETERS:  Option            - 
 *              TargetID          -  
 *              hDevice           - Device handle
 *              SectorNumber      - Which sector to access
 *              DataBuffer        - 
 *              BufferSize        - 
 *              Command           - 0x28 Read
                                    0x2A Write
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Read data from specific sector
 *
 ******************************************************************************/
bool SMIReadWriteSector(UINT Option, BYTE TargetID, HANDLE hDevice,
		ULONG SectorNumber, PUCHAR DataBuffer, ULONG BufferSize, UCHAR Command) 
{
	LOG_STACK_TRACE();
	
	JCASSERT( hDevice && (INVALID_HANDLE_VALUE != hDevice) );

	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
    ULONG   length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
	ULONG	returned = 0;
	bool	success;

	ZeroMemory(&sptdwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));

	if(Option == 0)	sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_IN;
	else			sptdwb.sptd.DataIn = SCSI_IOCTL_DATA_OUT;

	//Initial scsi passthru parameter
    sptdwb.sptd.Length= sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 1;
    sptdwb.sptd.Lun = 0;
	
	if ( CheckWinVista_7OS() )	sptdwb.sptd.CdbLength = CDB10GENERIC_LENGTH;	// win 7
	else						sptdwb.sptd.CdbLength = CDB16GENERIC_LENGTH;	// win xp
	sptdwb.sptd.SenseInfoLength = 0x00;
    sptdwb.sptd.DataTransferLength = BufferSize;
    sptdwb.sptd.TimeOutValue = 360;
    sptdwb.sptd.DataBuffer = DataBuffer;
    sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
    sptdwb.sptd.Cdb[0] = Command;
    sptdwb.sptd.Cdb[2] = HIBYTE(HIWORD(SectorNumber));
	sptdwb.sptd.Cdb[3] = LOBYTE(HIWORD(SectorNumber));
    sptdwb.sptd.Cdb[4] = HIBYTE(LOWORD(SectorNumber));
	sptdwb.sptd.Cdb[5] = LOBYTE(LOWORD(SectorNumber));
	sptdwb.sptd.Cdb[7] = HIBYTE(BufferSize/512);
	sptdwb.sptd.Cdb[8] = LOBYTE(BufferSize/512);

	success = (DeviceIoControl( hDevice, IOCTL_SCSI_PASS_THROUGH_DIRECT,
		&sptdwb, length, &sptdwb, length,
		&returned, FALSE ) != FALSE);
    return success;
}

bool SetAutoRun(void)
{
	LOG_STACK_TRACE();

	TCHAR str[BUF_SIZE];
	GetModuleFileName(NULL, str, BUF_SIZE);

	LOG_DEBUG(_T("module name: %s"), str);

	CString		strPath;
	strPath.Format(_T("%s /d"), str);

	HKEY		hRegKey;   

	if( RegOpenKey( HKEY_LOCAL_MACHINE, KEY_AUTORUN, &hRegKey) != ERROR_SUCCESS)
		THROW_WIN32_ERROR(_T("failure on openning regist key %s"), KEY_AUTORUN);
	
	LOG_DEBUG(_T("value name: %s"), VAL_NAME_AUTORUN);

	LPCTSTR str_val = strPath;
	LOG_DEBUG(_T("value data: %s"), str_val);

	JCSIZE value_len = strPath.GetLength();
	LOG_DEBUG(_T("value len: %d"), value_len); 

	if( ::RegSetValueEx(hRegKey, VAL_NAME_AUTORUN, 0, REG_SZ,   
				(const BYTE *)(str_val), value_len * sizeof(TCHAR) )   !=   ERROR_SUCCESS)   
	{
		THROW_WIN32_ERROR(_T("failure on setting regist value %s to subkey %s"), strPath, VAL_NAME_AUTORUN);
	}
	RegCloseKey(hRegKey);
	LOG_NOTICE(_T("auto run registed."))
	return true;  
}

int LogicalToPhysical(LPCTSTR szDrive)
{
	LOG_STACK_TRACE();

	STORAGE_DEVICE_NUMBER sd;
	DWORD dwRet;
	CString szPhysical;
	HANDLE h = CreateFile(szDrive, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);

	if(INVALID_HANDLE_VALUE != h)
	{
		if(DeviceIoControl(h, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sd, sizeof(STORAGE_DEVICE_NUMBER), &dwRet, NULL))
		{
			szPhysical.Format(_T("\\\\.\\PhysicalDrive%d"), sd.DeviceNumber);
		}
		CloseHandle(h);
	}
	LOG_NOTICE(_T("logical: %s, physical: %d"), szDrive, sd.DeviceNumber);
	return sd.DeviceNumber;
}

bool OpenDevice(HANDLE & disk)
{
	LOG_STACK_TRACE();
	JCASSERT( NULL == disk || INVALID_HANDLE_VALUE == disk);

	int disk_num = LogicalToPhysical(_T("\\\\.\\\\C:"));
	CString disk_name;
	disk_name.Format(_T("\\\\.\\PhysicalDrive%d"), disk_num);
	int retry = RETRY_TIMES;
	while ( (retry > 0) && (disk == INVALID_HANDLE_VALUE))
	{
		disk = CreateFile(	
			      disk_name, 
				  GENERIC_READ|GENERIC_WRITE, 
				  FILE_SHARE_READ|FILE_SHARE_WRITE, 
				  NULL, 
				  OPEN_EXISTING, 
				  0, 
				  NULL );
		--retry;
	}
	if (INVALID_HANDLE_VALUE == disk) THROW_WIN32_ERROR(_T("failure on openning disk %s"), disk_name);	
	return true;
}


bool BackupMBR(HANDLE disk, BYTE * read_buf, JCSIZE mbr_size)
{
	LOG_STACK_TRACE();

	JCASSERT( disk && (INVALID_HANDLE_VALUE != disk) );
// read mbr
	bool br = SMIReadWriteSector(Option_Read, NULL, disk, 0, read_buf, mbr_size, READ_CMD);
	if (!br) THROW_ERROR(ERR_APP, _T("failure on reading mbr"));

	FILE * file_mbr_back = NULL;
	stdext::jc_fopen(& file_mbr_back, MBR_BACKUP, _T("w+b"));
	if (NULL == file_mbr_back) THROW_ERROR(ERR_USER, _T("failure on creating file %s"), MBR_BACKUP);
	fwrite(read_buf, 1, mbr_size, file_mbr_back);
	fclose(file_mbr_back);
	file_mbr_back = NULL;

	// verify
	stdext::auto_array<BYTE> ver_buf(mbr_size);
	stdext::jc_fopen(& file_mbr_back, MBR_BACKUP, _T("rb"));
	if (NULL == file_mbr_back) THROW_ERROR(ERR_USER, _T("failure on creating file %s"), MBR_BACKUP);
	fread(ver_buf, 1, mbr_size, file_mbr_back);
	fclose(file_mbr_back);
	if (memcmp(read_buf, ver_buf, mbr_size) != 0) THROW_ERROR(ERR_APP, _T("failure on verify backup file."));

	LOG_NOTICE(_T("backup mbr succeded."));
	return true;
}

JCSIZE ReadResource(UINT resource_id, BYTE * & res)
{
	LOG_STACK_TRACE();

	JCASSERT(NULL == res);
	HMODULE hmodule = GetModuleHandle(NULL);
	if (NULL == hmodule) THROW_WIN32_ERROR(_T("failure on getting module"));
	HRSRC hresource = FindResource(hmodule, MAKEINTRESOURCE(resource_id), _T("BINARY"));
	if (NULL == hresource) THROW_WIN32_ERROR(_T("failure on openning resource %d"), resource_id);
	HGLOBAL hg = LoadResource(hmodule, hresource);
	if (NULL == hg) THROW_WIN32_ERROR(_T("failure on loading resource"));
	res = (BYTE*)(LockResource(hg));
	if (NULL == res) THROW_WIN32_ERROR(_T("failure on locking resource"));
	JCSIZE size = SizeofResource(hmodule, hresource);
	return size;
}


bool ExportFile(UINT resource_id, LPCTSTR file_name)
{
	LOG_STACK_TRACE();

	BYTE * res = NULL;
	JCSIZE size = ReadResource(resource_id, res);

	FILE * file = NULL;
	stdext::jc_fopen(&file, file_name, _T("w+b"));
	if (NULL == file) THROW_ERROR(ERR_USER, _T("failure on openning file %s"), file_name);
	fwrite(res, 1, size, file);
	fclose(file);
	LOG_NOTICE(_T("save resource %d to %s, size=%d"), resource_id, file_name, size);
	return true;
}

bool WriteGrldrMbr(HANDLE disk, BYTE* gr_buf, JCSIZE mbr_size)
{
	LOG_STACK_TRACE();

	JCASSERT( disk && (INVALID_HANDLE_VALUE != disk) );

	stdext::auto_array<BYTE> read_buf(mbr_size);
	bool br = SMIReadWriteSector(Option_Write, NULL, disk, 0, gr_buf, mbr_size, WRITE_CMD);
	if (!br) THROW_ERROR(ERR_APP, _T("failure on writing grldr to mbr."));

	stdext::auto_array<BYTE> ver_buf(mbr_size);
	br = SMIReadWriteSector(Option_Read, NULL, disk, 0, ver_buf, mbr_size, READ_CMD);
	if (!br) THROW_ERROR(ERR_APP, _T("failure on reading mbr."));
	if (memcmp(gr_buf, ver_buf, mbr_size) != 0) THROW_ERROR(ERR_APP, _T("failure on verify mbr."));
	
	LOG_NOTICE(_T("write grldr to mbr succeded."));
	return true;
}

bool Reboot(void)
{
	LOG_STACK_TRACE();

	HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tkp;

	OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,&tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1; 
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,(PTOKEN_PRIVILEGES)NULL, 0);
	ExitWindowsEx(EWX_FORCE | EWX_REBOOT,0);
	return true;
}

bool DeleteTempFiles(void)
{
	LOG_STACK_TRACE();
	// delete files
	DeleteFile(MBR_BACKUP);
	DeleteFile(FN_DOS_IMAGE);
	DeleteFile(FN_GRLDR);
	DeleteFile(FN_MENU_LST);
	DeleteFile(FN_GRLDR_MBR);
	LOG_NOTICE(_T("file removed"));
	// remove auto run

	HKEY		hRegKey;   

	if( RegOpenKey( HKEY_LOCAL_MACHINE, KEY_AUTORUN, &hRegKey) != ERROR_SUCCESS)
	{
		LOG_ERROR(_T("failure on openning reg key %s"), KEY_AUTORUN);
		return false;
	}
	//_tsplitpath_s(strPath.GetBuffer(0), NULL, 0, NULL, 0, VAL_NAME_AUTORUN, BUF_SIZE, NULL, 0);
	if( ::RegDeleteValue(hRegKey, VAL_NAME_AUTORUN) !=   ERROR_SUCCESS)   
	{
		LOG_ERROR(_T("failure on removing regist value %s"), VAL_NAME_AUTORUN);
		return false;
	}
	RegCloseKey(hRegKey);
	LOG_NOTICE(_T("auto run removed."))
	return true;
}

bool RestoreMBR(HANDLE disk)
{
	LOG_STACK_TRACE();
	JCASSERT( disk && (INVALID_HANDLE_VALUE != disk) );

	FILE *file = NULL;
	stdext::jc_fopen(&file, MBR_BACKUP, _T("rb"));
	if ( NULL == file) THROW_ERROR(ERR_USER, _T("failure on openning mbr backup file %s"), MBR_BACKUP);

	stdext::auto_array<BYTE> buf(MAX_MBR_SIZE);
	JCSIZE read_size = fread(buf, 1, MAX_MBR_SIZE, file);
	LOG_NOTICE(_T("read %d bytes from backup file"), read_size);
	fclose(file);

	bool br = SMIReadWriteSector(Option_Write, NULL, disk, 0, buf, read_size, WRITE_CMD);
	if (!br) THROW_ERROR(ERR_APP, _T("failure on restoring mbr."));

	// verify
	stdext::auto_array<BYTE> ver_buf(read_size);
	br = SMIReadWriteSector(Option_Read, NULL, disk, 0, ver_buf, read_size, READ_CMD);
	if (!br) THROW_ERROR(ERR_APP, _T("failure on reading mbr."));

	if (memcmp(buf, ver_buf, read_size) != 0) THROW_ERROR(ERR_APP, _T("failure on verify mbr."));
	LOG_NOTICE(_T("restore mbr succeded."));
	return true;
}