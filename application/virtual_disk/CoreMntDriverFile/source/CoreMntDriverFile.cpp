// CoreMntDriverFile.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <stdext.h>
#include "config.h"
#include "DriverImageFile.h"
#include "../../Comm/virtual_disk.h"


#include <ntddscsi.h>
#include <WinIoCtl.h>

#include <sstream>

#define STATUS_SUCCESS		0
#define STATUS_INVALID_PARAMETER         (0xC000000DL)    // winnt



LOCAL_LOGGER_ENABLE(_T("CoreMntDriverFile"), LOGGER_LEVEL_DEBUGINFO);


CDriverFactory g_factory;

CDriverFactory::CDriverFactory(void)
{
	LOG_STACK_TRACE();
}

CDriverFactory::~CDriverFactory(void)
{
	LOG_STACK_TRACE();
}

bool CDriverFactory::CreateDriver(const CJCStringT &driver_name, jcparam::IValue *param, IImage *&driver)
{
	LOG_STACK_TRACE();
	JCASSERT(driver == NULL);

	CJCStringT file_name;
	ULONG64 file_size;
	if (param)
	{
		jcparam::GetSubVal(param, _T("FILENAME"), file_name);
		LOG_DEBUG(_T("got file name: %s"), file_name.c_str());
		jcparam::GetSubVal(param, _T("FILESIZE"), file_size);
		LOG_DEBUG(_T("got file size: %I64d secs"), file_size);
	}

	driver = new CDriverImageFile(file_name, file_size);
	return true;
}

