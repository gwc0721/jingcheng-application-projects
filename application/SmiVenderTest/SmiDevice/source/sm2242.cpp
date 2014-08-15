#include "stdafx.h"
#include "sm2242.h"

LOCAL_LOGGER_ENABLE(_T("SM2242"), LOGGER_LEVEL_ERROR);

// === Flash ID ===
#define	SAMSUNG_ID	0xEC
#define	TOSHIBA_ID	0x98
#define	INFINEON_ID	0xC1
#define	SANDISK_ID	0x45
#define	HYNIX_ID	0xAD
#define	MICRON_ID	0x2C
#define	INTEL_ID	0x89
#define	ST_ID		0x20
#define	AGAND_ID	0x07

const JCSIZE CSM2242::CID_OFFSET = 0x4180;
const JCSIZE CSM2242::ISP_MAX_LEN = 400 * 1024;		// 400KB

const CCidMap CSM2242::m_cid_map(CCidMap::RULE()
//				name,				id,			byte offset,	bit offset, bit len, description
	 ( new CCIDDef(_T("wl_queue"),		CID_WL_QUEUE,		0X0B, 0, 8, _T("Wear-leveling Queue.")) )
	 ( new CCIDDef(_T("wl_delta"),		CID_WL_DELTA,		0X09, 0, 8, _T("Erase count threshold for Wear-leveling.") ) )
	 ( new CCIDDef(_T("data_refresh"),	CID_DATA_REFRESH,	0X2B, 4, 4,	_T("ECC bit for data refresh.")) )
	 ( new CCIDDef(_T("early_retire"),	CID_EARLY_RETIRE,	0X2B, 0, 4, _T("ECC bit for early retire.")) )
	 ( new CCIDDef(_T("lba48"),			CID_LBA48,			0X26, 0, 1, _T("Force enable LBA 48 command.")) )
);

const CSmartAttrDefTab CSM2242::m_smart_def_2242(CSmartAttrDefTab::RULE()
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


bool CSM2242::CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device)
{
	JCASSERT(NULL == smi_device);
	JCASSERT(storage);
	smi_device =  static_cast<ISmiDevice*>(new CSM2242(storage));
	return true;
}

bool CSM2242::Recognize(IStorageDevice * storage, BYTE * inquery)
{
	LOG_STACK_TRACE();

	char vendor[16];
	memset(vendor, 0, 16);
	memcpy_s(vendor, 16, inquery + 8, 15);
	LOG_DEBUG(_T("Checking inquery = %S"), vendor);

	// try vendor command
	LOG_DEBUG(_T("Checking vender command..."));
	bool br = false;
	try
	{
		stdext::auto_array<BYTE> buf0(SECTOR_SIZE);
		storage->ScsiRead(buf0, 0x00AA, 1, 60);
		storage->ScsiRead(buf0, 0xAA00, 1, 60);
		storage->ScsiRead(buf0, 0x0055, 1, 60);
		storage->ScsiRead(buf0, 0x5500, 1, 60);
		storage->ScsiRead(buf0, 0x55AA, 1, 60);

		br = LocalRecognize(buf0);
		// Stop vender command
		storage->ScsiRead(buf0, 0x55AA, 1, 60);
	}
	catch (...)
	{
	}
	return br;
}

CSM2242::CSM2242(IStorageDevice * dev, SMI_DEVICE_TYPE type)
	: CSmiDeviceComm(dev)
{
	LOG_STACK_TRACE();
}

CSM2242::~CSM2242(void)
{
}

void CSM2242::ReadCID(ICidTab * & cid)
{
	JCASSERT(NULL == cid);

	stdext::auto_cif<C2242IspBuf, IIspBuf> isp;
	ReadISP(isp);
	JCASSERT( isp.valid() );
	
	C2242CidTab * _cid=NULL;
	isp->GetCidTab(_cid);
	cid = static_cast<ICidTab*>(_cid);
}

