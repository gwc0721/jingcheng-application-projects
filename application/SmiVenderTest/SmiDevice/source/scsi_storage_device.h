#pragma once

#include "storage_device_comm.h"
#include <tchar.h>

///////////////////////////////////////////////////////////////////////////////
// -- device scsi

class CScsiStorageDevice: public CStorageDeviceComm
{
public:

protected:
	CScsiStorageDevice(HANDLE dev);
	virtual ~CScsiStorageDevice(void);
public:
	virtual bool Recognize(void) {return true;}
	static void Create(HANDLE dev, IStorageDevice * &);

	FILESIZE ReadCapacity(void);

	//virtual bool ReadSmartData(BYTE * buf, JCSIZE len);
	//virtual bool IdentifyDevice(BYTE * buf, JCSIZE len);

protected:
	static CStorageDeviceInfo	m_register;
};


///////////////////////////////////////////////////////////////////////////////
// -- device SOLO TESTER

class CSoloTester: public CStorageDeviceComm
{
public:
	enum TEST_TYPE
	{
		TT_UNKNOWN, TT_SM333, TT_SM334
	};

protected:
	CSoloTester(HANDLE dev);
	virtual ~CSoloTester(void);
public:
	virtual bool Recognize(void);
	static void Create(HANDLE dev, IStorageDevice * &);

protected:
	//bool TesterVendorCmdRead(BYTE * cmd, BYTE * buf, JCSIZE len);
	TEST_TYPE	m_test_type;
	UINT	m_port;
	bool	m_connected;

protected:
	static CStorageDeviceInfo	m_register;
};


