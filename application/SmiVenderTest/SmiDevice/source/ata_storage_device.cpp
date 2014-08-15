#include "stdafx.h"
#include "ata_storage_device.h"
#include <ntddscsi.h>

LOCAL_LOGGER_ENABLE(_T("CAtaStorageDevice"), LOGGER_LEVEL_ERROR); 

const JCSIZE CAtaStorageDevice::ID_TABLE_LEN = 256;		// In WORDS


bool CAtaStorageDevice::Recognize(void)
{
	LOG_STACK_TRACE();
	//JCASSERT(NULL == i_dev);

	stdext::auto_array<WORD> buf(ID_TABLE_LEN);

	ATA_REGISTER reg;
	memset(&reg, 0, sizeof(ATA_REGISTER) );
	reg.command = 0xEC;

	bool br = false;
	try
	{
		br = AtaCommand(reg, read, false, (BYTE*)((WORD*)buf), 1);
	}
	catch (std::exception &)	{ }
	if ( !br || (reg.status & 1) )		return false;

	// Get max LBA
	m_feature_lba48 = (buf[83] & 0x0400) > 0; 
	if (m_feature_lba48)	m_max_lba = MAKEQWORD(MAKELONG(buf[100], buf[101]), MAKELONG(buf[102], buf[103]));
	else					m_max_lba = MAKELONG(buf[60], buf[61]);

	return true;
}

void CAtaStorageDevice::Create(HANDLE dev, IStorageDevice * & i_dev)
{
	CAtaStorageDevice * ata_device = new CAtaStorageDevice(dev);
	i_dev = static_cast<IStorageDevice*>(ata_device);
}

#define DRIVE_HEAD_REG	0xA0

bool CAtaStorageDevice::ReadSmartData(BYTE * buf, JCSIZE len)
{
	JCASSERT(m_dev);
	DWORD readsize = 0;

	// Get Smart version
	GETVERSIONINPARAMS vip;

	BOOL br = DeviceIoControl( m_dev, SMART_GET_VERSION, 
		NULL, 0, 
		&vip, sizeof(GETVERSIONINPARAMS),
		&readsize, NULL);

	if (!br) THROW_WIN32_ERROR(_T("Get SMART version failed. "));
	if ( (vip.fCapabilities & CAP_SMART_CMD) != CAP_SMART_CMD ) 
		THROW_ERROR(ERR_UNSUPPORT, _T("Device doesn't support SMART feature"));

	// Smart Enable
	SENDCMDINPARAMS stCIP={0};
	SENDCMDOUTPARAMS stCOP={0};

	stCIP.cBufferSize=0;
	stCIP.irDriveRegs.bFeaturesReg=ENABLE_SMART;
	stCIP.irDriveRegs.bSectorCountReg = 1;
	stCIP.irDriveRegs.bSectorNumberReg = 1;
	stCIP.irDriveRegs.bCylLowReg = SMART_CYL_LOW;
	stCIP.irDriveRegs.bCylHighReg = SMART_CYL_HI;
	stCIP.irDriveRegs.bDriveHeadReg = DRIVE_HEAD_REG;
	stCIP.irDriveRegs.bCommandReg = SMART_CMD;
	
	br = DeviceIoControl(m_dev, SMART_SEND_DRIVE_COMMAND, 
		&stCIP, sizeof(stCIP),
		&stCOP, sizeof(stCOP),
		&readsize, NULL);
	if (!br) THROW_WIN32_ERROR(_T("SMART not enable. "));

	// Read SMART
	struct  ST_DRIVERSTAT
	{
		BYTE  bDriverError;
		BYTE  bIDEStatus;
		BYTE  bReserved[2];
		DWORD dwReserved[2];
	};

	struct ST_ATAOUTPARAM
	{
		DWORD      cBufferSize;
		ST_DRIVERSTAT DriverStatus;
		BYTE       bBuffer[READ_ATTRIBUTE_BUFFER_SIZE];
	}/* szOutput*/;
	static const JCSIZE OUT_BUFFER_SIZE = sizeof(ST_ATAOUTPARAM);

	stdext::auto_ptr<ST_ATAOUTPARAM> szOutput;
	memset(szOutput, 0, OUT_BUFFER_SIZE);


	memset(&stCIP, 0, sizeof(SENDCMDINPARAMS));

	// Read SMART
	stCIP.cBufferSize=READ_ATTRIBUTE_BUFFER_SIZE;
	//stCIP.bDriveNumber = drive_index;
	stCIP.irDriveRegs.bFeaturesReg=READ_ATTRIBUTES;
	stCIP.irDriveRegs.bSectorCountReg = 1;
	stCIP.irDriveRegs.bSectorNumberReg = 1;
	stCIP.irDriveRegs.bCylLowReg = SMART_CYL_LOW;
	stCIP.irDriveRegs.bCylHighReg = SMART_CYL_HI;
	stCIP.irDriveRegs.bDriveHeadReg = DRIVE_HEAD_REG;
	stCIP.irDriveRegs.bCommandReg = SMART_CMD;

	br = DeviceIoControl( m_dev, SMART_RCV_DRIVE_DATA,
		&stCIP, sizeof(stCIP),
		szOutput, OUT_BUFFER_SIZE, 
		& readsize, NULL);
	if (!br) THROW_WIN32_ERROR(_T("SMART receive data. ") );

	memcpy(buf, szOutput->bBuffer, min(READ_ATTRIBUTE_BUFFER_SIZE, len));
	return true;
}