UINT CDriverFactory::GetRevision() const
{
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// --

CDriverImageFile::CDriverImageFile(const CJCStringT file_name, ULONG64 secs) 
	: m_ref(1), m_file(NULL), m_file_secs(secs)
	, m_vendorcmd_st(VDS0)
{
	LOG_STACK_TRACE();

	m_file = ::CreateFileW(file_name.c_str(), FILE_ALL_ACCESS, 0, 
		NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL );
	if(INVALID_HANDLE_VALUE == m_file) THROW_WIN32_ERROR(_T("cannot open file %s"), file_name.c_str() );

    LARGE_INTEGER file_size = {0,0};
    if(!::GetFileSizeEx(m_file, &file_size))	THROW_WIN32_ERROR(_T("failure on getting file size"));

	char buf[SECTOR_SIZE];
	memset(buf, 0, SECTOR_SIZE);
	SetFilePointerEx(m_file, file_size, NULL, FILE_BEGIN);
	DWORD written = 0;
	printf("creating img file.");
	ULONG64 total_secs = file_size.QuadPart / SECTOR_SIZE;
	for (ULONG64 ss = total_secs; ss < secs; ++ss)
	{
		if ( (ss & 0x3FFFF) == 0) printf(".");
		WriteFile(m_file, buf, SECTOR_SIZE, &written, NULL);
	}

	// initialize vendor command
	memset(m_vendor_cmd, 0, SECTOR_SIZE);
	Initialize();

	printf("\n");
}

bool CDriverImageFile::Initialize(void)
{
	LoadBinFile(FN_VENDOR_BIN, m_buf_vendor, SECTOR_SIZE);
	LoadBinFile(FN_FID_BIN, m_buf_fid, SECTOR_SIZE);
	LoadBinFile(FN_SFR_BIN, m_buf_sfr, SECTOR_SIZE);
	LoadBinFile(FN_PAR_BIN, m_buf_par, SECTOR_SIZE);
	LoadBinFile(FN_IDTAB_BIN, m_buf_idtable, SECTOR_SIZE);
	LoadBinFile(FN_DEVINFO_BIN, m_buf_device_info, SECTOR_SIZE);
	m_isp_len = LoadBinFile(FN_ISP_BIN, m_buf_isp, MAX_ISP_SEC * SECTOR_SIZE);
	LOG_DEBUG(_T("load isp bin, len=%d"), m_isp_len / SECTOR_SIZE);
	m_info_len = LoadBinFile(FN_INFO_BIN, m_buf_info, MAX_ISP_SEC * SECTOR_SIZE);
	LOG_DEBUG(_T("load info bin, len=%d"), m_info_len / SECTOR_SIZE);
	m_orgbad_len = LoadBinFile(FN_ORGBAD_BIN, m_buf_orgbad,  MAX_ISP_SEC * SECTOR_SIZE);
	LOG_DEBUG(_T("load org bad bin, len=%d"), m_orgbad_len / SECTOR_SIZE);

	LoadBinFile(FN_BOOTISP_BIN, m_buf_bootisp, BOOTISP_SIZE * SECTOR_SIZE);
	// 
	m_run_type = RUN_ISP;
	return true;
}

CDriverImageFile::~CDriverImageFile(void)
{
	LOG_STACK_TRACE();
	CloseHandle(m_file);
}

ULONG32	CDriverImageFile::Read(UCHAR * buf, ULONG64 lba, ULONG32 secs)
{
	LOG_NOTICE(_T("[ATA],RD,%08I64X,%d"), lba, secs);

	ULONG32 byte_count = secs * SECTOR_SIZE;

    DWORD bytesWritten = 0;
    LARGE_INTEGER tmpLi;
	tmpLi.QuadPart = lba * SECTOR_SIZE;
	BOOL br = ::SetFilePointerEx(m_file, tmpLi, &tmpLi, FILE_BEGIN);
    if(!br) THROW_WIN32_ERROR(_T(" SetFilePointerEx failed. "));

	bool vs = VendorCmdStatus(READ, lba, secs, buf);
	if ( !vs )
	{	// normal command
		br = ::ReadFile(m_file, buf, byte_count, &bytesWritten, 0);
		if (!br) THROW_WIN32_ERROR(_T(" ReadFile failed. "));
	}

	LOG_DEBUG(_T("data read: %02X, %02X, %02X, %02X"), buf[0], buf[1], buf[2], buf[3]);
	return STATUS_SUCCESS;
}

ULONG32	CDriverImageFile::Write(const UCHAR * buf, ULONG64 lba, ULONG32 secs)
{
	LOG_NOTICE(_T("[ATA],WR,%08I64X,%d"), lba, secs);

	ULONG32 byte_count = secs * SECTOR_SIZE;

    DWORD bytesWritten = 0;
    LARGE_INTEGER tmpLi;tmpLi.QuadPart = lba * SECTOR_SIZE;
	BOOL br = ::SetFilePointerEx(m_file, tmpLi, &tmpLi, FILE_BEGIN);
    if (!br) THROW_WIN32_ERROR(_T(" SetFilePointerEx failed. "));

	bool vs = VendorCmdStatus(WRITE, lba, secs, const_cast<UCHAR*>(buf) );
	if (!vs)
	{
		LOG_DEBUG(_T("actual write"));
		br = ::WriteFile(m_file, buf, byte_count, &bytesWritten, 0);
		if (!br) 	THROW_WIN32_ERROR(_T(" WriteFile failed. "));
	}
	return STATUS_SUCCESS;
}

ULONG32	CDriverImageFile::FlushCache(void)
{
	LOG_NOTICE(_T("[ATA],FLUSH"));
	return STATUS_SUCCESS;
}

ULONG32 CDriverImageFile::ScsiCommand(READ_WRITE rd_wr, UCHAR *buf, JCSIZE buf_len, UCHAR *cb, JCSIZE cb_length, UINT timeout)
{
	ULONG64 lba = 0;
	ULONG32 secs = 0;
	ULONG32 status = STATUS_INVALID_PARAMETER;

	switch (cb[0])
	{
	case 0x28:	// READ (10)
		if (cb_length < 10)
		{
			LOG_ERROR(_T("[ERR],READ(10), cb length too small: %d."), cb_length);
			return STATUS_INVALID_PARAMETER;
		}
		lba = MAKELONG(MAKEWORD(cb[5],cb[4]), MAKEWORD(cb[3],cb[2]) );
		secs = MAKEWORD(cb[8], cb[7]);
		LOG_NOTICE(_T("[SCSI],READ(10),%08I64X,%d"), lba, secs);
		if (secs * SECTOR_SIZE > buf_len)
		{
			LOG_ERROR(_T("[ERR],READ(10), buf size too small: %d."), buf_len);
			return STATUS_INVALID_PARAMETER;
		}
		LOG_DEBUG(_T("read data buf=0x%08X"), buf);

		status = Read(buf, lba, secs);
		break;
		
	case 0x2A: // WRITE (10)
		if (cb_length < 10)
		{
			LOG_ERROR(_T("[ERR],WRITE(10), cb length too small: %d."), cb_length);
			return STATUS_INVALID_PARAMETER;
		}
		lba = MAKELONG(MAKEWORD(cb[5],cb[4]), MAKEWORD(cb[3],cb[2]) );
		secs = MAKEWORD(cb[8], cb[7]);
		LOG_NOTICE(_T("[SCSI],WRITE(10),%08I64X,%d"), lba, secs);
		if (secs * SECTOR_SIZE > buf_len)
		{
			LOG_ERROR(_T("[ERR],WRITe(10), buf size too small: %d."), buf_len);
			return STATUS_INVALID_PARAMETER;
		}

		status = Write(buf, lba, secs);
		break;

	default:
		LOG_NOTICE(_T("[SCSI],unknow command 0x%02X"), cb[0]);
		break;
	}

	return status;
}


ULONG32 CDriverImageFile::DeviceControl(ULONG code, READ_WRITE read_write, UCHAR * buf, ULONG32 & data_size, ULONG32 buf_size)
{
	LOG_NOTICE(_T("[ATA],DeviceControl"));
	ULONG32 status = STATUS_SUCCESS;
	switch (code)
	{
	case 0x0004D014: //IOCTL_SCSI_PASS_THROUGH_DIRECT: {
		{

		LOG_DEBUG(_T("SCSI_PASS_THROUGH size: %d, data size:%d"), sizeof(SCSI_PASS_THROUGH_DIRECT), data_size);
		SCSI_PASS_THROUGH_DIRECT & sptd = *(reinterpret_cast<SCSI_PASS_THROUGH_DIRECT *>(buf));

		LOG_DEBUG(_T("buf = 0x%08X"), buf);
		status = ScsiCommand( (sptd.DataIn == SCSI_IOCTL_DATA_IN)?READ:WRITE,
			buf + sizeof(SCSI_PASS_THROUGH_DIRECT) + sptd.SenseInfoLength,
			sptd.DataTransferLength,
			sptd.Cdb,
			sptd.CdbLength,
			sptd.TimeOutValue);
		break; }

	case 0x002D1400:	{ //IOCTL_STORAGE_QUERY_PROPERTY
		STORAGE_PROPERTY_QUERY * spq = reinterpret_cast<STORAGE_PROPERTY_QUERY*>(buf);
		LOG_NOTICE(_T("[IOCTL],STORAGE_PROPERTY_QUERY,id:%d,type:%d"), spq->PropertyId, spq->QueryType)
		break; }

	default:
		LOG_DEBUG(_T("unknown code 0x%08X"), code);
		break;

	}
	return status;
}


bool CDriverImageFile::VendorCmdStatus(READ_WRITE rdwr, ULONG64 lba, ULONG32 secs, UCHAR * buf)
{
	LOG_STACK_TRACE();
	bool vendor_cmd = false;
	switch (m_vendorcmd_st)
	{
	case VDS0:	// waiting 0x00AA
		if ( (rdwr == READ) && (secs == 1) && (lba == 0x00AA) )
		{
			LOG_DEBUG(_T("vendor status 1"));
			m_vendorcmd_st=VDS1;
		}
		else	m_vendorcmd_st = VDS0;
		break;
	case VDS1: // waiting 0xAA00
		if ( (rdwr == READ) && (secs == 1) && (lba == 0xAA00) )
		{
			LOG_DEBUG(_T("vendor status 2"));
			m_vendorcmd_st=VDS2;
		}
		else	m_vendorcmd_st = VDS0;
		break;
	case VDS2: // waiting 0x0055
		if ( (rdwr == READ) && (secs == 1) && (lba == 0x0055) )
		{
			LOG_DEBUG(_T("vendor status 3"));
			m_vendorcmd_st=VDS3;
		}
		else	m_vendorcmd_st = VDS0;
		break;
	case VDS3: // waiting 0x5500
		if ( (rdwr == READ) && (secs == 1) && (lba == 0x5500) )
		{
			LOG_DEBUG(_T("vendor status 4"));
			m_vendorcmd_st=VDS4;
		}
		else	m_vendorcmd_st = VDS0;
		break;
	case VDS4: // waiting 0x55AA
		if ( (rdwr == READ) && (secs == 1) && (lba == 0x55AA) )
		{
			memcpy_s(buf, SECTOR_SIZE, m_buf_vendor, SECTOR_SIZE);
			LOG_DEBUG(_T("vendor: %02X, %02X, %02X, %02X"), buf[0], buf[1], buf[2], buf[3]);
			LOG_DEBUG(_T("vendor status cmd"));
			m_vendorcmd_st=VDS_CMD;
			vendor_cmd = true;
		}
		else	m_vendorcmd_st = VDS0;
		break;
	case VDS_CMD:		{// send vender command
		if ( (rdwr == WRITE) && (secs == 1) && (lba == 0x55AA) )
		{
			// save vendor command
			memcpy_s(m_vendor_cmd, SECTOR_SIZE, buf, SECTOR_SIZE);
			LOG_DEBUG(_T("vendor status data"));
			m_vendorcmd_st=VDS_DATA;
			vendor_cmd = true;
		}
		else m_vendorcmd_st = VDS0;
		break;	}

	case VDS_DATA: // read/write data
		// copy data
		if ( (lba == 0x55AA) )
		{
			ProcessVendorCommand(m_vendor_cmd, SECTOR_SIZE, rdwr, buf, secs);
			LOG_DEBUG(_T("vendor data: %02X, %02X, %02X, %02X"), buf[0], buf[1], buf[2], buf[3]);
			vendor_cmd = true;
		}
		m_vendorcmd_st = VDS0;
		break;

	default:
		JCASSERT(0);
		break;
	}
	return vendor_cmd;
}

#define LOG_VENDOR_CMD	{						\
		CJCStringT str;		TCHAR _tmp[16];		\
		for (int ii =2; ii<16; ++ii)	{		\
			_stprintf_s(_tmp, _T("%02X,"), vcmd[ii]);	str += _tmp;	}	\
		LOG_DEBUG(_T("[VENDOR] content: %s"), str.c_str() );	\
}

