#pragma once

#include <stdext.h>
#include <jcparam.h>
#include "../../Comm/virtual_disk.h"

class IImage : public IJCInterface
{
public:
	virtual ULONG32	Read(UCHAR * buf, ULONG64 lba, ULONG32 secs) = 0;
	virtual ULONG32	Write(const UCHAR * buf, ULONG64 lba, ULONG32 secs) = 0;
	virtual ULONG32	FlushCache(void) = 0;
	// data size is in byte
	virtual ULONG32	DeviceControl(ULONG code, READ_WRITE read_write, UCHAR * buf, ULONG32 & data_size, ULONG32 buf_size) = 0;
	virtual ULONG64	GetSize(void) const = 0;
};

class IDriverFactory : public IJCInterface
{
public:
	virtual bool	CreateDriver(const CJCStringT & driver_name, jcparam::IValue * param, IImage * & driver) = 0;
	virtual UINT	GetRevision(void) const = 0;
};

typedef BOOL (*GET_DRV_FACT_PROC) (CJCLogger * log, IDriverFactory * & factory);



