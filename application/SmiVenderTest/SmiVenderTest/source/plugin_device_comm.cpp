#include "stdafx.h"

#include "plugin_device_comm.h"
#include "plugin_debug.h"

#include "application.h"

LOCAL_LOGGER_ENABLE(_T("plugin_device"), LOGGER_LEVEL_WARNING);

void OutPutStringRow(jcscript::IOutPort * outport, const CJCStringT & str)
{
	jcparam::IValue * val = jcparam::CTypedValue<CJCStringT>::Create(str);
	outport->PushResult(val);
	val->Release();
}

bool RegisterPlugin(jcscript::IPluginContainer * plugin_cont)
{
	JCASSERT(plugin_cont);

	stdext::auto_interface<CCategoryComm> cat(new CCategoryComm(_T("device")) );

	cat->RegisterFeatureT<CDeviceReadCtrl>();
	cat->RegisterFeatureT<CDeviceReadSpare>();
	cat->RegisterFeatureT<CDeviceReadFlash>();
	cat->RegisterFeatureT<CDeviceOriginalBad>();
	cat->RegisterFeatureT<CDevicePhOriginalBad>();
	cat->RegisterFeatureT<CDeviceNewBad>();
	cat->RegisterFeatureT<CDeviceDiffAdd>();
	cat->RegisterFeatureT<CDeviceInfo>();
	cat->RegisterFeatureT<CDeviceRestoreInfo>();
	cat->RegisterFeatureT<CDeviceReadSmart>();
	cat->RegisterFeatureT<CDeviceIdentify>();
	cat->RegisterFeatureT<CDeviceCleanCache>();
	cat->RegisterFeatureT<CDeviceReset>();
	cat->RegisterFeatureT<CSystemSleep>();
	cat->RegisterFeatureT<CDeviceSmartDecode>();
	cat->RegisterFeatureT<CDeviceDumpPage>();
	cat->RegisterFeatureT<CDeviceDumpFilter>();
	cat->RegisterFeatureT<CDeviceEraseCount>();

	plugin_cont->RegistPlugin(cat);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// -- 
LPCTSTR CDeviceReadCtrl::_BASE_TYPE::m_feature_name = _T("readctrl");
CParamDefTab CDeviceReadCtrl::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#00"), 0, offsetof(CDeviceReadCtrl, m_mode) ) )
	(new CTypedParamDef<JCSIZE>(_T("address"), 'a', offsetof(CDeviceReadCtrl, m_address) ) )
	(new CTypedParamDef<JCSIZE>(_T("length"), 'l', offsetof(CDeviceReadCtrl, m_length) ) )
	(new CTypedParamDef<JCSIZE>(_T("bank"), 'k', offsetof(CDeviceReadCtrl, m_bank) ) )
	);

const TCHAR CDeviceReadCtrl::m_desc[] = _T("Read data from controller, SRAM, FlashID, SFR, PAR.");

CDeviceReadCtrl::CDeviceReadCtrl(void)
	: m_smi_dev(NULL)
	, m_address(0)
	, m_length(1)
	, m_bank(0)
{
}

CDeviceReadCtrl::~CDeviceReadCtrl(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}

bool CDeviceReadCtrl::Init(void)
{
	if ( !m_smi_dev )
	{
		CSvtApplication::GetApp()->GetDevice(m_smi_dev);
		//m_smi_dev->GetCardInfo(m_card_info);
	}
	return true;
}

bool CDeviceReadCtrl::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	stdext::auto_interface<CSectorBuf> buf;

	if (m_mode == _T("SRAM"))
	{
		JCSIZE ram_add = m_address & 0xFE00;
		CreateSectorBuf(ram_add >> 9, buf);
		m_smi_dev->ReadSRAM(ram_add, m_bank, SECTOR_SIZE, buf->Lock() );
		buf->Unlock();
		outport->PushResult(buf);
	}
	else if (m_mode == _T("SFR"))
	{
		CreateSectorBuf(0, buf);
		m_smi_dev->ReadSFR(buf->Lock(), 1);
		buf->Unlock();
		outport->PushResult(buf);
	}
	else if (m_mode == _T("FID"))
	{
		CreateSectorBuf(0, buf);
		m_smi_dev->ReadFlashID(buf->Lock(), 1);
		buf->Unlock();
		outport->PushResult(buf);
	}
	else
	{
		THROW_ERROR(ERR_USER, _T("invalid control data type %s"), m_mode);
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// -- read spare

LPCTSTR CDeviceReadSpare::_BASE_TYPE::m_feature_name = _T("sparedata");
CParamDefTab CDeviceReadSpare::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("block"),	'b', offsetof(CDeviceReadSpare, m_block) ) )
	(new CTypedParamDef<UINT>(_T("page"),	'p', offsetof(CDeviceReadSpare, m_page) ) )
	(new CTypedParamDef<bool>(_T("all"),	'h', offsetof(CDeviceReadSpare, m_read_all) ) )
	(new CTypedParamDef<bool>(_T("ecc"),	'e', offsetof(CDeviceReadSpare, m_read_ecc) ) )
	//(new CTypedParamDef<BYTE>(_T("option"),	'o', offsetof(CDeviceReadSpare, m_option) ) )
	);

const TCHAR CDeviceReadSpare::m_desc[] = _T("Read meta data");

CDeviceReadSpare::CDeviceReadSpare(void)
	: m_page(0xFFFF), m_page_num(0), m_smi_dev(NULL)
	, m_block(0)
	, m_read_all(false)
	, m_read_ecc(false)
	, m_cur_page(0)
	, m_option(0)
{
}

CDeviceReadSpare::~CDeviceReadSpare(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}

bool CDeviceReadSpare::Init(void)
{
	LOG_STACK_TRACE();
	if (m_smi_dev) m_smi_dev->Release(), m_smi_dev=NULL;

	CSvtApplication::GetApp()->GetDevice(m_smi_dev);

	CCardInfo info;
	m_smi_dev->GetCardInfo(info);

	m_chunk_size = info.m_f_spck + 1;

	if (0xFFFF == m_page)	m_cur_page = 0,			m_page_num = info.m_f_ppb;
	else					m_cur_page = m_page,	m_page_num = 1;

	m_channel_num = info.m_channel_num;
	m_chunk_num = info.m_f_ckpp;

	return true;
}