JCSIZE CDriverImageFile::ProcessVendorCommand(const UCHAR * vcmd, JCSIZE vcmd_len,  READ_WRITE rdwr, UCHAR * data, JCSIZE data_sec)
{
	LOG_STACK_TRACE();
	WORD cmd_id = MAKEWORD(vcmd[1], vcmd[0]);
	BYTE vnd_data_len = vcmd[11];

	WORD block = MAKEWORD(vcmd[3], vcmd[2]);
	WORD page = MAKEWORD(vcmd[5], vcmd[4]);

	JCSIZE processed_len = 0;		// 已经处理的长度
	switch (cmd_id)
	{
	case 0xF004:	// READ PAR
		LOG_NOTICE(_T("[VENDOR] PAR (F004)"))
		memcpy_s(data, data_sec * SECTOR_SIZE, m_buf_par, SECTOR_SIZE);
		processed_len = SECTOR_SIZE;
		break;

	case 0xF00A:	// READ FLASH DATA
	case 0xF10A:	{
		BYTE chunk = vcmd[6];
		BYTE mode = vcmd[8];
		LOG_NOTICE(_T("[VENDOR] READ FLASH DATA (%04X): b=%04X, p=%04X,k=%02X,m=%02X,len=%d")
			, cmd_id, block, page, chunk, mode, vnd_data_len);
		ReadFlashData(block, page, chunk, mode, vnd_data_len, data, data_sec);
		break;			}

	case 0xF10B: {	// WRITE_FLASH
		//BYTE chunk = vcmd[6];
		//BYTE mode = vcmd[8];
		LOG_NOTICE(_T("[VENDOR] WRITE BLOCK (F10B), block:%04X, page:%04X"), block, page);
		LOG_VENDOR_CMD
		WriteFlashData(block, page, vnd_data_len, data, data_sec);
		break;}

	case 0xF020:	// READ FLASH ID
		LOG_NOTICE(_T("[VENDOR] READ FLASH ID (F020)"))
		memcpy_s(data, data_sec * SECTOR_SIZE, m_buf_fid, SECTOR_SIZE);
		data[0x0A] = (UCHAR)(m_run_type);
		processed_len = SECTOR_SIZE;
		break;

	case 0xF026:	// READ SFR
		LOG_NOTICE(_T("[VENDOR] READ SFR (F026)"))
		memcpy_s(data, data_sec * SECTOR_SIZE, m_buf_sfr, SECTOR_SIZE);
		processed_len = SECTOR_SIZE;
		break;

	case 0xF02C:	// RESET CPU
		LOG_NOTICE(_T("[VENDOR] RESET CPU (F02C)"));
		m_run_type = RUN_ISP;
		break;

	case 0xF03F:	// READ IDENTIFY DEVICE
	case 0xF13F:
		LOG_NOTICE(_T("[VENDOR] READ IDTABLE (%04X)"), cmd_id)
		memcpy_s(data, data_sec * SECTOR_SIZE, m_buf_idtable, SECTOR_SIZE);
		processed_len = SECTOR_SIZE;
		break;

	case 0xF047:	// DEVICE INFO
		LOG_NOTICE(_T("[VENDOR] DEVIDE INFO (F047), len:%d"), vnd_data_len)
		memcpy_s(data, data_sec * SECTOR_SIZE, m_buf_device_info, SECTOR_SIZE);
		processed_len = SECTOR_SIZE;
		break;

	case 0xF127:	// DOWNLOAD MPISP
		LOG_NOTICE(_T("[VENDOR] DOWNLOAD MPISP (F127), %02X, len:%d"), vcmd[4], data_sec);
		// save mpisp to file
		if (vnd_data_len > data_sec)	LOG_WARNING(_T("vnd_data_len (%d) > data (%d)"),
			vnd_data_len, data_sec);
		if (vcmd[4] == 0x40)		SaveBinFile(FN_MPISP_LOG, data, data_sec * SECTOR_SIZE);
		else if (vcmd[4] == 0x80)	SaveBinFile(FN_PRETEST_LOG, data, data_sec * SECTOR_SIZE);
		m_run_type = RUN_MPISP;
		break;

	case 0xF139: {	// DOWNLOAD BOOTISP		
		LOG_NOTICE(_T("[VENDOR] DOWNLOAD BOOTISP (F139), len:%d"),  data_sec);
		memcpy_s(m_buf_bootisp, BOOTISP_SIZE * SECTOR_SIZE, 
			data, min(data_sec, BOOTISP_SIZE) * SECTOR_SIZE);
		SaveBinFile(FN_BOOTISP_LOG, m_buf_bootisp, BOOTISP_SIZE * SECTOR_SIZE);
		break;		 }

	case 0xF029: {	// READ BOOTISP
		LOG_NOTICE(_T("[VENDOR] READ BOOTISP (F029), len:%d"),  data_sec);
		memcpy_s(data, data_sec * SECTOR_SIZE, m_buf_bootisp, BOOTISP_SIZE * SECTOR_SIZE);
		break;	 }

	case 0xF00C:	// ERASE BLOCK
		LOG_NOTICE(_T("[VENDOR] ERASE BLOCK (F00C), block:%04X"), block);
		break;

	default:		{
		LOG_NOTICE(_T("[VENDOR] UNKNOWN VENDOR CMD (%04X)"), cmd_id);
		LOG_VENDOR_CMD
		break;		}
	}
	LOG_DEBUG(_T("[VENDOR] data: %02X, %02X, %02X, %02X,..."), data[0], data[1], data[2], data[3]);
	return processed_len;
}