//void CSM2242::ReadISP(IIspBuf * & isp)
//{
//	JCASSERT(NULL == isp);
//	stdext::auto_array<BYTE> isp_buf(ISP_MAX_LEN);
//	JCSIZE isp_len = LoadISP(isp_buf);
//
//	C2242IspBuf * _isp = new C2242IspBuf(isp_len, this);
//	memcpy_s(_isp->Lock(), m_isp_length, isp_buf, m_isp_length);
//	_isp->Unlock();
//	isp = static_cast<IIspBuf*>(_isp);
//}

void CSM2242::DownloadISP(const C2242IspBuf * isp)
{
	// TODO : Fix this function

	//JCASSERT(m_dev);
	//JCASSERT(isp);

	//int download_page = m_sect_page * m_plane * m_channel;

	//bool b64k=false;
	//if (download_page == 128)
	//{
	//	// TODO
	//}

	//// Erase
	//CCmdEraseBlock	erase_cmd;
	//erase_cmd.sector() = 0;
	//erase_cmd.raw_byte(0x06) = 0;
	//erase_cmd.size() = 1;

	//stdext::auto_array<BYTE> dummy_buf(SECTOR_SIZE);

	//JCSIZE len = 1;
	//erase_cmd.isp_block() = m_isp_index[0];
	//VendorCommand(erase_cmd, read, dummy_buf, len);

	//erase_cmd.isp_block() = m_isp_index[1];
	//VendorCommand(erase_cmd, read, dummy_buf, len);

	//// Download
	//JCSIZE isp_len = isp->GetSize();
	//JCASSERT(0 == (isp_len % (download_page * SECTOR_SIZE)) );
	//int download_count = isp_len / (download_page * SECTOR_SIZE);

	//CCmdDownloadIsp	download_cmd(b64k);
	//download_cmd.size() = download_page;

	//// I don't know the reason, just copy from reference code
	//memset(&download_cmd.raw_byte(0x100), 0xE4, 12);

	//const BYTE * isp_buf = isp->Lock();

	//for ( int ll = 0; ll < 2; ++ll )
	//{
	//	JCSIZE offset = 0;
	//	download_cmd.isp_block() = m_isp_index[ll];
	//	for (int ii = 0; ii < download_count; ++ii )
	//	{
	//		if (b64k)
	//		{
	//			download_cmd.page() = ii / 2;
	//			download_cmd.mode() = (ii & 0x1) ? 0x80 : 0x00;
	//		}
	//		else
	//		{
	//			download_cmd.page() = ii;
	//		}
	//		JCSIZE len = download_page;
	//		BYTE * out_buf = const_cast<BYTE*>(isp_buf + offset);
	//		VendorCommand(download_cmd, write, out_buf, len);
	//		offset += download_page * SECTOR_SIZE;
	//	}
	//}

	//Initialize();

	//// Verify
	//stdext::auto_array<BYTE> ver_isp(ISP_MAX_LEN);
	//m_isp_length = isp_len;
	//isp_len = LoadISP(ver_isp);

	//if (memcmp(ver_isp, isp_buf, isp_len) != 0) THROW_ERROR(ERR_APP, _T("verify isp failed") );

	//isp_buf = NULL;
	//isp->Unlock();
}


void CSM2242::GetIspVersion(void)
{
	CSmiCommand cmd_ispver;
	cmd_ispver.id(0xF004);
	cmd_ispver.size() = 1;

	stdext::auto_array<BYTE> buf(SECTOR_SIZE);
	JCSIZE len = 1;
	VendorCommand(cmd_ispver, read, buf, len);
	memcpy(m_isp_ver, buf + 0x190, 0x10);
}