bool CDeviceReadSpare::InternalInvoke(jcparam::IValue *, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	// 如果指定长度：所有地址的缺省值为0，读取指定长度。
	// 如果没有指定长度：
	//		如果没有指定 page，读取整个block

	JCASSERT(m_smi_dev);
	// 如果指定page，则读取特定page的meta data，否则读取整个block的meta data

	JCSIZE buf_size = m_chunk_size * SECTOR_SIZE;
	stdext::auto_array<BYTE> buf(buf_size);

	CFlashAddress add(m_block, m_cur_page, 0);

	CSpareData spare;
	m_smi_dev->ReadFlashChunk(add, spare, buf, m_chunk_size);

	if ( 0xFF != spare.m_id || m_read_all )
	{
		stdext::auto_interface<BLOCK_ROW> block_row(new BLOCK_ROW);
		block_row->SetAddress(add);
		block_row->SetSpare(spare);

		if (m_read_ecc)
		{	// read all chunks for error bit
			block_row->CreateErrorBit(m_channel_num, m_chunk_num);
			block_row->SetErrorBit(spare, 0);

			for (BYTE chunk = 1; chunk < m_chunk_num; ++ chunk)
			{
				add.m_chunk = chunk;
				m_smi_dev->ReadFlashChunk(add, spare, buf, m_chunk_size);
				block_row->SetErrorBit(spare, chunk);
			}
		}
		outport->PushResult(block_row);
	}

	m_cur_page ++;
	m_page_num --;
	if (0 == m_page_num)	return false;
	else return (true);
}

///////////////////////////////////////////////////////////////////////////////
// -- read flash
LPCTSTR CDeviceReadFlash::_BASE_TYPE::m_feature_name = _T("readflash");
CParamDefTab CDeviceReadFlash::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("block"),	'b', offsetof(CDeviceReadFlash, m_block) ) )
	(new CTypedParamDef<UINT>(_T("page"),	'p', offsetof(CDeviceReadFlash, m_page) ) )
	(new CTypedParamDef<UINT>(_T("chunk"),	'k', offsetof(CDeviceReadFlash, m_chunk), _T("chunk address") ) )
	(new CTypedParamDef<UINT>(_T("count"),	'n', offsetof(CDeviceReadFlash, m_count), _T("data length in sector") ))

	(new CTypedParamDef<bool>(_T("all"),	'h', offsetof(CDeviceReadFlash, m_read_all) ) )
	//(new CTypedParamDef<bool>(_T("ecc"),	'e', offsetof(CDeviceReadFlash, m_read_ecc) ) )
		//PARDEF(_T("ecc"),	e, VT_BOOL,		_T("disable ecc engine.") )
		//PARDEF(_T("scramb"),s, VT_UINT,		_T("disable scramble.") )
	);
const TCHAR CDeviceReadFlash::m_desc[] = _T("read flash raw data");



CDeviceReadFlash::CDeviceReadFlash(void)
	: m_block(0), m_page(0), m_chunk(UINT_MAX)
	, m_count(UINT_MAX)
	, m_smi_dev(NULL)
	, m_read_all(false)
{
}

CDeviceReadFlash::~CDeviceReadFlash(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}

bool CDeviceReadFlash::Init(void)
{
	LOG_STACK_TRACE();
	if (m_smi_dev) m_smi_dev->Release(), m_smi_dev=NULL;
	CSvtApplication::GetApp()->GetDevice(m_smi_dev);
	m_smi_dev->GetCardInfo(m_card_info);

	if (m_chunk >= m_card_info.m_f_ckpp)	m_cur_chunk = 0, m_last_chunk = m_card_info.m_f_ckpp;
	else									m_cur_chunk = m_chunk, m_last_chunk = m_cur_chunk + 1;
	UINT chunks = m_card_info.m_f_ckpp - m_cur_chunk;
	
	if (m_count > m_card_info.m_f_spck)	m_count = m_card_info.m_f_spck;
	return true;
}

bool CDeviceReadFlash::InternalInvoke(jcparam::IValue *, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	JCSIZE chunk_size = m_card_info.m_f_spck + 1;
	JCSIZE buf_size = chunk_size  * SECTOR_SIZE;
	UINT option = 0;

	stdext::auto_array<BYTE> buf(buf_size);
	CSpareData spare;

	CFlashAddress add(m_block, m_page, m_cur_chunk);
	m_smi_dev->ReadFlashChunk(add, spare, buf, chunk_size, option);
	if ( (0xFF==spare.m_id) && (0xFFFF==spare.m_hblock) && (!m_read_all) )	
		return false;

	JCSIZE len = m_card_info.m_f_spck * SECTOR_SIZE;

	CSectorBuf * data = NULL;
	for (JCSIZE ss = 0; ss < m_count; ++ss)
	{	// 每次返回 512 byte, 显示时每次显示512 byte，: 控制前后显示
		JCSIZE sec_add = m_card_info.m_f_spck * m_cur_chunk + ss;
		CreateSectorBuf(sec_add, data);
		memcpy_s(data->Lock(), SECTOR_SIZE, buf + (ss * SECTOR_SIZE), SECTOR_SIZE);
		data->Unlock();
		outport->PushResult(data);
		data->Release(), data = NULL;
	}

	m_cur_chunk ++;
	if (m_cur_chunk >= m_last_chunk)	return false;
	else								return true;
}

///////////////////////////////////////////////////////////////////////////////
// -- original bad block
LPCTSTR CDeviceOriginalBad::_BASE_TYPE::m_feature_name = _T("orgbad");
CParamDefTab CDeviceOriginalBad::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("block"),	'b', offsetof(CDeviceOriginalBad, m_block), _T("specify info block id") ) )
	(new CTypedParamDef<UINT>(_T("page"),	'p', offsetof(CDeviceOriginalBad, m_page), _T("specify bit map page id") ) )
	);
const TCHAR CDeviceOriginalBad::m_desc[] = _T("read original bad f-block bit map LT2244 only");



CDeviceOriginalBad::CDeviceOriginalBad(void)
	: m_block(0xFFFFFFFF), m_page(0xFFFFFFFF)
	, m_smi_dev(NULL)
{
}

CDeviceOriginalBad::~CDeviceOriginalBad(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}

bool CDeviceOriginalBad::Init(void)
{
	LOG_STACK_TRACE();
	if (m_smi_dev) m_smi_dev->Release(), m_smi_dev=NULL;
	CSvtApplication::GetApp()->GetDevice(m_smi_dev);
	m_smi_dev->GetCardInfo(m_card_info);

	return true;
}

bool CDeviceOriginalBad::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	// get info block id and page id
	UINT val, info_block, bitmap_page;
	if (0xFFFFFFFF == m_block)
	{	
		m_smi_dev->GetProperty(_T("INFO_BLOCK"), val);
		info_block = HIWORD(val);
	}
	else info_block = m_block;

	if (0xFFFFFFFF == m_page)
	{
		m_smi_dev->GetProperty(_T("INFO_PAGE"), val);
		bitmap_page = LOBYTE( HIWORD(val) );
	}
	else bitmap_page = m_page;

	//
	JCSIZE chunk_size = m_card_info.m_f_spck + 1;
	JCSIZE buf_size = chunk_size  * SECTOR_SIZE;

	stdext::auto_array<BYTE> buf(buf_size);
	CSpareData spare;

	CFlashAddress add(info_block, bitmap_page, 0);
	m_smi_dev->ReadFlashChunk(add, spare, buf, chunk_size);

	JCSIZE len = m_card_info.m_f_block_num / 8 + 1;

	//jcparam::IValue * block_id = NULL;
	BLOCK_ROW * block_row = NULL;
	for (JCSIZE ii = 0; ii < len; ++ ii)
	{
		BYTE dd = buf[ii];
		BYTE mask = 1;
		for (JCSIZE jj = 0; jj < 8; ++ jj, mask <<= 1)
		{
			if (dd & mask)
			{
				block_row = new BLOCK_ROW;
				block_row->SetAddress(CFlashAddress(ii * 8 + jj));
				outport->PushResult(block_row);
				block_row->Release(), block_row = NULL;
			}
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// -- physical original bad block
LPCTSTR CDevicePhOriginalBad::_BASE_TYPE::m_feature_name = _T("phorgbad");
CParamDefTab CDevicePhOriginalBad::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("block"),	'b', offsetof(CDevicePhOriginalBad, m_block), _T("specify original block info id") ) )
	(new CTypedParamDef<UINT>(_T("page"),	'p', offsetof(CDevicePhOriginalBad, m_page), _T("starte page of original info") ) )
	(new CTypedParamDef<bool>(_T("order"),	'o', offsetof(CDevicePhOriginalBad, m_order_phy), _T("starte page of original info") ) )
	);
