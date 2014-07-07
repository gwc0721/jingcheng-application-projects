#include "stdafx.h"
#include "smi_device_comm.h"

LOCAL_LOGGER_ENABLE(_T("CSmiDeviceComm"), LOGGER_LEVEL_ERROR);

const CSmartAttrDefTab CSmiDeviceComm::m_smart_attr_tab(CSmartAttrDefTab::RULE()
	( new CSmartAttributeDefine(0x05, _T("Run time bad blocks")) )
	( new CSmartAttributeDefine(0x0C, _T("Power cycle")) )
	( new CSmartAttributeDefine(0xA1, _T("Valid spare blocks")) )
	( new CSmartAttributeDefine(0xA2, _T("Child pairs")) )
	( new CSmartAttributeDefine(0xA3, _T("Initial bad blocks")) )
	( new CSmartAttributeDefine(0xA4, _T("Total erase count")) )
	( new CSmartAttributeDefine(0xA5, _T("Max erase count")) )
	( new CSmartAttributeDefine(0xA6, _T("Min erase count")) )
	( new CSmartAttributeDefine(0xA7, _T("Average erase count")) )
	( new CSmartAttributeDefine(0xC0, _T("Sudden power down")) )
	( new CSmartAttributeDefine(0xC7, _T("CRC error")) )
	( new CSmartAttributeDefine(0xF1, _T("Max erase count")) )
	( new CSmartAttributeDefine(0xF6, _T("Total read sectors")) )
	( new CSmartAttributeDefine(0xF7, _T("Total write sectors")) )
	( new CSmartAttributeDefine(0xF8, _T("Total erase count")) )
	( new CSmartAttributeDefine(0xF9, _T("Initial bad blocks")) )
	( new CSmartAttributeDefine(0xFA, _T("Spare blocks")) )
	( new CSmartAttributeDefine(0xFB, _T("Child pairs")) )
	( new CSmartAttributeDefine(0xFC, _T("Min erase count")) )
	( new CSmartAttributeDefine(0xFD, _T("Average erase count")) )
	);

CSmiDeviceComm::CSmiDeviceComm(IStorageDevice * dev)
	: m_dev(dev)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dev);
	m_dev->AddRef();
}


CSmiDeviceComm::~CSmiDeviceComm(void)
{
	if (m_dev)
	{
		m_dev->DeviceUnlock();
		m_dev->Release();
	}
}

bool CSmiDeviceComm::QueryInterface(const char * if_name, IJCInterface * &if_ptr)
{
	JCASSERT(NULL == if_ptr);
	bool br = false;
	if (if_name == IF_NAME_STORAGE_DEVICE ||
		strcmp(if_name, IF_NAME_STORAGE_DEVICE) == 0)
	{
		if_ptr = static_cast<IJCInterface*>(m_dev);
		m_dev->AddRef();
		br = true;
	}
	else
	{
		br = __super::QueryInterface(if_name, if_ptr);
		if (!br) br = m_dev->QueryInterface(if_name, if_ptr);
	}
	return br;
}

//-- Implemnt for ISmiDevice
void CSmiDeviceComm::ReadSFR(BYTE * buf, JCSIZE secs)
{
	JCASSERT(m_dev);
	JCASSERT(secs >= 1);

	CSmiCommand cmd_srf;
	cmd_srf.id(0xF026);
	cmd_srf.size() = 1;

	JCSIZE len = 1;

	VendorCommand(cmd_srf, read, buf, len);
}


void CSmiDeviceComm::GetSmartData(BYTE * data)
{
	JCASSERT(m_dev);

	stdext::auto_cif<IAtaDevice>	storage;
	m_dev->QueryInterface(IF_NAME_ATA_DEVICE, storage);
	if (!storage.valid())  THROW_ERROR(ERR_UNSUPPORT, _T("Device doesn't support SMART feature"));
	storage->ReadSmartAttribute(data, 512);
}

void CSmiDeviceComm::ReadSRAM(WORD ram_add, JCSIZE len, BYTE * buf)
{
	stdext::auto_array<BYTE> _buf(SECTOR_SIZE);
	WORD start_add = ram_add & 0xFE00;
	JCSIZE offset = ram_add - start_add;

	ReadSRAM(start_add, _buf);

	JCSIZE copy_len = min(SECTOR_SIZE - offset, len);
	stdext::jc_memcpy(buf, len, _buf + offset, copy_len);

	while (1)
	{
		len -= copy_len;
		if (0 == len) break;
		start_add += SECTOR_SIZE;
		buf += copy_len;

		ReadSRAM(start_add, _buf);
		copy_len = min(SECTOR_SIZE, len);
		stdext::jc_memcpy(buf, len, _buf, copy_len);
	}
}