void CSM2242::ReadSmartFromWpro(BYTE * data)
{
	// read WPRO id
	//stdext::auto_array<BYTE> sram_buf(SECTOR_SIZE);
	BYTE wpro_add[2];

	ReadSRAM(0xEE38, 2, wpro_add);
	//WORD wpro = MAKEWORD(sram_buf[0x39], sram_buf[0x38]);
	WORD wpro = MAKEWORD(wpro_add[1], wpro_add[0]);

// TODO : physical add -> flash add
	//CFlashAddress add(wpro, 0, 0, 0, 0);
	//// find out last smart page	
	//JCSIZE sec_len = (m_channel + 1) * SECTOR_SIZE;
	//stdext::auto_array<BYTE> buf(sec_len * 2);

	//BYTE * buf_ptr[2];
	//buf_ptr[0] = buf, buf_ptr[1] = buf + sec_len;
	//int pre_ptr = 1, cur_ptr = 0;

	//JCSIZE page_per_block = m_max_page * ((m_card_mode & 0x20) ? 2 : 1);
	//JCSIZE page_id_offset = m_channel * SECTOR_SIZE  + 3;
	//for (JCSIZE pp = 0; pp < page_per_block; ++pp)
	//{
	//	add.m_ph_page = pp;
	//	BYTE * cur_buf = buf_ptr[cur_ptr];
	//	ReadFlashData(add, cur_buf, sec_len);
	//	BYTE page_id = cur_buf[page_id_offset];
	//	// if this page is smart, save
	//	if (0x01 == page_id)
	//	{
	//		pre_ptr = cur_ptr;
	//		cur_ptr = 1 - pre_ptr;
	//	}
	//	else if (0xFF == page_id) break;
	//}
	//memcpy_s(data, SECTOR_SIZE, buf_ptr[pre_ptr], SECTOR_SIZE);
}

JCSIZE CSM2242::GetSystemBlockId(JCSIZE id)
{
	switch (id & BID_SYSTEM_MASK)
	{
	case BID_WPRO:		{
		BYTE wpro_add[2];
		ReadSRAM(0xEE38, 2, wpro_add);
		return MAKEWORD(wpro_add[1], wpro_add[0]);
		}
		
	default:
		return __super::GetSystemBlockId(id);
	}
}


bool CSM2242::GetProperty(LPCTSTR prop_name, UINT & val)
{
	if ( FastCmpT(CSmiDeviceBase::PROP_WPRO, prop_name) )
	{
		stdext::auto_array<BYTE> buf(SECTOR_SIZE);
		ReadSRAM(0xEE00, buf);
		val = MAKEWORD(buf[0x39], buf[0x38]);
		return true;
	}
	else	return __super::GetProperty(prop_name, val);
}

JCSIZE CSM2242::GetNewBadBlocks(BAD_BLOCK_LIST & bad_list)
{
	// find start page
	CFlashAddress add;
	add.m_block = m_info_index[0];

	JCSIZE chunk_len = SECTOR_SIZE * (m_f_chunk_size + 1);
	stdext::auto_array<BYTE> buf(chunk_len);
	char * str_mark = (char*)(buf+0x120);

	CSpareData spare;

	for ( JCSIZE pp = 0; pp < m_f_page_per_block; ++pp)
	{
		add.m_page = pp;
		ReadFlashChunk(add, spare, buf, m_f_chunk_size + 1);
		if ( strstr(str_mark, "SM2242") == str_mark) break;
	}

	// get bad block count
	JCSIZE new_bad_num = MAKEWORD(buf[0x133], buf[0x132]);

	// Read bad block info
	BYTE * bad_buf = buf + 0x1D0;
	BYTE * end_of_buf = buf + (SECTOR_SIZE * m_f_chunk_size);

	for (JCSIZE ii = 0; ii < new_bad_num; ++ii)
	{
		bad_list.push_back( CBadBlockInfo(
			MAKEWORD(bad_buf[2], bad_buf[1]),  
			bad_buf[3], 
			CBadBlockInfo::BT_RUNTIME, 
			bad_buf[0]) );
		bad_buf += 4;

		if ( bad_buf >= end_of_buf )
		{
			++ add.m_chunk;
			ReadFlashChunk(add, spare, buf, m_f_chunk_size + 1);
			bad_buf = buf;
		}
	}
	return new_bad_num;
}

void CSM2242::GetSpare(CSpareData & spare, BYTE * spare_buf)
{
	spare.m_id = spare_buf[0];
	spare.m_hblock = MAKEWORD(spare_buf[2], spare_buf[1]);
	spare.m_hpage = spare_buf[3];

	spare.m_ecc_code = spare_buf[16];
	memcpy_s(spare.m_error_bit, 8, spare_buf + 18, 4);
}