bool CAtaStorageDevice::AtaCommand(ATA_REGISTER &reg, READWRITE read_write, bool dma, BYTE * buf, JCSIZE secs)
{
	LOG_STACK_TRACE();
	LOG_TRACE(_T("Send ata cmd: %02X, %02X, %02X, %02X, %02X, %02X, %02X")
		, reg.feature, reg.sec_count, reg.lba_low, reg.lba_mid, reg.lba_hi, reg.device, reg.command);

	JCASSERT(m_dev);
	JCSIZE buf_len = secs * SECTOR_SIZE;

	// 申请一个4KB对齐的内存。基本思想：申请一块内存比原来多4KB的空间，
	// 然后将指针移到4KB对齐的位置。移动最多不会超过4KB。
	static const JCSIZE ALIGNE_SIZE = 4096;		// Aligne to 4KB
	stdext::auto_array<BYTE>	_buf(buf_len + 2 * ALIGNE_SIZE);	// 
#if 1	// aligne
	BYTE * __buf = (BYTE*)_buf;
	JCSIZE align_mask = (~ALIGNE_SIZE) + 1;
	BYTE * align_buf = (BYTE*)((UINT)(__buf) & align_mask) + ALIGNE_SIZE;			// 4KBB aligne
#else
	BYTE * align_buf = buf;
#endif
	
	JCSIZE hdr_size = sizeof(ATA_PASS_THROUGH_DIRECT);

	ATA_PASS_THROUGH_DIRECT	hdr_in;
	memset(&hdr_in, 0, hdr_size);

	hdr_in.DataTransferLength = buf_len;
	hdr_in.DataBuffer = align_buf;
	if (write == read_write)
	{
		hdr_in.AtaFlags = ATA_FLAGS_DATA_OUT;
		if (buf_len > 0) memcpy_s(align_buf, buf_len, buf, buf_len);
	}
	else							
	{
		hdr_in.AtaFlags = ATA_FLAGS_DATA_IN;
	}
	if (dma)	hdr_in.AtaFlags |= ATA_FLAGS_USE_DMA;
	else		hdr_in.AtaFlags |= ATA_FLAGS_NO_MULTIPLE;
	hdr_in.AtaFlags |= ATA_FLAGS_DRDY_REQUIRED;

	hdr_in.Length = hdr_size;
    hdr_in.TimeOutValue = 5;
	memcpy_s(hdr_in.CurrentTaskFile, 8, &reg, 8);

	DWORD readsize = 0;
	BOOL br = DeviceIoControl(m_dev, IOCTL_ATA_PASS_THROUGH_DIRECT, &hdr_in, hdr_size, &hdr_in, hdr_size, &readsize, NULL);
	if (!br)
	{
		stdext::CWin32Exception err(GetLastError(), _T("failure on ata command "));
		LOG_ERROR( err.WhatT() );
	}
	memcpy_s(&reg, 8, hdr_in.CurrentTaskFile, 8);
	if ( (read == read_write) && (buf_len > 0) )			memcpy_s(buf, buf_len, align_buf, buf_len);
	LOG_DEBUG(_T("Returned ata cmd: %02X, %02X"), reg.error, reg.status);

	if (!br) THROW_WIN32_ERROR(_T("failure in invoking ata command."));
	
	return true;
}

