#include "stdafx.h"
#include "SM2236.h"

LOCAL_LOGGER_ENABLE(_T("device_sm2236"), LOGGER_LEVEL_ERROR);

static LPCTSTR STR_RESERVED = _T("Reserved");

const CSmartAttrDefTab CSM2236::m_smart_def_2236(CSmartAttrDefTab::RULE()
	( new CSmartAttributeDefine(0x01, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x02, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x05, _T("New bads")) )
	( new CSmartAttributeDefine(0x06, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x07, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x09, _T("Power on hours")) )
	( new CSmartAttributeDefine(0x0C, _T("Power cycle")) )
	( new CSmartAttributeDefine(0xA0, _T("Online UNC")) )
	( new CSmartAttributeDefine(0xA1, _T("Valid spare")) )
	( new CSmartAttributeDefine(0xA2, _T("Child pairs")) )
	( new CSmartAttributeDefine(0xA3, _T("Init bads")) )
	( new CSmartAttributeDefine(0xA4, _T("Total pe")) )
	( new CSmartAttributeDefine(0xA5, _T("Max pe")) )
	( new CSmartAttributeDefine(0xA6, _T("Min pe")) )
	( new CSmartAttributeDefine(0xA7, _T("Avg pe")) )
	( new CSmartAttributeDefine(0xA8, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0xA9, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0xC0, _T("Sudden down")) )
	( new CSmartAttributeDefine(0xC2, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0xC3, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0xC4, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0xC5, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0xC6, _T("Offline UNC") ) )
	( new CSmartAttributeDefine(0xC7, _T("CRC error")) )
	( new CSmartAttributeDefine(0xC8, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0xF1, _T("Total write sectors")) )
	( new CSmartAttributeDefine(0xF2, _T("Total read sectors")) )
);

/*
const CSmartAttrDefTab CSM2236::m_smart_neci(CSmartAttrDefTab::RULE()
	( new CSmartAttributeDefine(0x01, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x02, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x05, _T("New bads")) )
	( new CSmartAttributeDefine(0x06, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x07, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x09, _T("Power on hours")) )
	( new CSmartAttributeDefine(0x0C, _T("Power cycle")) )
	( new CSmartAttributeDefine(0xA0, _T("Online UNC")) )
	( new CSmartAttributeDefine(0xC0, _T("Sudden down")) )
	( new CSmartAttributeDefine(0xC6, _T("Offline UNC") ) )
	( new CSmartAttributeDefine(0xC7, _T("CRC error")) )
	( new CSmartAttributeDefine(0xF1, _T("Max pe")) )
	( new CSmartAttributeDefine(0xF6, _T("Total read sectors")) )
	( new CSmartAttributeDefine(0xF7, _T("Total write sectors")) )
	( new CSmartAttributeDefine(0xF8, _T("Total pe")) )
	( new CSmartAttributeDefine(0xF9, _T("Init bads")) )
	( new CSmartAttributeDefine(0xFA, _T("Spare blocks")) )
	( new CSmartAttributeDefine(0xFB, _T("Child pairs")) )
	( new CSmartAttributeDefine(0xFC, _T("Min pe")) )
	( new CSmartAttributeDefine(0xFD, _T("Avg pe")) )
);
*/


CSM2236::CSM2236(IStorageDevice * dev)
	: CLT2244(dev)
	, m_info_page(0xFF), m_bitmap_page(0xFF)
	, m_orphan_page(0xFF), m_blockindex_page(0xFF)
{
}

bool CSM2236::CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device)
{
	LOG_STACK_TRACE();
	JCASSERT(storage);
	smi_device =  static_cast<ISmiDevice*>(new CSM2236(storage));
	return true;
}

bool CSM2236::Recognize(IStorageDevice * storage, BYTE * inquery)
{
    LOG_STACK_TRACE();
	char vendor[16];
	memset(vendor, 0, 16);
	memcpy_s(vendor, 16, inquery + 8, 16);
	LOG_DEBUG(_T("Checking inquery = %S"), vendor);

	LOG_DEBUG(_T("Checking vender command..."));
	bool br = false;
	try
	{
		storage->DeviceLock();
		// try vendor command
		stdext::auto_array<BYTE> buf0(SECTOR_SIZE);
		storage->ScsiRead(buf0, 0x00AA, 1, 60);
		storage->ScsiRead(buf0, 0xAA00, 1, 60);
		storage->ScsiRead(buf0, 0x0055, 1, 60);
		storage->ScsiRead(buf0, 0x5500, 1, 60);
		storage->ScsiRead(buf0, 0x55AA, 1, 60);

		br = CSM2236::LocalRecognize(buf0);
		// Stop vender command
		storage->ScsiRead(buf0, 0x55AA, 1, 60);
	}
	catch (...)
	{
	}

	return br;
}