const TCHAR CDevicePhOriginalBad::m_desc[] = _T("read physical original bad block bit map LT2244 only");

CDevicePhOriginalBad::CDevicePhOriginalBad(void)
	: m_block(0xFFFFFFFF), m_page(0)
	, m_smi_dev(NULL)
	, m_order_phy(false)
{
}

CDevicePhOriginalBad::~CDevicePhOriginalBad(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}

bool CDevicePhOriginalBad::Init(void)
{
	LOG_STACK_TRACE();
	if (m_smi_dev) m_smi_dev->Release(), m_smi_dev=NULL;
	CSvtApplication::GetApp()->GetDevice(m_smi_dev);
	m_smi_dev->GetCardInfo(m_card_info);

	if (0xFFFFFFFF == m_block)		m_smi_dev->GetProperty(_T("ORG_BAD_INFO"), m_org_info);
	else							m_org_info = m_block;

	return true;
}

bool CDevicePhOriginalBad::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	// get info block id and page id
	JCSIZE chunk_size = m_card_info.m_f_spck;
	JCSIZE buf_size = chunk_size  * SECTOR_SIZE;

	stdext::auto_array<BYTE> buf(buf_size + SECTOR_SIZE);
	CSpareData spare;

	typedef std::vector<WORD>	BLOCK_LIST;
	typedef BLOCK_LIST::iterator	BLOCK_IT;
	BLOCK_LIST	phy_org_blocks[32];		// 4 channel, 4 ce, two plane

	// 计算page size
	JCSIZE page_size = m_card_info.m_f_spck * m_card_info.m_f_ckpp;
	// 计算64KB需要多少page
	JCSIZE start_page = 128 / page_size;
	m_page = start_page * m_card_info.m_interleave;

	UINT m_chunk = 0;
	JCSIZE block_id = 0;
	LOG_DEBUG(_T("blocks per die =%d"), m_card_info.m_p_bpd);
	while (1)
	{
		LOG_DEBUG(_T("read flash block = %d, page = %d, chunk = %d"), m_org_info, m_page, m_chunk);
		CFlashAddress add(m_org_info, m_page, m_chunk);
		m_smi_dev->ReadFlashChunk(add, spare, buf, chunk_size + 1);
		
		DWORD * _buf = (DWORD*)((BYTE*)buf);
		JCSIZE blk_in_chunk = buf_size / sizeof(DWORD);

		for (JCSIZE bb = 0; bb < blk_in_chunk; ++bb, ++block_id, ++_buf)
		{
			DWORD d = *_buf;
			if ( 0 == d ) continue;

			DWORD die_mask = 1;
			//WORD die_id = 0;
			TCHAR _str[512];
			LPTSTR str = _str;
			JCSIZE remain = 512;
			int ir = stdext::jc_sprintf(str, remain, _T("%04X,"), block_id);
			str += ir;
			remain -= ir;

			for (int ce = 0; ce < 4; ++ce)
			{
				for (JCSIZE plane = 0; plane < 2; ++plane)
				{
					for (JCSIZE ch = 0; ch < 4; ++ ch, die_mask <<= 1/*, die_id++*/)
					{
						if ( (ch >= m_card_info.m_channel_num) ) continue;
						if (d & die_mask)	
						{
							ir = _stprintf_s(str, remain, _T("CE%d PL%d CH%c,"), ce, plane, _T('A')+ch);
							
							JCSIZE ce_x = ce;
							JCSIZE block_id_x = block_id;
							if (block_id >= m_card_info.m_p_bpd)
							{
								ce_x = ce + block_id / m_card_info.m_p_bpd * m_card_info.m_interleave;
								block_id_x = block_id % m_card_info.m_p_bpd;
							}
							
							JCSIZE die_id = (ce_x * 2 + plane) * 4 + ch;
							JCASSERT(die_id < 32);
							phy_org_blocks[die_id].push_back((WORD)block_id_x);
						}
						else				ir = _stprintf_s(str, remain, _T("*** *** ***,") );
						str += ir;
						remain -= ir;
					}
				}
			}
			// 按f-block排序
			if (!m_order_phy)		OutPutStringRow(outport, _str);
		}

		//if (block_id >= m_card_info.m_p_bpd) break;
		if (block_id >= m_card_info.m_f_block_num) break;
		m_chunk ++;
		if (m_chunk >= m_card_info.m_f_ckpp )	m_page += m_card_info.m_interleave, m_chunk=0;
		//m_page +=2;
	}

	if (m_order_phy)
	{
		int dies = m_card_info.m_p_die / m_card_info.m_channel_num;
		for (int ce = 0; ce < dies; ++ce)
		{
			for (JCSIZE plane = 0; plane < 2; ++plane)
			{
				for (JCSIZE ch = 0; ch < m_card_info.m_channel_num; ++ ch)
				{
					TCHAR _str[32];
					_stprintf_s(_str, _T("#CE%d PL%d CH%c,"), ce, plane, _T('A')+ch);
					OutPutStringRow(outport, _str);

					JCSIZE die_id = (ce * 2 + plane) * 4 + ch;
					BLOCK_IT it = phy_org_blocks[die_id].begin();
					BLOCK_IT endit = phy_org_blocks[die_id].end();
					for ( ; it != endit; ++it)
					{
						_stprintf_s(_str, _T("%04X"), *it);
						OutPutStringRow(outport, _str);
					}
				}
			}
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// -- check new bad block list in info
LPCTSTR CDeviceNewBad::_BASE_TYPE::m_feature_name = _T("newbad");
CParamDefTab CDeviceNewBad::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("block"),	'b', offsetof(CDeviceNewBad, m_block), _T("specify info block id") ) )
	(new CTypedParamDef<UINT>(_T("page"),	'p', offsetof(CDeviceNewBad, m_page), _T("specify bit map page id") ) )
	);
const TCHAR CDeviceNewBad::m_desc[] = _T("read new bad from info block");