///////////////////////////////////////////////////////////////////////////////
//-- ISP and CID
C2242IspBuf::C2242IspBuf(JCSIZE buf_size, CSM2242 * sm2242)
	: CBinaryBuffer(buf_size)
	, m_sm2242(sm2242)
{
	JCASSERT(m_sm2242);
	m_sm2242->AddRef();
}

C2242IspBuf::~C2242IspBuf(void)
{
	m_sm2242->Release();
}

void C2242IspBuf::Save(void)
{
	JCASSERT(m_sm2242);
	m_sm2242->DownloadISP(this);
}

void C2242IspBuf::GetCidTab(C2242CidTab * & cid_buf)
{
	JCASSERT(NULL == cid_buf);
	cid_buf = new C2242CidTab(Lock() + CSM2242::CID_OFFSET, CSM2242::CID_LENGTH, this);
	Unlock();
}

void C2242IspBuf::SetCidTab(BYTE * buf, JCSIZE len)
{
	JCASSERT(buf);
	BYTE * isp_buf = Lock();
	memcpy_s(isp_buf + CSM2242::CID_OFFSET, CSM2242::CID_LENGTH, buf, len);
	Unlock();
}



C2242CidTab::C2242CidTab(BYTE * buf, JCSIZE length, C2242IspBuf * isp)
	: m_isp(isp)
	, m_buf(NULL)
{
	JCASSERT(buf);
	//memcpy_s(m_cid, CSM2242::CID_LENGTH, buf, length);
	CBinaryBuffer::Create(CSM2242::CID_LENGTH, m_buf);
	memcpy_s(m_buf->Lock(), CSM2242::CID_LENGTH, buf, length);
	m_buf->Unlock();

	JCASSERT(m_isp);
	m_isp->AddRef();
}

C2242CidTab::~C2242CidTab(void)
{
	JCASSERT(m_isp);
	m_isp->Release();

	JCASSERT(m_buf);
	m_buf->Release();
}

static const BYTE BIT_MASK[8] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};

DWORD C2242CidTab::GetValue(const CJCStringT & name)
{
	const CCIDDef * def = CSM2242::m_cid_map.GetItem(name);
	if (!def) THROW_ERROR(ERR_PARAMETER, _T("Unknow item name %s"), name.c_str());
	BYTE * buf = m_buf->Lock();
	
	DWORD val = 0;
	if (def->m_bit_length >= 8)
	{	// 按字节存储
		JCSIZE bytes = def->m_bit_length / 8;	// bytes
		for (JCSIZE ii = def->m_byte_offset; ii < (def->m_byte_offset + bytes); ++ii)
		{
			val <<= 8;
			val |= buf[ii];
		}
	}
	else
	{	// 按位存储，不支持跨字节位存储
		BYTE mask = BIT_MASK[def->m_bit_length];
		val = (buf[def->m_byte_offset] >> def->m_bit_offset) & mask;
	}
	m_buf->Unlock();

	return val;
}

void C2242CidTab::SetValue(const CJCStringT & name, DWORD val)
{
//	static const BYTE BIT_MASK[8] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};
	const CCIDDef * def = CSM2242::m_cid_map.GetItem(name);
	if (!def) THROW_ERROR(ERR_PARAMETER, _T("Unknow item name %s"), name.c_str());

	BYTE * buf = m_buf->Lock();

	if (def->m_bit_length >= 8)
	{	// 按字节存储
		JCSIZE bytes = def->m_bit_length / 8;	// bytes
		for (JCSIZE ii = def->m_byte_offset + bytes-1; ii >= def->m_byte_offset; --ii)
			buf[ii] = BYTE(val & 0xFF), val >>= 8;
	}
	else
	{	// 按位存储，不支持跨字节位存储
		BYTE mask = BIT_MASK[def->m_bit_length] << def->m_bit_offset;
		buf[def->m_byte_offset] &= ~mask;
		buf[def->m_byte_offset] |= (BYTE)(val << def->m_bit_offset) & mask;
	}
	m_buf->Unlock();
}