void CDriverImageFile::WriteFlashData(WORD block, WORD page, BYTE read_secs, UCHAR * buf, JCSIZE secs)
{
	LOG_STACK_TRACE();
	JCSIZE offset = (page * 32) * SECTOR_SIZE;
	if (block == 3 || block ==4)
	{	// isp
		if (offset >= MAX_ISP_SEC * SECTOR_SIZE) return;
		memcpy_s(m_buf_isp + offset, MAX_ISP_SEC * SECTOR_SIZE - offset, buf, secs * SECTOR_SIZE);
	}
}

void CDriverImageFile::ReadFlashData(WORD block, WORD page, BYTE chunk, BYTE mode, BYTE read_secs, UCHAR * buf, JCSIZE secs)
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("read len=%d, buf len=%d"), read_secs, secs);
	memset(buf, 0, secs * SECTOR_SIZE);

	// 计算offset
	JCSIZE offset = (page * 32 + chunk * 2) * SECTOR_SIZE;
	if (block == 3 || block ==4)
	{	// isp block
		if (offset >= m_isp_len)
		{
			memset(buf, 0xFF, 3*SECTOR_SIZE);
			return;
		}
		memcpy_s(buf, 2 * SECTOR_SIZE, m_buf_isp + offset, 2* SECTOR_SIZE);
		memset(buf + 2* SECTOR_SIZE, 0xE4, 9);
		buf[2* SECTOR_SIZE +6] = 0x16;
		buf[2* SECTOR_SIZE +7] = 0xB4;
	}
	else if (block == 1 || block == 2)
	{	// info
		if (offset >= m_info_len)
		{
			memset(buf, 0xFF, min(3, secs)*SECTOR_SIZE);
			return;
		}
		memcpy_s(buf, secs * SECTOR_SIZE, m_buf_info + offset, min(2,secs)* SECTOR_SIZE);
		if (secs > 2)
		{
			memset(buf + 2* SECTOR_SIZE, 0xE1, 4);
			memset(buf + 2* SECTOR_SIZE + 6 , 0x55, 4);
		}
	}
	else if (block == 6)
	{	// orgbad
		if (offset >= m_orgbad_len)
		{
			memset(buf, 0xFF, min(3, secs)*SECTOR_SIZE);
			return;
		}
		memcpy_s(buf, secs * SECTOR_SIZE, m_buf_orgbad + offset, min(2,secs)* SECTOR_SIZE);
		if (secs > 2)
		{
			memset(buf + 2* SECTOR_SIZE, 0xEA, 4);
			memset(buf + 2* SECTOR_SIZE + 6 , 0x55, 4);
		}
		
	}
	else
	{	// other blocks
		char str[128];
		sprintf_s(str, "BLOCK:%04X------PAGE:%04X-------CHUNK:%02X", block, page, chunk);
		memcpy_s(buf, 2*SECTOR_SIZE, str, strlen(str));
	}
}

JCSIZE CDriverImageFile::LoadBinFile(const CJCStringT & fn, UCHAR * buf, JCSIZE buf_len)
{
	FILE * file = NULL;
	stdext::jc_fopen(&file, fn.c_str(), _T("rb"));
	if (file == NULL) THROW_ERROR(ERR_PARAMETER, _T("failure on openning file %s"), fn.c_str());
	JCSIZE read = fread(buf, 1, buf_len, file);
	fclose(file);
	return read;
}

void CDriverImageFile::SaveBinFile(const CJCStringT & fn, UCHAR * buf, JCSIZE buf_len)
{
	FILE * file = NULL;
	stdext::jc_fopen(&file, fn.c_str(), _T("w+b"));
	if (file == NULL) THROW_ERROR(ERR_PARAMETER, _T("failure on openning file %s"), fn.c_str());
	JCSIZE written = fwrite(buf, 1, buf_len, file);
	fclose(file);
	//return read;
}