CDeviceNewBad::CDeviceNewBad(void)
	: m_block(0xFFFFFFFF), m_page(0xFFFFFFFF)
{
}

bool CDeviceNewBad::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	//!! FOR 2244LT only
	LOG_STACK_TRACE();

	stdext::auto_interface<ISmiDevice> dev;
	CSvtApplication::GetApp()->GetDevice(dev);
	JCASSERT( dev.valid() );
	CCardInfo card_info;
	dev->GetCardInfo(card_info);

	// get info block id and page id
	UINT val, info_block, info_page;
	if (0xFFFFFFFF == m_block)
	{	
		dev->GetProperty(_T("INFO_BLOCK"), val);
		info_block = HIWORD(val);
	}
	else info_block = m_block;

	if (0xFFFFFFFF == m_page)
	{
		dev->GetProperty(_T("INFO_PAGE"), val);
		info_page = HIBYTE( HIWORD(val) );
	}
	else info_page = m_page;

	//
	JCSIZE chunk_size = card_info.m_f_spck + 1;
	JCSIZE buf_size = chunk_size  * SECTOR_SIZE;

	stdext::auto_array<BYTE> buf(buf_size);
	CSpareData spare;

	CFlashAddress add(info_block, info_page, 0);
	dev->ReadFlashChunk(add, spare, buf, chunk_size);

	// get new bad block number
	JCSIZE newbad_num = MAKEWORD(buf[0xB3], buf[0xB2]);
	LOG_DEBUG(_T("new bad block num = %d"), newbad_num);

	// read new bad block info
	JCSIZE chunk = 0;
	JCSIZE offset = 0x1D0;
	while (newbad_num != 0)
	{
		BYTE * ptr = buf + offset;
		stdext::auto_interface<CNewBadBlockRow>	new_bad_block( new CNewBadBlockRow(
			CNewBadBlock(MAKEWORD(ptr[2], ptr[1]), MAKEWORD(ptr[4], ptr[3]), ptr[0] ) ));
		outport->PushResult(new_bad_block);
		offset += 8;
		if (offset >= (card_info.m_f_spck * SECTOR_SIZE) )
		{	// read next chunk
			chunk ++;
			if (chunk >= card_info.m_f_ckpp) break;
			CFlashAddress add(info_block, info_page, chunk);
			dev->ReadFlashChunk(add, spare, buf, chunk_size);
			offset = 0;
		}
		newbad_num --;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// -- difference address
LPCTSTR CDeviceDiffAdd::_BASE_TYPE::m_feature_name = _T("diffadd");
CParamDefTab CDeviceDiffAdd::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("block"),	'b', offsetof(CDeviceDiffAdd, m_block), _T("specify original block info id") ) )
	//(new CTypedParamDef<UINT>(_T("page"),	'p', offsetof(CDeviceDiffAdd, m_page), _T("starte page of original info") ) )
	);
const TCHAR CDeviceDiffAdd::m_desc[] = _T("calculate difference address");

CDeviceDiffAdd::CDeviceDiffAdd(void)
	: m_block(0xFFFFFFFF), m_page(0)
	, m_smi_dev(NULL)
	, m_orphan_buf(NULL)
	, m_start_index(-1)
{
}

CDeviceDiffAdd::~CDeviceDiffAdd(void)
{
	if (m_smi_dev) m_smi_dev->Release();
	delete [] m_orphan_buf;
}

bool CDeviceDiffAdd::Init(void)
{
	LOG_STACK_TRACE();
	if (m_smi_dev) m_smi_dev->Release(), m_smi_dev=NULL;
	CSvtApplication::GetApp()->GetDevice(m_smi_dev);
	m_smi_dev->GetCardInfo(m_card_info);

	UINT val;

	if (0xFFFFFFFF == m_block)		m_smi_dev->GetProperty(_T("INFO_BLOCK"), m_info_block);
	else							m_info_block = m_block;
	m_info_block &= 0xFFFF;

	m_smi_dev->GetProperty(_T("INFO_PAGE"), val);
	m_index_page = LOBYTE( LOWORD(val) );
	m_orphan_page = HIBYTE( LOWORD(val) );

	if (NULL == m_orphan_buf) m_orphan_buf = new BYTE[(m_card_info.m_f_spck+1) * SECTOR_SIZE];

	return true;
}

#define STR_BUF_SIZE	255
bool CDeviceDiffAdd::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	// get info block id and page id
	JCSIZE chunk_size = m_card_info.m_f_spck;
	JCSIZE buf_size = chunk_size  * SECTOR_SIZE;

	stdext::auto_array<BYTE> buf(buf_size + SECTOR_SIZE);
	CSpareData spare;

	// read index of difference address
	JCSIZE block_id = 0;
	UINT chunk = 0;
	TCHAR _str[STR_BUF_SIZE+1];
	WORD ph_add[32];

	// output header
	stdext::jc_sprintf(_str, _T("FBlock,type,CE0/PL0/CHA,CE0/PL0/CHB,CE0/PL1/CHA,CE0/PL1/CHB,CE1/PL0/CHA,CE1/PL0/CHB,CE1/PL1/CHA,CE1/PL1/CHB,"));
	OutPutStringRow(outport, _str);

	//jcparam::IValue * val = jcparam::CTypedValue<CJCStringT>::Create(_str);
	//outport->PushResult(val);
	//val->Release();

	while (block_id < m_card_info.m_f_block_num)
	{
		CFlashAddress add(m_info_block, m_index_page, chunk);
		m_smi_dev->ReadFlashChunk(add, spare, buf, chunk_size + 1);

		for (JCSIZE ii = 0; ii < buf_size; ii+=2, ++block_id)
		{
			WORD index = MAKEWORD(buf[ii+1], buf[ii]);
			if (0 == index) continue;

			LPTSTR str = _str;
			JCSIZE remain = STR_BUF_SIZE;

			WORD type = 0;
			GetDiffAddress(index, type, ph_add);
			int ir = stdext::jc_sprintf(str, remain, _T("%04X,%d,"), block_id, type);
			str += ir, remain -=ir;
			// CE0/PL0/CHA, CE0/PL0/CHB, CE0/PL1/CHA, CE0/PL1/CHB, CE1/PL0/CHA, CE1/PL0/CHB, CE1/PL1/CHA, CE1/PL1/CHB		
			//ir = stdext::jc_sprintf(str, remain, _T("%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X,"), 
			//	ph_add[0], ph_add[1], ph_add[2], ph_add[3], ph_add[4], ph_add[5], ph_add[6], ph_add[7]);
			ir = stdext::jc_sprintf(str, remain, _T("%04X(%d),%04X(%d),"),
				ph_add[0] >> 1, ph_add[0] & 1, ph_add[1]>>1, ph_add[1] & 1);
			str += ir, remain -=ir;

			ir = stdext::jc_sprintf(str, remain, _T("%04X(%d),%04X(%d),"),
				ph_add[2] >> 1, ph_add[2] & 1, ph_add[3]>>1, ph_add[3] & 1); 
			str += ir, remain -=ir;
			ir = stdext::jc_sprintf(str, remain, _T("%04X(%d),%04X(%d),"),
				ph_add[4] >> 1, ph_add[4] & 1, ph_add[5]>>1, ph_add[5] & 1);
			str += ir, remain -=ir;
			ir = stdext::jc_sprintf(str, remain, _T("%04X(%d),%04X(%d),"),
				ph_add[6] >> 1, ph_add[6] & 1, ph_add[7]>>1, ph_add[7] & 1);
			str += ir, remain -=ir;
			OutPutStringRow(outport, _str);

			//jcparam::IValue * val = jcparam::CTypedValue<CJCStringT>::Create(_str);
			//outport->PushResult(val);
			//val->Release();
		}

		chunk ++;
		if (chunk >= m_card_info.m_f_ckpp) break;
	}

	return false;
}

