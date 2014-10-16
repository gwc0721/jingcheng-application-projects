// CoreMntDriverFile.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <stdext.h>
#include "DriverImageFile.h"
#include "../Comm/virtual_disk.h"

LOCAL_LOGGER_ENABLE(_T("CoreMntDriverFile"), LOGGER_LEVEL_DEBUGINFO);


CDriverFactory g_factory;

CDriverFactory::CDriverFactory(void)
{
	LOG_STACK_TRACE();
	//g_test+=10;
	//_tprintf(_T("dll: test=%d\n"), g_test);
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
    if(INVALID_HANDLE_VALUE == m_file) THROW_WIN32_ERROR(_T("cannot open file %s"), file_name);

    LARGE_INTEGER file_size = {0,0};
    if(!::GetFileSizeEx(m_file, &file_size))	THROW_WIN32_ERROR(_T("failure on getting file size"));

	//ULONG64 file_size_ = file_size.QuadPart / SECTOR_SIZE;
    //LARGE_INTEGER tmp_ptr;
	//tmp_ptr.QuadPart = file_size_ * SECTOR_SIZE;
	
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
	printf("\n");
}

CDriverImageFile::~CDriverImageFile(void)
{
	LOG_STACK_TRACE();
	CloseHandle(m_file);
}

bool	CDriverImageFile::Read(char * buf, ULONG64 lba, ULONG32 secs)
{
	LOG_NOTICE(_T("[ATA],RD,%08I64X,%d"), lba, secs);

	ULONG32 byte_count = secs * SECTOR_SIZE;

    DWORD bytesWritten = 0;
    LARGE_INTEGER tmpLi;
	tmpLi.QuadPart = lba * SECTOR_SIZE;
    if(!::SetFilePointerEx(m_file, tmpLi, &tmpLi, FILE_BEGIN))
		THROW_WIN32_ERROR(_T(" SetFilePointerEx failed. "));

	VENDOR_CMD_STATUS vs = VendorCmdStatus(READ, lba, secs, buf);
	if (vs < VDS_CMD)
	{	// normal command
		if (!::ReadFile(m_file, buf, byte_count, &bytesWritten, 0))
			THROW_WIN32_ERROR(_T(" ReadFile failed. "));
	}
	else
	{	// vendor command
	}

	return true;
}

bool	CDriverImageFile::Write(const char * buf, ULONG64 lba, ULONG32 secs)
{
	LOG_NOTICE(_T("[ATA],WR,%08I64X,%d"), lba, secs);

	ULONG32 byte_count = secs * SECTOR_SIZE;

    DWORD bytesWritten = 0;
    LARGE_INTEGER tmpLi;tmpLi.QuadPart = lba * SECTOR_SIZE;
    if(!::SetFilePointerEx(m_file, tmpLi, &tmpLi, FILE_BEGIN))
		THROW_WIN32_ERROR(_T(" SetFilePointerEx failed. "));


	VENDOR_CMD_STATUS vs = VendorCmdStatus(READ, lba, secs, const_cast<char*>(buf) );
	if (vs < VDS_CMD)
	{
		if (!::WriteFile(m_file, buf, byte_count, &bytesWritten, 0))
			THROW_WIN32_ERROR(_T(" ReadFile failed. "));
	}
	else
	{
	}

	return true;
}

CDriverImageFile::VENDOR_CMD_STATUS CDriverImageFile::VendorCmdStatus(CDriverImageFile::READ_WRITE rdwr, ULONG64 lba, ULONG32 secs, char * buf)
{
	switch (m_vendorcmd_st)
	{
	case VDS0:	// waiting 0x00AA
		if ( (rdwr == READ) && (secs == 1) && (lba == 0x00AA) ) m_vendorcmd_st=VDS1;
		else	m_vendorcmd_st = VDS0;
		break;
	case VDS1: // waiting 0xAA00
		if ( (rdwr == READ) && (secs == 1) && (lba == 0xAA00) ) m_vendorcmd_st=VDS2;
		else	m_vendorcmd_st = VDS0;
		break;
	case VDS2: // waiting 0x0055
		if ( (rdwr == READ) && (secs == 1) && (lba == 0x0055) ) m_vendorcmd_st=VDS3;
		else	m_vendorcmd_st = VDS0;
		break;
	case VDS3: // waiting 0x5500
		if ( (rdwr == READ) && (secs == 1) && (lba == 0x5500) ) m_vendorcmd_st=VDS4;
		else	m_vendorcmd_st = VDS0;
		break;
	case VDS4: // waiting 0x55AA
		if ( (rdwr == READ) && (secs == 1) && (lba == 0x55AA) )
		{
			memset(buf, 0, SECTOR_SIZE);
			m_vendorcmd_st=VDS_CMD;
		}
		else	m_vendorcmd_st = VDS0;
		break;
	case VDS_CMD:		{// send vender command
		if ( (rdwr == WRITE) && (secs == 1) && (lba == 0x55AA) )
		{
			TCHAR _vstr[64];
			LPTSTR vstr = _vstr;
			for (int ii=2; ii<16; ++ii)
			{
				int len = stdext::jc_sprintf(vstr, 64, _T("%02X,"), buf[ii]);
				vstr += len;
			}
			LOG_NOTICE(_T("[VND] %04X, %s"), MAKEWORD(buf[1], buf[0]), _vstr);
			m_vendorcmd_st=VDS_DATA;
		}
		else m_vendorcmd_st = VDS0;
		break;	}

	case VDS_DATA: // read/write data
		// copy data
		if ( (lba == 0x55AA) )
		{
			memset(buf, 0, secs * SECTOR_SIZE);
		}
		m_vendorcmd_st = VDS0;
		break;

	default:
		JCASSERT(0);
		break;
	}
	return m_vendorcmd_st;
}


//ULONG64	CDriverImageFile::GetSize(void) const
//{
//	return 0;
//}