bool CSM2236::Initialize(void)
{
	LOG_STACK_TRACE();
	BYTE bb = 0;
	// Read Flash ID
	stdext::auto_array<BYTE> buf(SECTOR_SIZE * 2);
	memset(buf, 0, SECTOR_SIZE);
	ReadFlashID(buf, 1);

	switch (buf[0x0A])
	{
	case 0x00:	m_isp_mode = ISPM_ROM_CODE; break;
	case 0x01:	m_isp_mode = ISPM_MPISP; break;
	case 0x02:	m_isp_mode = ISPM_ISP; break;
	default:	m_isp_mode = ISPM_UNKNOWN;	break;
	}
	m_info_block_valid = (0x01 == buf[0x01]);

	m_mu = buf[0];

	// Check channels and CE
	m_channel_num = 0;
	DWORD flash_id;
	flash_id = MAKELONG( MAKEWORD(buf[0x43], buf[0x42]), MAKEWORD(buf[0x41], buf[0x40]) );
	for (int ii = 0x40; ii < 0x140; ii += 0x40, ++m_channel_num)	
		if (0 == buf[ii]) break;

	m_ce_num = 0;
	for (int ii = 0x40; ii < 0x80; ii += 0x08, ++m_ce_num) 
		if (0 == buf[ii]) break;

	m_info_index[0] = MAKEWORD(buf[0x143], buf[0x142]);
	m_info_index[1] = MAKEWORD(buf[0x145], buf[0x144]);

	m_isp_index[0] = MAKEWORD(buf[0x147], buf[0x146]);
	m_isp_index[1] = MAKEWORD(buf[0x149], buf[0x148]);

	m_org_bad_info = MAKEWORD(buf[0x14B], buf[0x14A]);

	bb = buf[2];
	if (bb & 0x01)			m_f_chunk_size = 4;
	else if (bb & 0x02)		m_f_chunk_size = 8;
	else					m_f_chunk_size = 2;

	memset(buf, 0, SECTOR_SIZE);
	ReadSFR(buf, 1);
	bool slc_mode = ((buf[0x12E] & 0x20) != 0);

	// Read Info block
	if (m_info_block_valid)
	{
		memset(buf, 0, SECTOR_SIZE);
		// Parameter page for card mode
		CCmdBaseLT2244 cmd(16);
		cmd.id(0xF00A);		// read flash
		cmd.block(m_info_index[0]);
		cmd.size() = 1;
		VendorCommand(cmd, read, buf, 2);	// 2 sector for page index

		bb = buf[0x0A];
		if (bb & 0x04)		m_interleave = 4;
		else if(bb & 0x02)	m_interleave = 2;
		else				m_interleave = 1;
		
		if ( buf[0x1C] & 0x80 )
		{
			if (slc_mode)	m_nand_type = CCardInfo::NT_SLC_MODE;
			else			m_nand_type = CCardInfo::NT_MLC;
		}
		else	m_nand_type = CCardInfo::NT_SLC;

		bb = buf[0x1B];
		if (bb & 0x20)			m_plane = 4;
		else if (bb & 0x10)		m_plane = 2;
		else					m_plane = 1;

		JCSIZE ph_page_size = 0;
		bb = buf[0x1C];
		if (bb & 0x04)			ph_page_size = 8;		// in sectors
		else if (bb & 0x01)		ph_page_size = 32;
		else if (bb & 0x02)		ph_page_size = 16;
		else					ph_page_size = 0;

		if (bb & 0x08)			m_p_page_per_block = 256;
		else if (bb & 0x10)		m_p_page_per_block = 192;
		else if (bb & 0x20)		m_p_page_per_block = 128;
		else					m_p_page_per_block = 64;

		//if (0x98DE9482 == flash_id)
		//{
		//	if (slc_mode) m_p_page_per_block /= 2;		// for 651G8~B (L0315)
		//}
		//else if (0x98D7A032 == flash_id)	m_p_page_per_block /= 2;

		if (slc_mode)
		{
			m_p_page_per_block /= 2;
		}
		
		// unnecessary: for 651G4 (L0315) 651G1

		m_f_page_per_block = m_p_page_per_block * m_interleave;	// 
		m_p_chunk_per_page = ph_page_size * m_channel_num / m_f_chunk_size;		
		m_f_chunk_per_page = m_p_chunk_per_page * m_plane;		

		// get info page index
		m_info_page = buf[0x26A] * m_interleave;
		m_bitmap_page = buf[0x26B] * m_interleave;
		m_orphan_page = buf[0x26C] * m_interleave;
		m_blockindex_page = buf[0x26D] * m_interleave;

		// Read page 1 for f-block number
		// search page 1
		for (UINT pp = 1; pp < 10; ++pp)
		{
			cmd.page(pp);
			VendorCommand(cmd, read, buf, 1);
			if (strcmp((char*)(buf + 0xA0), ("SM2236AC")) == 0)
			{
				m_f_block_num = MAKEWORD(buf[0xB7], buf[0xB6]);
				break;
			}
		}


	}
	return true;
}

