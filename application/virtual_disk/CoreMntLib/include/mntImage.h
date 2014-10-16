#pragma once

#include <stdext.h>
#include <jcparam.h>

class IImage : public IJCInterface
{
public:
	virtual bool	Read(char * buf, ULONG64 lba, ULONG32 secs) = 0;
	virtual bool	Write(const char * buf, ULONG64 lba, ULONG32 secs) = 0;
	virtual ULONG64	GetSize(void) const = 0;
};

class IDriverFactory : public IJCInterface
{
public:
	virtual bool	CreateDriver(const CJCStringT & driver_name, jcparam::IValue * param, IImage * & driver) = 0;
	virtual UINT	GetRevision(void) const = 0;
};

typedef BOOL (*GET_DRV_FACT_PROC) (CJCLogger * log, IDriverFactory * & factory);