void C2242CidTab::Save(void)
{
	JCASSERT(m_isp);
	m_isp->SetCidTab(m_buf->Lock(), CSM2242::CID_LENGTH);
	m_isp->Save();
	m_buf->Unlock();
}

//void C2242CidTab::Format(FILE * file, LPCTSTR format)
//{
//	JCASSERT(m_buf);
//	if ( NULL == format || FastCmpT(EMPTY, format) )	format = _T("raw");
//	if ( FastCmpT(_T("raw"), format) )
//	{
//		m_buf->Format(file, _T("text") );
//	}
//	else if ( FastCmpT(_T("text"), format) )
//	{
//	}
//	else if ( FastCmpT(_T("bin"), format) )
//	{
//		m_buf->Format(file, format);
//	}
//}
//
//bool C2242CidTab::QueryInterface(const char * if_name, IJCInterface * &if_ptr)
//{
//	JCASSERT(NULL == if_ptr);
//	bool br = false;
//	if ( FastCmpA(jcparam::IF_NAME_VALUE_FORMAT, if_name) )
//	{
//		if_ptr = static_cast<IJCInterface*>(this);
//		if_ptr->AddRef();
//		br = true;
//	}
//	else br = __super::QueryInterface(if_name, if_ptr);
//	return br;
//}

bool CSM2242::Initialize(void)
{
	LOG_STACK_TRACE();
	// Read Flash ID
	stdext::auto_array<BYTE> buf(SECTOR_SIZE);
	memset(buf, 0, SECTOR_SIZE);
	ReadFlashID(buf, 1);
	//ReadFlashID(buf);

	switch (buf[0x0A])
	{
	case 0x00:	m_isp_mode = ISPM_ROM_CODE; break;
	case 0x01:	m_isp_mode = ISPM_ISP; break;
	default:	m_isp_mode = ISPM_UNKNOWN;	break;
	}

	m_info_block_valid = (0x01 == buf[0x01]);

	m_mu = buf[0];
	m_f_block_num = m_mu * 256;
	m_channel_num = (buf[2] & 0x01)? 4 : 2;

	m_info_index[0] = MAKEWORD(buf[0x181], buf[0x180]);
	m_info_index[1] = MAKEWORD(buf[0x183], buf[0x182]);

	m_isp_index[0] = MAKEWORD(buf[0x185], buf[0x184]);
	m_isp_index[1] = MAKEWORD(buf[0x187], buf[0x186]);

	m_f_chunk_size = m_channel_num;

	ReadSFR(buf, 1);
	m_plane = (buf[0x122] & 0x10) ? 2:1;

	// Read Info block
	if (m_info_block_valid)
	{
		memset(buf, 0, SECTOR_SIZE);

		// Read 1st sector in info block. Since card mode is not initialized, use vendor command directorly.
		CCmdReadFlash cmd;
		cmd.block(m_info_index[0]);
		cmd.size() = 1;
		VendorCommand(cmd, read, buf, 1);

		BYTE bb = buf[0x0A];
		m_interleave = (bb & 0x03) ? 2:1;
		
		if (buf[0x0A] & 0x40)		m_nand_type = CCardInfo::NT_SLC_MODE;
		else if	( buf[0x0C] & 0x40)	m_nand_type = CCardInfo::NT_MLC;
		else						m_nand_type = CCardInfo::NT_SLC;

		JCSIZE ph_page_size = 0;

		ph_page_size = buf[0x19] * 2;
		m_p_page_per_block = buf[0x18] * 64;		
		if (CCardInfo::NT_SLC_MODE == m_nand_type) m_p_page_per_block /= 2;

		m_f_page_per_block = m_p_page_per_block * m_interleave;	// 
		m_p_chunk_per_page = ph_page_size;
		m_f_chunk_per_page = ph_page_size * m_plane;
	}
	return true;
}

void CSM2242::FlashAddToPhysicalAdd(const CFlashAddress & add, CSmiCommand & cmd, UINT option)
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
		if (add.m_chunk >= ph_page_size)
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
	//if (false == ecc)		mode |= 2;
	if ( option & ISmiDevice::RFO_DISABLE_ECC ) mode |= 2;
	block_cmd.mode() = mode;
}