void CSM2236::ReadSmartFromWpro(BYTE * data)
{
}

void CSM2236::FlashAddToPhysicalAdd(const CFlashAddress & add, CSmiCommand & cmd, UINT option)
{
	CCmdBaseLT2244 & block_cmd = reinterpret_cast<CCmdBaseLT2244 &>(cmd);
	BYTE mode = 0;

	block_cmd.block(add.m_block);

	if (m_interleave == 2)
	{
		block_cmd.page(add.m_page >> 1);
		mode = (add.m_page & 1);
	}
	else	block_cmd.page(add.m_page);

	if (m_plane > 1)
	{
		if (add.m_chunk >= m_p_chunk_per_page)
		{
			block_cmd.chunk() = add.m_chunk - m_p_chunk_per_page;
			mode |= 0x80; 
		}
		else	block_cmd.chunk() = add.m_chunk;
	}
	else block_cmd.chunk() = add.m_chunk;	

	if ( option & ISmiDevice::RFO_DISABLE_ECC ) mode |= 0x08;
	if (option & ISmiDevice::RFO_DISABLE_SCRAMB ) mode |= 0x40;

	//if (false == ecc)		mode |= 8;
	
	block_cmd.mode() = mode;
}

bool CSM2236::GetProperty(LPCTSTR prop_name, UINT & val)
{
	if ( FastCmpT(prop_name, CSmiDeviceBase::PROP_CACHE_NUM ) )
	{
		stdext::auto_array<BYTE> buf(SECTOR_SIZE);
		ReadSRAM(0xDE00, 0, buf);
		val = MAKEWORD(buf[0x10D], buf[0x10C]);
		return true;
	}
	else if ( FastCmpT(prop_name, CSmiDeviceBase::PROP_WPRO) )
	{
		stdext::auto_array<BYTE> buf(SECTOR_SIZE);
		ReadSRAM(0xB000, 0, buf);
		WORD wpro1 = MAKEWORD(buf[0xC0], buf[0xBF]);
		WORD wpro2 = MAKEWORD(buf[0xC2], buf[0xC1]);
		val = MAKELONG(wpro1, wpro2);
		return true;
	}
	else if (  FastCmpT(prop_name, CSmiDeviceBase::PROP_INFO_PAGE) )
	{
		val = MAKELONG(
			MAKEWORD(m_blockindex_page, m_orphan_page), MAKEWORD(m_bitmap_page, m_info_page) );
		return true;
	}
	else if (  FastCmpT(prop_name, CSmiDeviceBase::PROP_ORG_BAD_INFO) )
	{
		val = m_org_bad_info;
		return true;
	}
	else	return __super::GetProperty(prop_name, val);
}

void CSM2236::GetSpare(CSpareData & spare, BYTE * spare_buf)
{
	spare.m_id = spare_buf[0];
	spare.m_hblock = MAKEWORD(spare_buf[2], spare_buf[1]);
	spare.m_hpage = MAKEWORD(spare_buf[4], spare_buf[3]);
	spare.m_erase_count = spare_buf[5];
	
	BYTE * ecc_ch = spare_buf;
	for (BYTE cc = 0; cc < m_channel_num; ++cc, ecc_ch += 16)
	{	// 读取每个channel的error bit数
		if (ecc_ch[0xC] & 1)	spare.m_error_bit[cc] = 0xFF;
		else					spare.m_error_bit[cc] = ecc_ch[0x0A];
	}
	spare.m_ecc_code = spare_buf[0x40];

	// serial number
	if ( 0x40 == (spare_buf[0] & 0xF0) )	spare.m_serial_no = spare_buf[8];
}

JCSIZE CSM2236::GetSystemBlockId(JCSIZE id)
{
	switch (id & BID_SYSTEM_MASK)
	{
	case BID_WPRO:		{
		BYTE wpro_add[2];
		CSmiDeviceComm::ReadSRAM(0xB0BC, 0, 2, wpro_add);
		return MAKEWORD(wpro_add[1], wpro_add[0]);
		}
		
	default:
		return __super::GetSystemBlockId(id);
	}
}

void CSM2236::ReadSRAM(WORD ram_add, WORD bank, BYTE * buf)
{
	JCASSERT( 0 == (ram_add & 0x001EE) );
	CCmdReadRam cmd;
	cmd.add() = ram_add >> 8;
	if ( (ram_add >= 0xC000) && (ram_add < 0xE000) && (bank != 0) )
	{
		cmd.bank(bank);
	}
	JCSIZE sec = 1;
	VendorCommand(cmd, read, buf, sec);
}