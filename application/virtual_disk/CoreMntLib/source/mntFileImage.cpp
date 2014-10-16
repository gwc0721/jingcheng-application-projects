#include "stdafx.h"

LOCAL_LOGGER_ENABLE(_T("FileImage"), LOGGER_LEVEL_DEBUGINFO);

#include "../include/mntFileImage.h"
#include "../../Comm/virtual_disk.h"

FileImage::FileImage(const wchar_t * fileName, ULONG64 secs)
	: m_file_size(secs), m_hFile(NULL), m_ref(1)
{
    m_hFile = ::CreateFileW(fileName, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
        0, NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL );
    if(INVALID_HANDLE_VALUE == m_hFile) THROW_WIN32_ERROR(_T("cannot open file %s"), fileName);

    LARGE_INTEGER fileSize = {0,0};
    if(!::GetFileSizeEx(m_hFile, &fileSize))	THROW_WIN32_ERROR(_T("failure on getting file size"));

	ULONG64 file_size = fileSize.QuadPart / SECTOR_SIZE;
    LARGE_INTEGER tmp_ptr;
	tmp_ptr.QuadPart = file_size * SECTOR_SIZE;
	
	char buf[SECTOR_SIZE];
	memset(buf, 0, SECTOR_SIZE);
	SetFilePointerEx(m_hFile, tmp_ptr, &tmp_ptr, FILE_BEGIN);
	DWORD written = 0;
	printf("creating img file.");
	for (ULONG64 ss = file_size; ss < secs; ++ss)
	{
		if ( (ss & 0x3FFFF) == 0) printf(".");
		WriteFile(m_hFile, buf, SECTOR_SIZE, &written, NULL);
	}
	printf("\n");
}

FileImage::~FileImage()
{
    ::CloseHandle(m_hFile);
}

bool FileImage::Read(char* buf, ULONG64 lba, ULONG32 secs)
{
	//ULONG64 offset = lba * SECTOR_SIZE;
	ULONG32 byte_count = secs * SECTOR_SIZE;

    DWORD bytesWritten = 0;
    LARGE_INTEGER tmpLi;
	tmpLi.QuadPart = lba * SECTOR_SIZE;
    if(!::SetFilePointerEx(m_hFile, tmpLi, &tmpLi, FILE_BEGIN))
		THROW_WIN32_ERROR(_T(" SetFilePointerEx failed. "));

    if (!::ReadFile(m_hFile, buf, byte_count, &bytesWritten, 0))
		THROW_WIN32_ERROR(_T(" ReadFile failed. "));
	return true;
}


bool FileImage::Write(const char* buf, ULONG64 lba, ULONG32 secs)
{
	//ULONG64 offset = lba * SECTOR_SIZE;
	ULONG32 byte_count = secs * SECTOR_SIZE;

    DWORD bytesWritten = 0;
    LARGE_INTEGER tmpLi;tmpLi.QuadPart = lba * SECTOR_SIZE;
    if(!::SetFilePointerEx(m_hFile, tmpLi, &tmpLi, FILE_BEGIN))
		THROW_WIN32_ERROR(_T(" SetFilePointerEx failed. "));

	if (!::WriteFile(m_hFile, buf, byte_count, &bytesWritten, 0))
		THROW_WIN32_ERROR(_T(" ReadFile failed. "));

	return true;
}

//ULONG64 FileImage::GetSize() const 
//{
//    LARGE_INTEGER fileSize = {0,0};
//    if(!::GetFileSizeEx(m_hFile, &fileSize))
//        throw std::exception("GetFileSizeEx failed.");
//    return fileSize.QuadPart;
//}