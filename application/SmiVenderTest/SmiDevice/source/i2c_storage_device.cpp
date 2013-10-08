#include "stdafx.h"
#include "i2c_storage_device.h"

LOCAL_LOGGER_ENABLE(_T("CI2CStorageDevice"), LOGGER_LEVEL_ERROR); 

CI2CStorageDevice::CI2CStorageDevice(HANDLE dev)
	: CStorageDeviceComm(dev)
{
}

CI2CStorageDevice::~CI2CStorageDevice(void)
{
}

bool CI2CStorageDevice::Recognize(void)
{
	LOG_STACK_TRACE();

	stdext::auto_array<BYTE> buf(SECTOR_SIZE);

	Inquiry(buf, SECTOR_SIZE);

	char vendor[16];
	memset(vendor, 0, 16);
	memcpy_s(vendor, 16, buf + 8, 16);
	if ( strstr(vendor, "SM333") == NULL )		return false;

	memset(buf, 0xFF, 512);
	ReadStatus(buf);
	if ( 0x00 != buf[0] )		return false;

	// Get max LBA
	//ata_device->m_feature_lba48 = (buf[83] & 0x0400) > 0; 
	//if (ata_device->m_feature_lba48)	ata_device->m_max_lba = MAKEQWORD(MAKELONG(buf[100], buf[101]), MAKELONG(buf[102], buf[103]));
	//else					ata_device->m_max_lba = MAKELONG(buf[60], buf[61]);

	return true;
}

void CI2CStorageDevice::Create(HANDLE dev, IStorageDevice * & i_dev)
{
	CI2CStorageDevice * i2c_device = new CI2CStorageDevice(dev);
	i_dev = static_cast<IStorageDevice*>(i2c_device);
}

#define CB_LENGTH	(16)

bool CI2CStorageDevice::I2CCommand(WORD cmd_id, READWRITE rd_wr, BYTE * buf, JCSIZE buf_len)
{
	LOG_STACK_TRACE();
	JCASSERT(buf);
	BYTE cb[CB_LENGTH];
	memset(cb, 0, CB_LENGTH);
	cb[0] = HIBYTE(cmd_id);
	cb[1] = LOBYTE(cmd_id);
	cb[0xb] = 1;

	return ScsiCommand(rd_wr, buf, SECTOR_SIZE, cb, CB_LENGTH, 1000);
}

// buf大小为1 sector。buf从调用者传递，以减少new的次数，提高效率。
bool CI2CStorageDevice::SendAtaCommand(ATA_REGISTER & reg, BYTE * buf)
{
	LOG_STACK_TRACE();
	JCASSERT(buf);
	memset(buf, 0, SECTOR_SIZE);
	buf[0] = 0x10;
	buf[1] = 0x11;
	buf[5] = 0x08;
	bool br = I2CCommand(0xF184, write, buf, SECTOR_SIZE);
	if ( !br) return false;

	memset(buf, 0, SECTOR_SIZE);
	buf[0] = reg.command;
	buf[2] = reg.sec_count;
	buf[3] = reg.lba_low;
	buf[4] = reg.lba_mid;
	buf[5] = reg.lba_hi;
	buf[6] = reg.device;
	br = I2CCommand(0xF185, write, buf, SECTOR_SIZE);
	return br;
}

bool CI2CStorageDevice::ReadStatus(BYTE * buf)
{
	LOG_STACK_TRACE();
	JCASSERT(buf);
	memset(buf, 0, SECTOR_SIZE);
	buf[0] = 0x10;
	buf[1] = 0x11;
	buf[2] = 0x01;
	buf[3] = 0x8C;
	buf[5] = 0x10;
	buf[7] = 0x01;
	bool br = I2CCommand(0xF184, write, buf, SECTOR_SIZE);
	if ( !br) return false;

	I2CCommand(0xF085, read, buf, SECTOR_SIZE);
	return br;
}

bool CI2CStorageDevice::WriteData(BYTE * buf, BYTE * data_buf)
{
	LOG_STACK_TRACE();
	JCASSERT(buf);
	JCASSERT(data_buf);

	memset(buf, 0, SECTOR_SIZE);
	buf[0] = 0x12;
	buf[1] = 0x10;
	buf[6] = 0x02;
	bool br = I2CCommand(0xF184, write, buf, SECTOR_SIZE);
	if ( !br) return false;

	I2CCommand(0xF185, write, data_buf, SECTOR_SIZE);
	return br;
}

bool CI2CStorageDevice::ReadData(BYTE * buf, BYTE * data_buf)
{
	LOG_STACK_TRACE();
	JCASSERT(buf);
	JCASSERT(data_buf);

	memset(buf, 0, SECTOR_SIZE);
	buf[0] = 0x13;
	buf[1] = 0x10;
	buf[6] = 0x02;
	bool br = I2CCommand(0xF184, write, buf, SECTOR_SIZE);
	if ( !br) return false;

	I2CCommand(0xF085, read, data_buf, SECTOR_SIZE);
	return br;
}


bool CI2CStorageDevice::ScsiRead(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout)
{
	LOG_STACK_TRACE();
	JCASSERT(buf);

	bool br = false;

	stdext::auto_array<BYTE> _i2c_buf(SECTOR_SIZE);
	BYTE * i2c_buf = _i2c_buf;

	br = ReadStatus(i2c_buf);

	ATA_REGISTER reg;
	memset(&reg, 0, sizeof(ATA_REGISTER) );
	reg.command = 0x20;
	reg.sec_count = secs;
	reg.lba_low = (BYTE)(lba & 0xFF);
	reg.lba_mid = (BYTE)( (lba>>8) & 0xFF);
	reg.lba_hi = (BYTE)( (lba>>16) & 0xFF);
	reg.device = 0xE0 | (BYTE)((lba>>24) & 0xF);

	br = SendAtaCommand(reg, i2c_buf);

	for (JCSIZE ss = 0; ss < secs; ++ss)
	{
		br = ReadStatus(i2c_buf);
		br = ReadData(i2c_buf, buf);
		buf += SECTOR_SIZE;
	}
	return true;
}

bool CI2CStorageDevice::ScsiWrite(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout)
{
	LOG_STACK_TRACE();
	JCASSERT(buf);

	stdext::auto_array<BYTE> _i2c_buf(SECTOR_SIZE);
	BYTE * i2c_buf = _i2c_buf;

	ATA_REGISTER reg;
	memset(&reg, 0, sizeof(ATA_REGISTER) );
	reg.command = 0x30;
	reg.sec_count = secs;
	reg.lba_low = (BYTE)(lba & 0xFF);
	reg.lba_mid = (BYTE)( (lba>>8) & 0xFF);
	reg.lba_hi = (BYTE)( (lba>>16) & 0xFF);
	reg.device = 0xE0 | (BYTE)((lba>>24) & 0xF);

	SendAtaCommand(reg, i2c_buf);
	//ReadStatus(i2c_buf);

	for (JCSIZE ss = 0; ss < secs; ++ss)
	{
		WriteData(i2c_buf, buf);
		buf += SECTOR_SIZE;
	}
	return true;
}