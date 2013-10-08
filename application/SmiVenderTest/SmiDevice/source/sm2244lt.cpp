#include "stdafx.h"
#include "SM2244LT.h"

LOCAL_LOGGER_ENABLE(_T("SM2244LT"), LOGGER_LEVEL_ERROR);

static LPCTSTR STR_RESERVED = _T("Reserved");

const CSmartAttrDefTab CLT2244::m_smart_def_2244lt(CSmartAttrDefTab::RULE()
	( new CSmartAttributeDefine(0x01, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x02, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x05, _T("New bads")) )
	( new CSmartAttributeDefine(0x06, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x07, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x09, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0x0C, _T("Power cycle")) )
	( new CSmartAttributeDefine(0xA0, STR_RESERVED, false) )
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
	( new CSmartAttributeDefine(0xC6, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0xC7, _T("CRC error")) )
	( new CSmartAttributeDefine(0xC8, STR_RESERVED, false) )
	( new CSmartAttributeDefine(0xF1, _T("Total write sectors")) )
	( new CSmartAttributeDefine(0xF2, _T("Total read sectors")) )
);

CLT2244::CLT2244(IStorageDevice * dev)
	: CSM2242(dev)
{
	//Init2244();
}

bool CLT2244::CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device)
{
	LOG_STACK_TRACE();
	JCASSERT(storage);
	smi_device =  static_cast<ISmiDevice*>(new CLT2244(storage));
	return true;
}

bool CLT2244::Recognize(IStorageDevice * storage, BYTE * inquery)
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

		br = CLT2244::LocalRecognize(buf0);
		// Stop vender command
		storage->ScsiRead(buf0, 0x55AA, 1, 60);
	}
	catch (...)
	{
	}

	return br;
}

bool CLT2244::Initialize(void)
{
	LOG_STACK_TRACE();
	BYTE bb = 0;
	// Read Flash ID
	stdext::auto_array<BYTE> buf(SECTOR_SIZE);
	memset(buf, 0, SECTOR_SIZE);
	ReadFlashID(buf, 1);

	m_isp_running = (0x02 == buf[0x0A]);
	m_info_block_valid = (0x01 == buf[0x01]);

	m_mu = buf[0];
	//m_f_block_num = m_mu * 256;
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

	bb = buf[2];
	if (bb & 0x01)			m_f_chunk_size = 4;
	else if (bb & 0x02)		m_f_chunk_size = 8;
	else					m_f_chunk_size = 2;

	memset(buf, 0, SECTOR_SIZE);
	ReadSFR(buf, 1);
	bool slc_mode = ((buf[0x12E] & 20) != 0);

	// Read Info block
	if (m_info_block_valid)
	{
		memset(buf, 0, SECTOR_SIZE);

		// Parameter page for card mode
		CCmdBaseLT2244 cmd(16);
		cmd.id(0xF00A);		// read flash
		//CCmdReadFlash cmd;
		cmd.block(m_info_index[0]);
		cmd.size() = 1;
		VendorCommand(cmd, read, buf, 1);

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
		else if (bb & 0x02)		ph_page_size = 16;
		else if (bb & 0x01)		ph_page_size = 32;
		else					ph_page_size = 0;

		if (bb & 0x08)			m_p_page_per_block = 256;
		else if (bb & 0x10)		m_p_page_per_block = 192;
		else if (bb & 0x20)		m_p_page_per_block = 128;
		else					m_p_page_per_block = 64;

		if (0x98DE9482 == flash_id)
		{
			if (slc_mode) m_p_page_per_block /= 2;		// for 651G8~B (L0315)
		}
		else if (0x98D7A032 == flash_id)	m_p_page_per_block /= 2;
		
		// unnecessary: for 651G4 (L0315) 651G1

		m_f_page_per_block = m_p_page_per_block * m_interleave;	// 
		m_p_chunk_per_page = ph_page_size * m_channel_num / m_f_chunk_size;		
		m_f_chunk_per_page = m_p_chunk_per_page * m_plane;		

		// Read page 1 for f-block number
		cmd.page(1);
		VendorCommand(cmd, read, buf, 1);
		m_f_block_num = MAKEWORD(buf[0xB7], buf[0xB6]);
	}
	return true;
}

