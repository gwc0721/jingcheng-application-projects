#include "stdafx.h"
#include "sm2232.h"

LOCAL_LOGGER_ENABLE(_T("SM2232"), LOGGER_LEVEL_DEBUGINFO);

const CSmartAttrDefTab CSM2232::m_smart_def_2232(CSmartAttrDefTab::RULE()
	( new CSmartAttributeDefine(0x01, _T("Reserved"), false) )
	( new CSmartAttributeDefine(0x02, _T("Reserved"), false) )
	( new CSmartAttributeDefine(0x05, _T("New bads")) )
	( new CSmartAttributeDefine(0x07, _T("Total f-block")) )
	( new CSmartAttributeDefine(0x08, _T("Reserved"), false) )
	( new CSmartAttributeDefine(0x0C, _T("Power cycle")) )
	( new CSmartAttributeDefine(0xC0, _T("Sudden down")) )
	( new CSmartAttributeDefine(0xC3, _T("Reserved"), false) )
	( new CSmartAttributeDefine(0xC4, _T("Reserved"), false) )
	( new CSmartAttributeDefine(0xC5, _T("Reserved"), false) )
	( new CSmartAttributeDefine(0xC6, _T("Reserved"), false) )
	( new CSmartAttributeDefine(0xC7, _T("CRC error")) )
	( new CSmartAttributeDefine(0xC8, _T("Reserved"), false) )
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

bool CSM2232::CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device)
{
	JCASSERT(storage);
	smi_device =  static_cast<ISmiDevice*>(new CSM2232(storage));
	return true;
}

bool CSM2232::Recognize(IStorageDevice * storage, BYTE * inquery/*, ISmiDevice *& smi_device*/)
{
	char vendor[16];
	memset(vendor, 0, 16);
	memcpy_s(vendor, 16, inquery + 8, 16);
	if ( strstr(vendor, "SM333") == NULL && strstr(vendor, "SILICONMOTION") == NULL )	return false;

	// Lock device to send vender command
	storage->DeviceLock();
	// try vendor command
	stdext::auto_array<BYTE> buf0(SECTOR_SIZE);
	storage->ScsiRead(buf0, 0x00AA, 1, 60);
	storage->ScsiRead(buf0, 0xAA00, 1, 60);
	storage->ScsiRead(buf0, 0x0055, 1, 60);
	storage->ScsiRead(buf0, 0x5500, 1, 60);
	storage->ScsiRead(buf0, 0x55AA, 1, 60);

	char * ic_ver = (char*)(buf0 + 0x2E);	
	bool br = ( strstr(ic_ver, "SM223EAD") != NULL );
	// Stop vender command
	storage->ScsiRead(buf0, 0x55AA, 1, 60);
	return br;
}

bool CSM2232::CheckVenderCommand(BYTE * data)
{
	char * ic_ver = (char*)(data + 0x2E);	
	return (NULL != strstr(ic_ver, "SM223EAD") );
}