void CAtaStorageDevice::AtaCommand48(ATA_REGISTER &reg_pre, ATA_REGISTER &reg_cur, READWRITE read_write, BYTE * buf, JCSIZE secs)
{
	JCASSERT(m_dev);

	JCSIZE buf_len = secs * SECTOR_SIZE;
	JCSIZE hdr_size = sizeof(ATA_PASS_THROUGH_DIRECT);
	ATA_PASS_THROUGH_DIRECT	hdr_in;
	memset(&hdr_in, 0, hdr_size);

	ATA_PASS_THROUGH_DIRECT	hdr_out;
	memset(&hdr_out, 0, hdr_size);
	if (write == read_write)
	{
		hdr_out.DataBuffer = buf;
		hdr_out.DataTransferLength = buf_len;
		hdr_in.AtaFlags = ATA_FLAGS_DATA_OUT;
	}
	else
	{
		hdr_in.DataTransferLength = buf_len;
		hdr_in.DataBuffer = buf;
		hdr_in.AtaFlags = ATA_FLAGS_DATA_IN;
	}
	hdr_in.AtaFlags |= ATA_FLAGS_48BIT_COMMAND;

	hdr_in.Length = hdr_size;
    hdr_in.TimeOutValue = 10;
	memcpy_s(hdr_in.CurrentTaskFile, 8, &reg_cur, 8);
	memcpy_s(hdr_in.PreviousTaskFile, 8, &reg_pre, 8);

	hdr_out.Length = hdr_size;

	DWORD readsize = 0;
	BOOL br = DeviceIoControl(m_dev, IOCTL_ATA_PASS_THROUGH_DIRECT, &hdr_in, hdr_size, &hdr_out, hdr_size, &readsize, NULL);
	memcpy_s(&reg_cur, 8, hdr_out.CurrentTaskFile, 8);
	memcpy_s(&reg_pre, 8, hdr_out.PreviousTaskFile, 8);
}

bool CAtaStorageDevice::QueryInterface(const char * if_name, IJCInterface * &if_ptr)
{
	JCASSERT(NULL == if_ptr);
	bool br = false;
	if ( FastCmpA(IF_NAME_ATA_DEVICE, if_name) || FastCmpA(IF_NAME_STORAGE_DEVICE, if_name) )
	{
		if_ptr = static_cast<IJCInterface*>(this);
		if_ptr->AddRef();
		br = true;
	}
	else br = __super::QueryInterface(if_name, if_ptr);
	return br;
}

BYTE CAtaStorageDevice::ReadDMA(FILESIZE lba, JCSIZE secs, BYTE &error, BYTE * buf)
{
	ATA_REGISTER reg;
	reg.command = 0xC8;
	reg.sec_count = (BYTE)(secs & 0xFF);
	reg.lba_low = (BYTE)(lba & 0xFF);
	reg.lba_mid = (BYTE)( (lba>>8) & 0xFF);
	reg.lba_hi = (BYTE)( (lba>>16) & 0xFF);
	reg.device = 0x40 | (BYTE)((lba>>24) & 0xF);

	AtaCommand(reg, read, true, buf, secs);
	error = reg.error;
	return reg.status;
}

BYTE CAtaStorageDevice::ReadPIO(FILESIZE lba, BYTE secs, BYTE &error, BYTE * buf)
{
	JCSIZE data_secs = (secs==0)?256:secs;
	ATA_REGISTER reg;
	reg.command = 0x20;
	reg.sec_count = secs;
	reg.lba_low = (BYTE)(lba & 0xFF);
	reg.lba_mid = (BYTE)( (lba>>8) & 0xFF);
	reg.lba_hi = (BYTE)( (lba>>16) & 0xFF);
	reg.device = 0x40 | (BYTE)((lba>>24) & 0xF);

	AtaCommand(reg, read, false, buf, data_secs);
	error = reg.error;
	return reg.status;
}