void CSmiDeviceComm::ReadSRAM(WORD ram_add, BYTE * buf)
{
	JCASSERT( 0 == (ram_add & 0x001EE) );
	CCmdReadRam cmd;
	cmd.add() = ram_add >> 8;
	JCSIZE sec = 1;
	VendorCommand(cmd, read, buf, sec);
}

#ifndef VENDER_RETRY
#define VENDER_RETRY 3
#endif

bool CSmiDeviceComm::VendorCommand(CSmiCommand & cmd, READWRITE rd_wr, BYTE* data_buf, JCSIZE secs)
{
	LOG_STACK_TRACE();
	LOG_TRACE(_T("Vendor Cmd 0x%04X, 0x%04X, 0x%04X, 0x%04X, secs=%d"), 
		cmd.raw_word(0), cmd.raw_word(1), cmd.raw_word(2), cmd.raw_word(3), cmd.size() );
	JCASSERT(m_dev);
	BYTE buf[SECTOR_SIZE];

	int retry = VENDER_RETRY;
	while (retry > 0)
	{
		m_dev->ScsiRead(buf, 0x00AA, 1, 60);
		m_dev->ScsiRead(buf, 0xAA00, 1, 60);
		m_dev->ScsiRead(buf, 0x0055, 1, 60);
		m_dev->ScsiRead(buf, 0x5500, 1, 60);
		m_dev->ScsiRead(buf, 0x55AA, 1, 60);
		if ( CheckVenderCommand(buf) ) break;
		--retry;
#ifdef _DEBUG
		FILE * file = NULL;
		_tfopen_s(&file, _T("dump_vender.bin"), _T("w+b"));
		fwrite(buf, SECTOR_SIZE, 1, file);
		fclose(file);
#endif
		LOG_ERROR(_T("Vender command 0x55AA failed. retry remain: %d"), retry);
	}

	if ( 0 == retry ) 
	{
		THROW_ERROR(ERR_PARAMETER, _T("Failure in verify vender command."));
	}

	memset(buf, 0, SECTOR_SIZE);
	memcpy_s(buf, SECTOR_SIZE, cmd.data(), cmd.length() );
	m_dev->ScsiWrite(buf, 0x55AA, 1, 600);

	if (rd_wr == read)	m_dev->ScsiRead(data_buf, 0x55AA, secs, 600);
	else				m_dev->ScsiWrite(data_buf, 0x55AA, secs, 600);

	return true;
}

void CSmiDeviceComm::ReadFlashID(BYTE * buf, JCSIZE secs)
{
	JCASSERT(m_dev);
	JCASSERT(secs >= 1);

	CSmiCommand flash;
	flash.id(0xF020);
	flash.size() = 1;

	JCSIZE len = 1;

	VendorCommand(flash, read, buf, len);
}

void CSmiDeviceComm::ReadFlashChunk(const CFlashAddress & add, CSpareData & spare, BYTE * buf, JCSIZE secs, UINT option)
{
	LOG_STACK_TRACE();
	LOG_NOTICE(_T("f-block=0x%04X, f-page=0x%04X, f-chunk=0x%04X"), add.m_block, add.m_page, add.m_chunk);
	JCASSERT(m_dev);
	JCSIZE f_secs = m_f_chunk_size + 1;
	JCASSERT(secs >= f_secs);

	CCmdReadFlash	cmd;
	FlashAddToPhysicalAdd(add, cmd, option);
	cmd.size() = f_secs;
	VendorCommand(cmd, read, buf, f_secs);

	BYTE * spare_buf = buf + m_f_chunk_size * SECTOR_SIZE;
	GetSpare(spare, spare_buf);
}

JCSIZE CSmiDeviceComm::WriteFlash(const CFlashAddress & add, BYTE * buf, JCSIZE secs)
{
	LOG_STACK_TRACE();
	LOG_NOTICE(_T("f-block=0x%04X, f-page=0x%04X, size=%d"), add.m_block, add.m_page, secs);
	JCASSERT(m_dev);

	JCSIZE f_page_size = m_f_chunk_size * m_f_chunk_per_page;
	if (secs > f_page_size) secs = f_page_size;

	CCmdWriteFlash	cmd;
	FlashAddToPhysicalAdd(add, cmd, 0);
	cmd.size() = secs;
	VendorCommand(cmd, write, buf, secs);

	return secs;
}