void CDeviceDiffAdd::GetDiffAddress(JCSIZE index, WORD & type, WORD * add)
{
	JCASSERT(m_orphan_buf);

	JCSIZE entry_in_chunk = m_card_info.m_f_spck * 8;	// 8 entries per chunk

	if ( 0xFFFFFFFF == m_start_index || index < m_start_index || index >= m_start_index + entry_in_chunk)
	{	// not match, need to reload
		JCSIZE chunk = index / entry_in_chunk;
		CSpareData spare;
		CFlashAddress add(m_info_block, m_orphan_page, chunk);
		m_smi_dev->ReadFlashChunk(add, spare, m_orphan_buf, m_card_info.m_f_spck + 1);
		m_start_index = chunk * entry_in_chunk;
	}
	type = m_orphan_buf[(index-m_start_index)];

	BYTE * entry = m_orphan_buf + (index - m_start_index) * 64;
	for (int ii = 0, jj = 0; ii < 64; ii+=2, ++jj)
	{
		add[jj] = MAKEWORD(entry[ii+1], entry[ii]);
		//if (add[jj] > m_card_info.m_f_block_num) add[jj] -= m_card_info.m_f_block_num;
	}
}

///////////////////////////////////////////////////////////////////////////////
// -- device info (card mode)
LPCTSTR CDeviceInfo::_BASE_TYPE::m_feature_name = _T("info");
CParamDefTab CDeviceInfo::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("#dummy"),	0, offsetof(CDeviceInfo, m_dummy), _T("") ) )
	);
const TCHAR CDeviceInfo::m_desc[] = _T("show device info");

CDeviceInfo::CDeviceInfo(void)
: m_smi_dev(NULL)
{
}

CDeviceInfo::~CDeviceInfo(void)
{
	if (m_smi_dev)	m_smi_dev->Release();
}

bool CDeviceInfo::Init(void) 
{
	LOG_STACK_TRACE();
	if (m_smi_dev) m_smi_dev->Release(), m_smi_dev=NULL;
	CSvtApplication::GetApp()->GetDevice(m_smi_dev);
	m_smi_dev->GetCardInfo(m_card_info);

	return true;
}

CAttributeRow * CDeviceInfo::MakeAttributeRow(UINT id, const CJCStringT & name, UINT val, const CJCStringT & desc)
{
	CAttributeRow * row = NULL;

	row = new CAttributeRow;
	row->m_id = id;
	row->m_name = name;
	row->m_desc = desc;
	row->m_val = val;
	return row;
}

bool CDeviceInfo::PushAttribute(jcscript::IOutPort * outport, UINT id, const CJCStringT & name, UINT val, const CJCStringT & desc)
{
	JCASSERT(outport);
	CAttributeRow * row = MakeAttributeRow(id, name, val, desc);
	outport->PushResult(row);
	row->Release(), row = NULL;
	return true;
}

bool CDeviceInfo::InternalInvoke(jcparam::IValue * , jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

	UINT id = 0;
	PushAttribute(outport, id++, _T("controller"), 0, m_card_info.m_controller_name);

	UINT val = 0;

	m_smi_dev->GetProperty(_T("ISP_MODE"), val);

	WORD info_valid = HIWORD(val);
	PushAttribute(outport, id++, _T("info"), info_valid, info_valid?_T("valid"):_T("invalid"));

	CJCStringT	isp_mode;
	switch ( LOWORD(val) )
	{
	case ISPM_ROM_CODE: isp_mode = _T("ROM CODE");	break;
	case ISPM_ISP:		isp_mode = _T("ISP");		break;
	case ISPM_MPISP:	isp_mode = _T("MPISP");		break;
	default:			isp_mode = _T("UNKNOWN");	break;
	}
	PushAttribute(outport, id++, _T("isp"), LOWORD(val), isp_mode);
	
	// --
	CJCStringT	nand_type;
	switch (m_card_info.m_type)
	{
	case CCardInfo::NT_SLC:			nand_type = _T("SLC"); break;
	case CCardInfo::NT_MLC:			nand_type = _T("MLC"); break;
	case CCardInfo::NT_SLC_MODE:	nand_type = _T("SLC_MODE"); break;
	case CCardInfo::NT_TLC:			nand_type = _T("TLC"); break;
	}
	PushAttribute(outport, id++, _T("nand_type"), (UINT)(m_card_info.m_type), nand_type);
	
	// --
	//static const JCSIZE STR_BUF_SIZE= 127;
	stdext::auto_array<TCHAR> str(STR_BUF_SIZE +1);

	stdext::jc_sprintf(str, STR_BUF_SIZE, _T("%dKB"), m_card_info.m_f_spck / 2);
	PushAttribute(outport, id++, _T("f-chunk_size"), m_card_info.m_f_spck, (LPCTSTR)str);

	// --
	JCSIZE page_size = m_card_info.m_f_spck * m_card_info.m_f_ckpp;
	stdext::jc_sprintf(str, STR_BUF_SIZE, _T("%dKB"), page_size / 2);
	PushAttribute(outport, id++, _T("f-page_size"), page_size, (LPCTSTR)str);

	// --
	JCSIZE block_size = page_size * m_card_info.m_f_ppb;
	if (block_size >= 2048)		stdext::jc_sprintf(str, STR_BUF_SIZE, _T("%dMB"), block_size / 2048);
	else						stdext::jc_sprintf(str, STR_BUF_SIZE, _T("%dKB"), block_size / 2);
	PushAttribute(outport, id++, _T("f-block_size"), block_size, (LPCTSTR)str);

	// -- 
	stdext::jc_sprintf(str, STR_BUF_SIZE, _T("%d"), m_card_info.m_f_block_num);
	PushAttribute(outport, id++, _T("f-block_number"), m_card_info.m_f_block_num, (LPCTSTR)str);

	// --
	PushAttribute(outport, id++, _T("interleave"), m_card_info.m_interleave, _T("") );

	// --
	PushAttribute(outport, id++, _T("plane"), m_card_info.m_plane, _T("") );

	return false;
}


///////////////////////////////////////////////////////////////////////////////
// -- restore info