bool CSM2232::Initialize(void)
{
	LOG_STACK_TRACE();
	// Read Flash ID
	stdext::auto_array<BYTE> buf(SECTOR_SIZE);
	memset(buf, 0, SECTOR_SIZE);
	ReadFlashID(buf, 1);

	switch (buf[0x0A])
	{
	case 0x00:	m_isp_mode = ISPM_ROM_CODE; break;
	case 0x01:	m_isp_mode = ISPM_ISP; break;
	default:	m_isp_mode = ISPM_UNKNOWN;	break;
	}

	m_info_block_valid = (0x01 == buf[0x01]);
	
	m_mu = buf[0x00];
	m_f_block_num = m_mu * 512;

	m_info_index[0] = MAKEWORD(buf[0x181], buf[0x180]);
	m_info_index[1] = MAKEWORD(buf[0x183], buf[0x182]);

	m_isp_index[0] = MAKEWORD(buf[0x185], buf[0x184]);
	m_isp_index[1] = MAKEWORD(buf[0x187], buf[0x186]);

	// Read Info block
	BYTE bb = buf[0x05];

	JCSIZE ph_page_size = 0;

	if (bb & 0x01)		ph_page_size = 4;	// 2K page
	else if (bb & 0x02) ph_page_size = 8;
	else if (bb & 0x04) ph_page_size = 16;

	if (bb & 0x08)		m_p_page_per_block = 64;
	else if (bb & 0x10) m_p_page_per_block = 128;
	else if (bb & 0x20)	m_p_page_per_block = 256;

	bb = buf[0x02];
	m_channel_num = (bb & 0x01) ? 2:1;	// twin

	m_nand_type = (bb & 0x04) ? CCardInfo::NT_MLC : CCardInfo::NT_SLC;
	m_interleave = (bb & 0x20) ? 2:1; 

	m_plane = (buf[0x04] & 0x04) ? 2:1;		

	m_f_chunk_size = m_channel_num * 2;
	JCSIZE fpage = ph_page_size * m_plane * m_channel_num;
	m_f_chunk_per_page = fpage / m_f_chunk_size;
	m_f_page_per_block = m_p_page_per_block * m_interleave;
	//m_p_chunk_per_page = ph_page_size * m_channel_num / m_f_chunk_size;	

	LOG_DEBUG(_T("chunk: %d sec, page: %d sec, block %d sec"), 
		m_f_chunk_size, m_f_chunk_size * m_f_chunk_per_page, 
		m_f_chunk_size * m_f_chunk_per_page * m_f_page_per_block);

	return true;
}

void CSM2232::FlashAddToPhysicalAdd(const CFlashAddress & add, CSmiCommand & cmd, UINT option)
{
	CSmiBlockCmd & block_cmd = reinterpret_cast<CSmiBlockCmd &>(cmd);

	BYTE mode = 0;

	block_cmd.block(add.m_block);
	if (m_interleave > 1)
	{
		block_cmd.page() = add.m_page >> 1;
		mode = (add.m_page & 1) ? 0x40 : 0;
	}
	else	block_cmd.page() = add.m_page;

	if (m_plane > 1)
	{
		//if (add.m_chunk >= m_p_page_size)
		JCSIZE ph_page_size = (m_f_chunk_per_page > 1);		// /2
		if ( add.m_chunk >= ph_page_size )
		{
			block_cmd.sector() = add.m_chunk - ph_page_size;
			mode |= 0x80; 
		}
		else
		{
			block_cmd.sector() = add.m_chunk;
		}
	}
	else block_cmd.sector() = add.m_chunk;
	if ( option & ISmiDevice::RFO_DISABLE_ECC ) mode |= 2;
	block_cmd.mode() = mode;
}

JCSIZE CSM2232::GetNewBadBlocks(BAD_BLOCK_LIST & bad_list)
{
	// find start page
	CFlashAddress add;
	add.m_block = m_info_index[0];

	JCSIZE chunk_len = SECTOR_SIZE * (m_f_chunk_size + 1);
	stdext::auto_array<BYTE> buf(chunk_len);
	char * str_mark = (char*)(buf+0x9A);

	CSpareData spare;

	for ( JCSIZE pp = 0; pp < m_f_page_per_block; ++pp)
	{
		add.m_page = pp;
		ReadFlashChunk(add, spare, buf, m_f_chunk_size + 1);
		if ( strstr(str_mark, "SM223E-AD") == str_mark) break;
	}

	// get bad block count
	JCSIZE new_bad_num = MAKEWORD(buf[0x93], buf[0x92]);

	// Read bad block info
	BYTE * bad_buf = buf + 0x130;
	BYTE * end_of_buf = buf + (SECTOR_SIZE * m_f_chunk_size);

	for (JCSIZE ii = 0; ii < new_bad_num; ++ii)
	{
		bad_list.push_back( CBadBlockInfo(
			MAKEWORD(bad_buf[3], bad_buf[2]),   // block id
			MAKEWORD(bad_buf[5], bad_buf[4]),	// page id
			CBadBlockInfo::BT_RUNTIME,			// type
			bad_buf[0]) );						// error code
		bad_buf += 6;

		if ( bad_buf >= end_of_buf )
		{
			++ add.m_chunk;
			ReadFlashChunk(add, spare, buf, m_f_chunk_size + 1);
			bad_buf = buf;
		}
	}
	return new_bad_num;
}