BYTE CAtaStorageDevice::WritePIO(FILESIZE lba, JCSIZE secs, BYTE &error, BYTE * buf)
{
	JCSIZE data_secs = (secs==0)?256:secs;
	ATA_REGISTER reg;
	reg.command = 0x30;
	reg.sec_count = secs;
	reg.lba_low = (BYTE)(lba & 0xFF);
	reg.lba_mid = (BYTE)( (lba>>8) & 0xFF);
	reg.lba_hi = (BYTE)( (lba>>16) & 0xFF);
	reg.device = 0x40 | (BYTE)((lba>>24) & 0xF);

	AtaCommand(reg, write, false, buf, data_secs);
	error = reg.error;
	return reg.status;
}


BYTE CAtaStorageDevice::WriteDMA(FILESIZE lba, JCSIZE secs, BYTE &error, BYTE * buf)
{
	JCSIZE data_secs = (secs==0)?256:secs;
	ATA_REGISTER reg;
	reg.command = 0xCA;
	reg.sec_count = secs;
	reg.lba_low = (BYTE)(lba & 0xFF);
	reg.lba_mid = (BYTE)( (lba>>8) & 0xFF);
	reg.lba_hi = (BYTE)( (lba>>16) & 0xFF);
	reg.device = 0x40 | (BYTE)((lba>>24) & 0xF);

	AtaCommand(reg, write, true, buf, data_secs);
	error = reg.error;
	return reg.status;
}


BYTE CAtaStorageDevice::FlushCache(BYTE & error)
{
	ATA_REGISTER reg;
	reg.command = 0xE7;
	reg.sec_count = 0;
	reg.device = 0x40;
	AtaCommand(reg, read, false, NULL, 0);
	error = reg.error;
	return reg.status;
}

bool CAtaStorageDevice::IdentifyDevice(BYTE * buf, JCSIZE len)
{
	JCASSERT(m_dev);
	JCASSERT(len >= SECTOR_SIZE);

	ATA_REGISTER reg;
	memset(&reg, 0, sizeof(ATA_REGISTER) );

	reg.command = 0xEC;
	bool br = AtaCommand(reg, read, false, buf, 1);
	return br;
}


#if 0
bool CAtaStorageDevice::SectorRead(BYTE * buf, FILESIZE lba, JCSIZE sectors)
{
	LOG_STACK_TRACE();

	// 申请一个4KB对齐的内存。基本思想：申请一块内存比原来多4KB的空间，
	// 然后将指针移到4KB对齐的位置。移动最多不会超过4KB。

	JCSIZE hdr_size = sizeof(ATA_PASS_THROUGH_DIRECT);

	ATA_PASS_THROUGH_DIRECT	hdr_in;
	memset(&hdr_in, 0, hdr_size);

	JCSIZE buf_len = SECTOR_SIZE * sectors;

	hdr_in.DataTransferLength = buf_len;
	hdr_in.DataBuffer = buf;
	hdr_in.AtaFlags = ATA_FLAGS_DATA_IN | ATA_FLAGS_USE_DMA;
	hdr_in.Length = hdr_size;
    hdr_in.TimeOutValue = 5;

	hdr_in.CurrentTaskFile[6] = 0xC8;
	hdr_in.CurrentTaskFile[1] = sectors;
	hdr_in.CurrentTaskFile[2] = (BYTE)(lba & 0xFF);
	hdr_in.CurrentTaskFile[3] = (BYTE)( (lba>>8) & 0xFF);
	hdr_in.CurrentTaskFile[4] = (BYTE)( (lba>>16) & 0xFF);
	hdr_in.CurrentTaskFile[5] = 0x40 | (BYTE)((lba>>24) & 0xF);

	DWORD readsize = 0;
	BOOL br = DeviceIoControl(m_dev, IOCTL_ATA_PASS_THROUGH_DIRECT, &hdr_in, hdr_size, &hdr_in, hdr_size, &readsize, NULL);
	if (!br)
	{
		stdext::CWin32Exception err(GetLastError(), _T("failure on ata command "));
		LOG_ERROR( err.WhatT() );
	}
	return (br == TRUE);
}
#endif