LPCTSTR CDeviceRestoreInfo::_BASE_TYPE::m_feature_name = _T("restoreinfo");
CParamDefTab CDeviceRestoreInfo::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("#dummy"),	0, offsetof(CDeviceRestoreInfo, m_dummy), _T("") ) )
	);
const TCHAR CDeviceRestoreInfo::m_desc[] = _T("show device info");

CDeviceRestoreInfo::CDeviceRestoreInfo(void)
: m_smi_dev(NULL)
{
}

CDeviceRestoreInfo::~CDeviceRestoreInfo(void)
{
	if (m_smi_dev)	m_smi_dev->Release();
}

bool CDeviceRestoreInfo::Init(void) 
{
	LOG_STACK_TRACE();
	if (m_smi_dev) m_smi_dev->Release(), m_smi_dev=NULL;
	CSvtApplication::GetApp()->GetDevice(m_smi_dev);
	m_smi_dev->GetCardInfo(m_card_info);

	return true;
}

bool CDeviceRestoreInfo::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

	return false;
}


///////////////////////////////////////////////////////////////////////////////
// -- smart

LPCTSTR CDeviceReadSmart::_BASE_TYPE::m_feature_name = _T("smart");
CParamDefTab CDeviceReadSmart::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<bool>(_T("vendor"),	_T('v'), offsetof(CDeviceReadSmart, m_vendor), _T("using vendor command") ) )
	);
const TCHAR CDeviceReadSmart::m_desc[] = _T("read smart data");

bool CDeviceReadSmart::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	stdext::auto_interface<ISmiDevice>	dev;
	CSvtApplication::GetApp()->GetDevice(dev);
	JCASSERT(dev);

	stdext::auto_interface<CSectorBuf>	data(NULL);
	CreateSectorBuf(0, data);

	stdext::auto_cif<IAtaDevice>	ata;
	dev->QueryInterface(IF_NAME_ATA_DEVICE, ata);
	if ( (!ata.valid()) || (m_vendor) )
	{	// read smart via vendor command
		bool br = dev->VendorReadSmart(data->Lock() );
		if (!br) THROW_ERROR(ERR_USER, _T("device do not support SMART vendor command."));
	}
	// read smart via ata interface	
	else ata->ReadSmartData(data->Lock(), SECTOR_SIZE);
	data->Unlock();
	outport->PushResult(data);
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// -- identify device

LPCTSTR CDeviceIdentify::_BASE_TYPE::m_feature_name = _T("identify");
CParamDefTab CDeviceIdentify::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<bool>(_T("vendor"),	_T('v'), offsetof(CDeviceIdentify, m_vendor), _T("using vendor command") ) )
	);
const TCHAR CDeviceIdentify::m_desc[] = _T("identify device");

bool CDeviceIdentify::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	stdext::auto_interface<ISmiDevice>	dev;
	CSvtApplication::GetApp()->GetDevice(dev);
	JCASSERT(dev);

	stdext::auto_interface<CSectorBuf>	data(NULL);
	CreateSectorBuf(0, data);

	stdext::auto_cif<IAtaDevice>	ata;
	dev->QueryInterface(IF_NAME_ATA_DEVICE, ata);
	if ( (!ata.valid()) || (m_vendor) )
	{	// read smart via vendor command
		bool br = dev->VendorIdentifyDevice(data->Lock() );
		if (!br) THROW_ERROR(ERR_USER, _T("device do not support IDENTIFY DEVICE vendor command."));
	}	
	// read smart via ata interface
	else	ata->IdentifyDevice(data->Lock(), SECTOR_SIZE);
	data->Unlock();
	outport->PushResult(data);
	return false;
}



///////////////////////////////////////////////////////////////////////////////
// -- smart decode

LPCTSTR CDeviceSmartDecode::_BASE_TYPE::m_feature_name = _T("smartdecode");
CParamDefTab CDeviceSmartDecode::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("revision"),	_T('r'), offsetof(CDeviceSmartDecode, m_rev), _T("revision of smart defination") ) )
	(new CTypedParamDef<bool>(_T("byid"),	_T('i'), offsetof(CDeviceSmartDecode, m_by_id), _T("decode by position") ) )
	);
const TCHAR CDeviceSmartDecode::m_desc[] = _T("show device info");

CDeviceSmartDecode::CDeviceSmartDecode(void)
	: m_smi_dev(NULL)
	, m_smart_def_tab(NULL)
	, m_col_list(NULL)
	, m_by_id(false)
{
}

CDeviceSmartDecode::~CDeviceSmartDecode(void)
{
	if (m_smi_dev)	m_smi_dev->Release();
	if (m_col_list)	m_col_list->Release();
}

bool CDeviceSmartDecode::Init(void) 
{
	LOG_STACK_TRACE();
	if (!m_smi_dev)
	{
		CSvtApplication::GetApp()->GetDevice(m_smi_dev);
		// create column list according to smart defination
		m_smart_def_tab = m_smi_dev->GetSmartAttrDefTab(m_rev.c_str() );		JCASSERT(m_smart_def_tab);
		m_col_list = new jcparam::CColInfoList;					JCASSERT(m_col_list);

		JCSIZE smart_def_size = m_smart_def_tab->GetSize();
		for (JCSIZE ii=0, jj=0; ii < smart_def_size; ++ii)
		{
			const CSmartAttributeDefine * def = m_smart_def_tab->GetItem(ii);
			JCASSERT(def);
			if ( !def->m_using ) continue;
			TCHAR title[128];
			stdext::jc_sprintf(title, 128, _T("0x%s (%s)"), def->m_str_id, def->m_name);

			m_col_list->AddInfo( new jcparam::COLUMN_INFO_BASE(ii, jcparam::VT_UINT, 0, title));
		}
	}
	return true;
}

bool CDeviceSmartDecode::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);
	JCASSERT(m_col_list);

	CSectorBuf * buf = dynamic_cast<CSectorBuf*>(row);
	if (!buf) return false;

	BYTE * smart_buf = buf->Lock();

#define MAX_LINE_SIZE	512
	stdext::auto_array<TCHAR>	str_buf(MAX_LINE_SIZE);
	LPTSTR str = str_buf;
	JCSIZE remain = MAX_LINE_SIZE;

	JCSIZE smart_def_size = m_smart_def_tab->GetSize();
	JCSIZE offset = 2;
	for (JCSIZE ii=0, jj=0; ii < smart_def_size; ++ii, offset += 12)
	{
		BYTE attr_id = smart_buf[offset];
		const CSmartAttributeDefine * def = NULL;
		if (m_by_id)
		{
			TCHAR str_id[3];
			if ( 0 == attr_id ) continue;
			stdext::itohex(str_id, 2, attr_id);
			def = m_smart_def_tab->GetItem(str_id);
			if ( !def ) continue;
		}
		else
		{
			def = m_smart_def_tab->GetItem(ii);
			JCASSERT(def);
			if ( !def->m_using )	continue;
		}

		DWORD lo = *(DWORD*) (smart_buf + offset + 5);
		int ir = stdext::jc_sprintf(str, remain, _T("%d,"), lo);
		str += ir, remain -= ir;
	}

	stdext::auto_interface<jcparam::CGeneralRow>	attr_row;
	jcparam::CGeneralRow::CreateGeneralRow(m_col_list, attr_row);		JCASSERT(attr_row);
	attr_row->SetValueText(str_buf);
	outport->PushResult(attr_row);

	return false;
}