void CLT2244::ReadSmartFromWpro(BYTE * data)
{
	// TODO :
	// Search for WPRO block

	// read WPRO id
	//stdext::auto_array<BYTE> sram_buf(SECTOR_SIZE);
	//ReadSRAM(0xB000, sram_buf, SECTOR_SIZE);
	//WORD wpro = MAKEWORD(sram_buf[0xBD], sram_buf[0xBC]);
	//
	//CPhysicalAddress add(wpro, 0, 0, 0, 0);
	////add.m_ph_block = wpro;
	//JCSIZE secs = (m_channel + 1);
	//JCSIZE sec_len = secs * SECTOR_SIZE;
	//stdext::auto_array<BYTE> buf(sec_len * 2);

	//BYTE * buf_ptr[2];
	//buf_ptr[0] = buf, buf_ptr[1] = buf + sec_len;
	//int pre_ptr = 1, cur_ptr = 0;

	////JCSIZE page_per_block = m_max_page * ((m_card_mode & 0x20) ? 2 : 1);
	//JCSIZE page_id_offset = m_channel * SECTOR_SIZE  + 4;
	//for (JCSIZE pp = 0; pp < m_f_page_per_block; ++pp)
	//{
	//	add.m_ph_page = pp;
	//	BYTE * cur_buf = buf_ptr[cur_ptr];
	//	ReadFlashData(add, cur_buf, secs);
	//	// if this page is smart, save
	//	BYTE page_id = cur_buf[page_id_offset];
	//	if (0x01 == page_id)
	//	{
	//		pre_ptr = cur_ptr;
	//		cur_ptr = 1 - pre_ptr;
	//	}
	//	else if (0xFF == page_id) 
	//		break;
	//}
	//memcpy_s(data, SECTOR_SIZE, buf_ptr[pre_ptr], SECTOR_SIZE);
}

void CLT2244::FlashAddToPhysicalAdd(const CFlashAddress & add, CSmiCommand & cmd, UINT option)
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

bool CLT2244::GetProperty(LPCTSTR prop_name, UINT & val)
{
	//JCASSERT(NULL == val);

	if ( FastCmpT(prop_name, CSmiDeviceBase::PROP_CACHE_NUM ) )
	{
		stdext::auto_array<BYTE> buf(SECTOR_SIZE);
		ReadSRAM(0xDE00, buf);
		//val = jcparam::CTypedValue<UINT>::Create( MAKEWORD(buf[0x10D], buf[0x10C]) );
		val = MAKEWORD(buf[0x10D], buf[0x10C]);
		return true;
	}
	else if ( FastCmpT(prop_name, CSmiDeviceBase::PROP_WPRO) )
	{
		stdext::auto_array<BYTE> buf(SECTOR_SIZE);
		ReadSRAM(0xB000, buf);
		//val = jcparam::CTypedValue<UINT>::Create( MAKEWORD(buf[0xBD], buf[0xBC]) );
		val = MAKEWORD(buf[0xBD], buf[0xBC]);
		return true;
	}
	//else if (  FastCmpT(prop_name, _T("FBLOCK_NUM") ) )
	//{

	//}
	else	return __super::GetProperty(prop_name, val);
}

void CLT2244::GetSpare(CSpareData & spare, BYTE * spare_buf)
{
	spare.m_id = spare_buf[0];
	spare.m_hblock = MAKEWORD(spare_buf[2], spare_buf[1]);
	spare.m_hpage = MAKEWORD(spare_buf[4], spare_buf[3]);
	spare.m_erase_count = spare_buf[5];
	
	BYTE * ecc_ch = spare_buf;
	for (BYTE cc = 0; cc < m_channel_num; ++cc, ecc_ch += 16)
	{
		if (ecc_ch[0xC] & 1)	spare.m_error_bit[cc] = 0xFF;
		else					spare.m_error_bit[cc] = ecc_ch[0x0A];
	}
	spare.m_ecc_code = spare_buf[0x40];

	// serial number
	if ( 0x40 == (spare_buf[0] & 0xF0) )
	{
		spare.m_serial_no = spare_buf[8];
	}


}

JCSIZE CLT2244::GetSystemBlockId(JCSIZE id)
{
	switch (id & BID_SYSTEM_MASK)
	{
	case BID_WPRO:		{
		BYTE wpro_add[2];
		ReadSRAM(0xB0BC, 2, wpro_add);
		return MAKEWORD(wpro_add[1], wpro_add[0]);
		}
		
	default:
		return __super::GetSystemBlockId(id);
	}
}