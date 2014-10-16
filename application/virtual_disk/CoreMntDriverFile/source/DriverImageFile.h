#pragma once

#include "../CoreMntLib/include/mntImage.h"


class CDriverFactory : public IDriverFactory
{
public:
	CDriverFactory(void);
	~CDriverFactory(void);

public:
	inline virtual void AddRef()	{};			// static object
	inline virtual void Release(void)	{};		// static object
	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr){return false;};

	virtual bool	CreateDriver(const CJCStringT & driver_name, jcparam::IValue * param, IImage * & driver);
	virtual UINT	GetRevision(void) const;
};



class CDriverImageFile : public IImage
{
public:
	CDriverImageFile(const CJCStringT file_name, ULONG64 secs);
	~CDriverImageFile(void);

	IMPLEMENT_INTERFACE;

public:
	virtual bool	Read(char * buf, ULONG64 lba, ULONG32 secs);
	virtual bool	Write(const char * buf, ULONG64 lba, ULONG32 secs);
	virtual ULONG64	GetSize(void) const {return m_file_secs;}

protected:

	enum READ_WRITE
	{
		READ,
		WRITE,
	};

	enum VENDOR_CMD_STATUS
	{
		VDS0 = 0,	// standby
		VDS1 = 1,
		VDS2 = 2,
		VDS3 = 3,
		VDS4 = 4,
		VDS_CMD = 5,
		VDS_DATA = 6,
	};

	VENDOR_CMD_STATUS VendorCmdStatus(READ_WRITE rdwr, ULONG64 lba, ULONG32 secs, char * buf);

	VENDOR_CMD_STATUS	m_vendorcmd_st;

protected:
	HANDLE		m_file;
	ULONG64		m_file_secs;
};

extern CDriverFactory	g_factory;