///////////////////////////////////////////////////////////////////////////////
// -- dump page

LPCTSTR CDeviceDumpPage::_BASE_TYPE::m_feature_name = _T("dumpdata");
CParamDefTab CDeviceDumpPage::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("block"),	'b', offsetof(CDeviceDumpPage, m_block), _T("block id for dump") ) )
	(new CTypedParamDef<UINT>(_T("page"),	'p', offsetof(CDeviceDumpPage, m_page), _T("page id for dump, default dump all pages in block") ) )
	(new CTypedParamDef<bool>(_T("all"),	'h', offsetof(CDeviceDumpPage, m_read_all), _T("dump date including empty page") ) )
	(new CTypedParamDef<bool>(_T("data"),	'd', offsetof(CDeviceDumpPage, m_read_data), _T("read data in the page") ) )
	);
const TCHAR CDeviceDumpPage::m_desc[] = _T("dump data");

CDeviceDumpPage::CDeviceDumpPage(void)
	: m_smi_dev(NULL)
	, m_block(0), m_page(UINT_MAX)
	, m_read_all(false), m_read_data(false)
{
}

CDeviceDumpPage::~CDeviceDumpPage(void)
{
	if (m_smi_dev) m_smi_dev->Release();
}

bool CDeviceDumpPage::Init(void)
{
	LOG_STACK_TRACE();
	if ( !m_smi_dev )
	{
		CSvtApplication::GetApp()->GetDevice(m_smi_dev);
		m_smi_dev->GetCardInfo(m_card_info);
	}
	m_chunk_size = m_card_info.m_f_spck + 1;

	if (m_page >= m_card_info.m_f_ppb)	m_cur_page = 0, m_end_page = m_card_info.m_f_ppb;
	else								m_cur_page = m_page, m_end_page = m_page + 1;
	return true;
}

bool CDeviceDumpPage::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);
	
	JCSIZE buf_size = m_chunk_size * SECTOR_SIZE;

	CFlashAddress add(m_block, m_cur_page, 0);
	stdext::auto_array<BYTE> buf(buf_size);
	CSpareData spare;
	m_smi_dev->ReadFlashChunk(add, spare, buf, m_chunk_size);

	if ( 0xFF != spare.m_id || m_read_all )
	{	// save meta data
		stdext::auto_interface<BLOCK_ROW> block_row(new BLOCK_ROW);
		block_row->SetAddress(add);
		block_row->SetSpare(spare);
		block_row->CreateErrorBit(m_card_info.m_channel_num, m_card_info.m_f_ckpp);
		block_row->SetErrorBit(spare, 0);
		
		if (m_read_data)
		{	// save data
			JCSIZE chunk = 0;
			JCSIZE sec_add = 0;
			while (1)
			{// save one chunk data
				for (JCSIZE ss = 0; ss < m_card_info.m_f_spck; ss++)
				{
					stdext::auto_interface<CSectorBuf>	data;
					CreateSectorBuf(sec_add, data);
					memcpy_s(data->Lock(), SECTOR_SIZE, buf + (ss * SECTOR_SIZE), SECTOR_SIZE);
					data->Unlock();
					block_row->PushData(data);
					sec_add ++;
				}
				// read next chunk
				chunk ++;
				if (chunk >= m_card_info.m_f_ckpp) break;
				add.m_chunk = chunk;
				m_smi_dev->ReadFlashChunk(add, spare, buf, m_chunk_size);
				block_row->SetErrorBit(spare, chunk);
			}
		}
		outport->PushResult(block_row);
	}

	m_cur_page ++;

	if (m_cur_page >= m_end_page)	return false;		// next block
	else							return true;		// continue reading this block
}


///////////////////////////////////////////////////////////////////////////////
// -- dump filter

LPCTSTR CDeviceDumpFilter::_BASE_TYPE::m_feature_name = _T("dumpfilter");
CParamDefTab CDeviceDumpFilter::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("dumpdata"),	'd', offsetof(CDeviceDumpFilter, m_dump_data), _T("specify block types which need to dump data") ) )
	(new CTypedParamDef<CJCStringT>(_T("dumppage"),	'p', offsetof(CDeviceDumpFilter, m_dump_page_id), _T("specify block types which need to dump page metadata") ) )
	(new CTypedParamDef<CJCStringT>(_T("dumpblock"),'l', offsetof(CDeviceDumpFilter, m_dump_block_id), _T("specify block types which need to dump block metadata") ) )
	(new CTypedParamDef<UINT>(_T("block"),	'b', offsetof(CDeviceDumpFilter, m_block), _T("block id for dump") ) )
	);
const TCHAR CDeviceDumpFilter::m_desc[] = _T("make an address list for dump");

//CDeviceDumpFilter::CDeviceDumpFilter(void)
//	: m_smi_dev(NULL)
//	, m_block(0), m_page(UINT_MAX)
//{
//}
//
//CDeviceDumpFilter::~CDeviceDumpFilter(void)
//{
//	if (m_smi_dev) m_smi_dev->Release();
//}

bool CDeviceDumpFilter::Init(void)
{
	LOG_STACK_TRACE();
	// 参数解析，设置各类型block的dump方法
	memset(m_dump_type, 0, sizeof (m_dump_type) );
	// 缺省：mother block(0x2x, 3x)保存block meta data;
	for (int ii = 0x20; ii< 0x40; ++ii)	m_dump_type[ii] = DUMP_BLOCK;
	// cache block（0x40~0xE0)：保存page meta data
	for (int ii = 0x40; ii< 0xE0; ++ii)	m_dump_type[ii] = DUMP_PAGE;
	// system block (0xE0 ~):保存所有data
	for (int ii = 0xE0; ii< 0x100; ++ii)	m_dump_type[ii] = DUMP_DATA;

	//


	return true;
}

