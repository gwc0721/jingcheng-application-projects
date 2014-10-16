#pragma once

#include "mntImage.h"
#include <windows.h>

class FileImage : public IImage
{
public:
    FileImage(const wchar_t * fileName, ULONG64 secs);
    ~FileImage();

	IMPLEMENT_INTERFACE;

public:
    virtual bool Read(char* buf, ULONG64 lba, ULONG32 secs);
    virtual bool Write(const char* buf, ULONG64 lba, ULONG32 secs);
	virtual ULONG64 GetSize(void) const {return m_file_size;} ;

protected:
    HANDLE m_hFile;
	ULONG64 m_file_size;	// in sector

};