void CSmiDeviceComm::EraseFlash(const CFlashAddress & add)
{
	LOG_STACK_TRACE();
	LOG_NOTICE(_T("f-block=0x%04X"), add.m_block);
	JCASSERT(m_dev);

	stdext::auto_array<BYTE> buf(SECTOR_SIZE);
	CCmdEraseBlock cmd;
	cmd.block(add.m_block);
	VendorCommand(cmd, read, buf, 1);
}


JCSIZE CSmiDeviceComm::GetSystemBlockId(JCSIZE id)
{
	BYTE buf[SECTOR_SIZE];
	JCSIZE index = id & 0xF;

	switch (id & BID_SYSTEM_MASK)
	{
	case BID_ISP: {
		JCASSERT(index < 2);
		return m_isp_index[index];
		}

	case BID_INFO: 
		ReadFlashID(buf, 1);
		if (0 == index)			return MAKEWORD(buf[0x181], buf[0x180]);
		else if (1 == index)	return MAKEWORD(buf[0x183], buf[0x182]);
		else	THROW_ERROR(ERR_PARAMETER, _T("Index %d of info block is not exist"), index);

	default:
		THROW_ERROR(ERR_PARAMETER, _T("System block id 0x%08X is unknown"), id);
	}
	return 0;
}

bool CSmiDeviceComm::GetCardInfo(CCardInfo & card_info)
{
	//NAND_TYPE	type;	// SLC, MLC, TLC, SLC-mode
	card_info.m_controller_name = Name();

	card_info.m_type = m_nand_type;	
	
	// Card mode
	card_info.m_channel_num = m_channel_num;
	card_info.m_interleave = m_interleave;
	card_info.m_plane = m_plane;

	// physical parameter
	card_info.m_p_ppb = m_p_page_per_block;		// physical page per physical block

	card_info.m_f_block_num = m_f_block_num;
	card_info.m_f_ppb = m_f_page_per_block;
	card_info.m_f_ckpp = m_f_chunk_per_page;
	card_info.m_f_spck = m_f_chunk_size;

	return true;
}

void CSmiDeviceComm::SetCardInfo(const CCardInfo & cardinfo, UINT mask)
{
	if ( mask & ISmiDevice::CIM_F_BLOCK_NUM)	m_f_block_num = cardinfo.m_f_block_num;
	if ( mask & ISmiDevice::CIM_F_PPB)			m_f_page_per_block = cardinfo.m_f_ppb;
	if ( mask & ISmiDevice::CIM_F_CKPP)			m_f_chunk_per_page = cardinfo.m_f_ckpp;
	if ( mask & ISmiDevice::CIM_F_SPCK)			m_f_chunk_size= cardinfo.m_f_spck;

	if ( mask & ISmiDevice::CIM_M_INTLV)		m_interleave = cardinfo.m_interleave;
	if ( mask & ISmiDevice::CIM_M_PLANE)		m_plane = cardinfo.m_plane;
	if ( mask & ISmiDevice::CIM_M_CHANNEL)		m_channel_num = cardinfo.m_channel_num;


	m_p_page_per_block = m_f_page_per_block / m_interleave;		// physical page per physical block
	m_p_chunk_per_page = m_f_chunk_per_page / m_plane;		// (*) physical chunk per page = m_f_chunk_per_page / m_plane
}


void CSmiDeviceComm::GetSpare(CSpareData & spare, BYTE * spare_buf)
{
	spare.m_id = spare_buf[0];
	spare.m_hblock = MAKEWORD(spare_buf[2], spare_buf[1]);
	spare.m_hpage = spare_buf[3];
}
	
bool CSmiDeviceComm::GetProperty(LPCTSTR prop_name, UINT & val)
{
	bool br = false;
	if (  FastCmpT(prop_name, CSmiDeviceBase::PROP_INFO_BLOCK) )
	{
		val = MAKELONG(m_info_index[0], m_info_index[1]);
		return true;
	}
	else if (  FastCmpT(prop_name, CSmiDeviceBase::PROP_ISP_BLOCK) )
	{
		val = MAKELONG(m_isp_index[0], m_isp_index[1]);
		return true;
	}
	
	return br;
}