bool CDeviceDumpFilter::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	stdext::auto_interface<ISmiDevice>	dev;
	CSvtApplication::GetApp()->GetDevice(dev);
	CCardInfo	card_info;
	dev->GetCardInfo(card_info);
	JCSIZE	chunk_size = card_info.m_f_spck + 1;

	JCSIZE buf_size = chunk_size * SECTOR_SIZE;
	stdext::auto_array<BYTE> buf(buf_size);

	CFlashAddress add(m_block, 0, 0);

	CSpareData spare;
	dev->ReadFlashChunk(add, spare, buf, chunk_size);

	if ( 0xFF == spare.m_id )
	{	// try page 1
		add.m_page = 1;
		dev->ReadFlashChunk(add, spare, buf, chunk_size);
	}

	stdext::auto_interface<BLOCK_ROW> block_row(new BLOCK_ROW);

	//BYTE id = spare.m_id & 0xF0;
	DUMP dump = m_dump_type[spare.m_id & 0xFF];

	switch (dump) 
	{
	case DUMP_NON:	break;

	case DUMP_BLOCK:
		block_row->SetAddress( CFlashAddress(m_block, 0));
		spare.m_serial_no = 0;
		block_row->SetSpare(spare);
		outport->PushResult(block_row);
		break;

	case DUMP_PAGE:
		block_row->SetAddress( CFlashAddress(m_block, 0xFFFF));
		spare.m_serial_no = 0;
		block_row->SetSpare(spare);
		outport->PushResult(block_row);
		break;

	case DUMP_DATA:
		block_row->SetAddress( CFlashAddress(m_block, 0xFFFF) );
		spare.m_serial_no = 1;
		block_row->SetSpare(spare);
		outport->PushResult(block_row);
		break;
	}

	//if ( 0x20 == id )
	//{	// mother block, meta data of page 0 only
	//}
	//else if ( id >= 0x40 && id < 0xE0 )
	//{	// cache block, dump meta data
	//}
	//else if ( 0xE0 == id )
	//{	// system block, dump all data
	//}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// -- erase count

LPCTSTR CDeviceEraseCount::_BASE_TYPE::m_feature_name = _T("erasecount");
CParamDefTab CDeviceEraseCount::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<bool>(_T("vendor"),	_T('v'), offsetof(CDeviceEraseCount, m_vendor), _T("using vendor command") ) )
	);
const TCHAR CDeviceEraseCount::m_desc[] = _T("read block erase count");

#define ERASE_COUNT_SIZE	(0x21)

bool CDeviceEraseCount::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	stdext::auto_interface<ISmiDevice>	dev;
	CSvtApplication::GetApp()->GetDevice(dev);
	JCASSERT(dev);

	stdext::auto_array<BYTE> pe_buf(SECTOR_SIZE * ERASE_COUNT_SIZE);
	memset(pe_buf, 0, SECTOR_SIZE * ERASE_COUNT_SIZE);

	stdext::auto_cif<IAtaDevice>	ata;
	dev->QueryInterface(IF_NAME_ATA_DEVICE, ata);
	if ( (!ata.valid()) || (m_vendor) )
	{	// read smart via vendor command
		CSmiCommand cmd;
		cmd.id(0xF042);
		cmd.size() = ERASE_COUNT_SIZE;
		bool br = dev->VendorCommand(cmd, read, pe_buf, ERASE_COUNT_SIZE);
		if (!br) THROW_ERROR(ERR_USER, _T("failure on vendor cmd."));
	}	
	else
	{	// read smart via ata interface
		ATA_REGISTER reg;
		reg.command = 0xB0;		// SMART
		reg.feature = 0xE2;		
		reg.lba_mid = 0x4F;
		reg.lba_hi = 0xC2;
		reg.sec_count = ERASE_COUNT_SIZE;
		reg.device = 0xE0;
		
		bool br = ata->AtaCommand(reg, read, false, pe_buf, ERASE_COUNT_SIZE);
		if (!br) THROW_WIN32_ERROR(_T("failure on erase count vendor cmd. "));
		if (reg.status & 1)	THROW_ERROR(ERR_USER, _T("device returned an error. status=0x%02X, code=0x%02X"), 
			reg.status, reg.error)
	}

	// analysis data
	JCSIZE mu = MAKEWORD(pe_buf[0x4000], pe_buf[0x4001]);
	JCSIZE block_num = MAKEWORD(pe_buf[0x4007], pe_buf[0x4006]);
	if (0 == mu || 0 == block_num)	THROW_ERROR(ERR_USER, _T("wear-leveling data wrong") );
	JCSIZE base_pe = MAKELONG( MAKEWORD(pe_buf[0x4005], pe_buf[0x4004]), MAKEWORD(pe_buf[0x4003], pe_buf[0x4002]) );

	for (JCSIZE bb = 0; bb < block_num; ++ bb)
	{
		CBlockEraseCountRow * row = new CBlockEraseCountRow;
		row->m_id = bb;
		if (pe_buf[bb] == 0)	row->m_erase_count = -1;
		else					row->m_erase_count = base_pe + pe_buf[bb] -1;
		outport->PushResult(static_cast<jcparam::IValue*>(row));
		row ->Release();
	}
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// -- clean cache
LPCTSTR CDeviceCleanCache::_BASE_TYPE::m_feature_name = _T("cleancache");
CParamDefTab CDeviceCleanCache::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<bool>(_T("clean"),	_T('c'), offsetof(CDeviceCleanCache, m_clean), _T("do force clean. otherwise check cache number only") ) )
	);
const TCHAR CDeviceCleanCache::m_desc[] = _T("force clean cache");

bool CDeviceCleanCache::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	stdext::auto_interface<ISmiDevice>	dev;
	CSvtApplication::GetApp()->GetDevice(dev);
	JCASSERT( dev.valid() );

	UINT cache_num;
	dev->GetProperty(CSmiDeviceBase::PROP_CACHE_NUM, cache_num);
	LOG_DEBUG(_T("cache number before clean = %d"), cache_num);

	if (m_clean)
	{
		CSmiBlockCmd cmd;
		cmd.id(0xF0F4);
		stdext::auto_array<BYTE> buf(SECTOR_SIZE);

		dev->VendorCommand(cmd, read, buf, 1, 900);
	}

	dev->GetProperty(CSmiDeviceBase::PROP_CACHE_NUM, cache_num);
	LOG_DEBUG(_T("cache number after clean = %d"), cache_num);

	return false;
}


///////////////////////////////////////////////////////////////////////////////
// -- cup reset
LPCTSTR CDeviceReset::_BASE_TYPE::m_feature_name = _T("reset");
CParamDefTab CDeviceReset::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("#dummy"),	0, offsetof(CDeviceReset, m_dummy), _T("") ) )
	);
const TCHAR CDeviceReset::m_desc[] = _T("reset cpu");

bool CDeviceReset::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();

	stdext::auto_interface<ISmiDevice>	dev;
	CSvtApplication::GetApp()->GetDevice(dev);
	JCASSERT( dev.valid() );

	dev->ResetCpu();
	//CSmiBlockCmd cmd;
	//cmd.id(0xF02C);
	//stdext::auto_array<BYTE> buf(SECTOR_SIZE);
	//LOG_DEBUG(_T("CPU reseting..."));
	//dev->VendorCommand(cmd, read, buf, 1);
	//LOG_DEBUG(_T("CPU reset done."));

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// -- sleep
LPCTSTR CSystemSleep::_BASE_TYPE::m_feature_name = _T("sleep");
CParamDefTab CSystemSleep::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("duration"),	_T('t'), offsetof(CSystemSleep, m_sleep), _T("sleep duration in ms") ) )
	);
const TCHAR CSystemSleep::m_desc[] = _T("wait for several ms");

bool CSystemSleep::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();
	::Sleep(m_sleep);
	return false;
}