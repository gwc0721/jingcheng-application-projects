
#include "stdafx.h"

#include "scsi_storage_device.h"


LOCAL_LOGGER_ENABLE(_T("CScsiStorageDevice"), LOGGER_LEVEL_DEBUGINFO); 

///////////////////////////////////////////////////////////////////////////////
// -- device scsi

CScsiStorageDevice::CScsiStorageDevice(HANDLE dev)
	: CStorageDeviceComm(dev)
{
}

CScsiStorageDevice::~CScsiStorageDevice(void)
{
}

void CScsiStorageDevice::Create(HANDLE dev, IStorageDevice * & i_dev)
{
	i_dev = static_cast<IStorageDevice*>(new CScsiStorageDevice(dev));
}

///////////////////////////////////////////////////////////////////////////////
// -- device SOLO TESTER

CSoloTester::CSoloTester(HANDLE dev)
	: CStorageDeviceComm(dev)
	, m_test_type(TT_UNKNOWN)
	, m_port(0)
{
}

CSoloTester::~CSoloTester(void)
{
}

void CSoloTester::Create(HANDLE dev, IStorageDevice * & i_dev)
{
	i_dev = static_cast<IStorageDevice*>(new CSoloTester(dev));
}

bool CSoloTester::Recognize(void)
{
	LOG_STACK_TRACE();
	stdext::auto_array<BYTE> buf(SECTOR_SIZE);

	// inquiry command to check if it is SM333
	Inquiry(buf, SECTOR_SIZE);

	bool check1, check2;
	char vendor[16];
	memcpy_s(vendor, 16, buf + 8, 16);	vendor[15] = 0;
	LOG_DEBUG(_T("check1 = %S"), vendor);
	check1 = ( strstr(vendor, "SM333") != NULL );

	memcpy_s(vendor, 16, buf + 5, 3);	vendor[3] = 0;
	LOG_DEBUG(_T("check2 = %S"), vendor);
	check2 = ( strstr(vendor, "smi") != NULL );

	if ( !check1 && !check2) return false;

	// check SM333
	BYTE cb[CDB10GENERIC_LENGTH];

	// Initial card command
	memset(cb, 0, CDB10GENERIC_LENGTH);
	cb[0] = 0xF2, cb[1] = 0x80, cb[2] = 1;
	bool br = ScsiCommand(read, buf, SECTOR_SIZE, cb, CDB10GENERIC_LENGTH, 600);
	if (!br) 
	{
		LOG_WARNING(_T("vendor cmd 0xF280 failed"));
		return false;
	}
	Sleep(1000);

	// inquiry tester
	memset(cb, 0, CDB10GENERIC_LENGTH);
	cb[0] = 0xF0, cb[1] = 0x83, cb[6] = 1, cb[0xB] = 1, cb[0xF] = 1;
	br = ScsiCommand(read, buf, SECTOR_SIZE, cb, CDB10GENERIC_LENGTH, 600);
	if (br)
	{
		if ( 0x55==buf[0x1E] && 0xAA==buf[0x1F] )
		{
			LOG_DEBUG(_T("test = SM333"));
			m_test_type = TT_SM333;
		}
		m_port = buf[0x1D];
		m_connected = (buf[0x0C] != 0);
		LOG_DEBUG(_T("port = %d, connectd = %d"), m_port, m_connected);
	}

	return true;
}

