#include "stdafx.h"
#include "plugin_manager.h"
#include "plugin_debug.h"

#include "application.h"
#include "feature_2246_readcount.h"

LOCAL_LOGGER_ENABLE(_T("debug_pi"), LOGGER_LEVEL_NOTICE);

///////////////////////////////////////////////////////////////////////////////
// -- cache mode simulator
CJCStringT CPluginBase2<CPluginDebug>::m_name(_T("debug"));

FEATURE_LIST CPluginBase2<CPluginDebug>::m_feature_list(
	FEATURE_LIST::RULE()
	(new CTypedFtInfo< CPluginDebug::CachePage::_BASE >( _T("") ) ) 
	(new CTypedFtInfo< CPluginDebug::LbaToHadd::_BASE >( _T("convert lba to h address") ) )
	(new CTypedFtInfo< CPluginDebug::DelayPattern::_BASE >( _T("") ) ) 
	//(new CTypedFtInfo< CPluginDebug::Connect::_BASE>(_T("connect to simulator device") ) )
	);

CPluginDebug::CPluginDebug(void)
{
};

CPluginDebug::~CPluginDebug(void)
{
}

bool RegisterPluginDebug(jcscript::IPluginContainer * plugin_cont)
{
	JCASSERT(plugin_cont);
	stdext::auto_interface<CCategoryComm> cat(new CCategoryComm(_T("debug")) );

	cat->RegisterFeatureT<CFeature2246ReadCount>();
	cat->RegisterFeatureT<CDebugRepeat>();
	cat->RegisterFeatureT<CDebugInject>();
	cat->RegisterFeatureT<CDebugPowerCycleTest>();

	plugin_cont->RegistPlugin(cat);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// -- cache pages

LPCTSTR CPluginDebug::CachePage::_BASE::m_feature_name = _T("cachepage");
CParamDefTab CPluginDebug::CachePage::_BASE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<CJCStringT>(_T("#filename"), 0, offsetof(CPluginDebug::CachePage, m_file_name) ) )
	);


CPluginDebug::CachePage::CachePage(void)
	: m_smi_dev(NULL)
	, m_buf(NULL)
{
}

CPluginDebug::CachePage::~CachePage(void)
{
	delete [] m_buf;
	if (m_smi_dev)	m_smi_dev->Release();
	BLOCK_INFO_LIST::iterator it, endit = m_cache_list.end();
	for (it = m_cache_list.begin(); it != endit; ++it)
	{
		JCASSERT(*it);
		delete (*it);
	}
}

void CPluginDebug::CachePage::GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const
{

}

void CPluginDebug::CachePage::ReadSpare(JCSIZE block, JCSIZE page, CFBlockInfo* info/*, BYTE * buf*/)
{
	info->m_f_add.m_block = block;
	info->m_f_add.m_page = page;
	info->m_f_add.m_chunk = 0;

	m_smi_dev->ReadFlashChunk(info->m_f_add, info->m_spare, m_buf, m_chunk_size);
	info->m_id = info->m_spare.m_id;
}

struct sort_cache
{
	bool operator() (const CFBlockInfo * l, const CFBlockInfo * r) const
	{
		return l->m_spare.m_serial_no < r->m_spare.m_serial_no;
	}
};

bool CPluginDebug::CachePage::Init(void)
{
	LOG_STACK_TRACE();
	if (!m_smi_dev)	CSvtApplication::GetApp()->GetDevice(m_smi_dev);

	// search and store cache blocks
	CCardInfo card_info;
	m_smi_dev->GetCardInfo(card_info);

	m_fblock_num = card_info.m_f_block_num;
	m_page_per_block = card_info.m_f_ppb;
	m_chunk_size = card_info.m_f_spck + 1;

	m_buf = new BYTE[m_chunk_size * SECTOR_SIZE];

	CFBlockInfo * cache_block = NULL;
	//CFBlockInfo::m_channel_num = card_info.m_channel_num;
	
	for (JCSIZE bb = 0; bb < m_fblock_num; ++bb)
	{
		if (NULL == cache_block) cache_block = new CFBlockInfo;
		// try page 0
		ReadSpare(bb, 0, cache_block);
		LOG_DEBUG(_T("f-block %04X, f-page 0, id %02X"), bb, cache_block->m_id);
		if (0xFF == cache_block->m_id)
		{	// try page 1
			ReadSpare(bb, 1, cache_block);
			LOG_DEBUG(_T("f-block %04X, f-page 1, id %02X"), bb, cache_block->m_id);
		}

		if (0x40 == (cache_block->m_id & 0xF0) )
		{
			m_cache_list.push_back(cache_block);
			cache_block = NULL;
		}
	}
	// 排序
	m_cache_list.sort( struct sort_cache() );

	// for debug
#ifdef _DEBUG
	BLOCK_INFO_LIST::iterator it, endit = m_cache_list.end();
	for (it = m_cache_list.begin(); it != endit; ++it)
	{
		JCASSERT(*it);
		LOG_DEBUG(_T("fb=%04X, sn=%02X"), (*it)->m_f_add.m_block, (*it)->m_spare.m_serial_no );
	}
#endif

	delete [] cache_block;

	// preapre for start
	m_cur_cache = m_cache_list.begin();
	m_cur_page = 0;

	return true;
}

bool CPluginDebug::CachePage::InvokeOnce(void)
{
	//if ( m_cur_cache == m_cache_list.end() )		return false;
	//if (m_output)	m_output->Release(), m_output = NULL;
	//
	//CFBlockInfo * cache = (*m_cur_cache);
	//JCASSERT(cache);

	//stdext::auto_interface<BLOCK_ROW> row(new BLOCK_ROW);
	//ReadSpare(cache->m_f_add.m_block, m_cur_page, static_cast<CFBlockInfo*>(row)/*, buf*/);
	//row.detach(m_output);

	//++ m_cur_page;
	//if (m_cur_page >= m_page_per_block)	 ++ m_cur_cache, m_cur_page = 0;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// -- convert lba to h-address
LPCTSTR CPluginDebug::LbaToHadd::_BASE::m_feature_name = _T("lba2h");

CParamDefTab CPluginDebug::LbaToHadd::_BASE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<FILESIZE>(_T("lba"), _T('a'), offsetof(CPluginDebug::LbaToHadd, m_lba), _T("input lba") ) )
	);

CPluginDebug::LbaToHadd::LbaToHadd()
	: m_hadd(NULL)
	, m_lba(0)
	, m_init(false)
{
}


CPluginDebug::LbaToHadd::~LbaToHadd(void)
{
	if (m_hadd)	m_hadd->Release();
}


bool CPluginDebug::LbaToHadd::GetResult(jcparam::IValue * & val)
{
	val = static_cast<jcparam::IValue*>(m_hadd);
	if (val) val->AddRef();
	return true;
}

bool CPluginDebug::LbaToHadd::Init(void)
{
	stdext::auto_interface<ISmiDevice>	dev;
	CSvtApplication::GetApp()->GetDevice(dev);

	CCardInfo card_info;
	dev->GetCardInfo(card_info);

	m_page_size = card_info.m_f_spck * card_info.m_f_ckpp;
	m_block_size = card_info.m_f_ppb * m_page_size;
	return true;
}

bool CPluginDebug::LbaToHadd::Invoke(void)
{
	//if (!m_init)	Init();

	//if (m_hadd)	m_hadd->Release(),	m_hadd= NULL;

	//if (!m_src_op) return false;
	//stdext::auto_interface<jcparam::IValue> val;
	//m_src_op->GetResult(val);
	//if ( !val ) return false;

	//jcparam::GetSubVal(val, _T("lba"), m_lba);
	//m_hadd = new BLOCK_ROW;

	//m_hadd->m_spare.m_hblock = (WORD)(m_lba / m_block_size);
	//m_hadd->m_spare.m_hpage = (WORD)((m_lba % m_block_size) / m_page_size);

	return true;
}



///////////////////////////////////////////////////////////////////////////////
// -- delay pattern

LPCTSTR CPluginDebug::DelayPattern::_BASE::m_feature_name = _T("delayptn");
CParamDefTab CPluginDebug::DelayPattern::_BASE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("latency"),	_T('l'), offsetof(CPluginDebug::DelayPattern, m_latency),	_T("cache block number") ) )
	(new CTypedParamDef<UINT>(_T("offset"),		_T('o'), offsetof(CPluginDebug::DelayPattern, m_offset),	_T("start h-block") ) )
	);

CPluginDebug::DelayPattern::DelayPattern(void)
{
	m_latency = 90;
	m_offset = 0;
}


CPluginDebug::DelayPattern::~DelayPattern(void)
{
}


void CPluginDebug::DelayPattern::GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const
{

}

bool CPluginDebug::DelayPattern::Init(void)
{
	m_block = 0, m_page = 1;
	m_first_page = true;
	return true;
}

bool CPluginDebug::DelayPattern::InvokeOnce(void)
{
	//if (m_output)	m_output->Release(), m_output = NULL;
	//if (m_page > m_latency)	return false;

	//CAtaTraceRow * trace = new CAtaTraceRow;
	//trace->m_cmd_code = CMD_WRITE_DMA;
	//trace->m_sectors = 1;

	//if (m_first_page)
	//{
	//	trace->m_lba = H2LBA(m_block + m_offset, 0) +1;
	//	m_first_page = false;
	//}
	//else
	//{
	//	trace->m_lba = H2LBA(m_block + m_offset, m_page) +1;
	//	m_block ++;
	//	if ( (m_block &0xFF) == 0 )		m_first_page = true;
	//}

	//m_output = static_cast<jcparam::IValue*>(trace);


	//if (m_block >= 512)
	//{
	//	m_block = 0;
	//	m_page +=2;
	//}
	return true;
}

JCSIZE CPluginDebug::DelayPattern::H2LBA(JCSIZE block, JCSIZE page)
{
	// 64 sectors per page, 512 pages per block
	JCSIZE lba = block * 512 * 64 + page * 64;
	return lba;
}

///////////////////////////////////////////////////////////////////////////////
// -- debug repeat

LPCTSTR CDebugRepeat::_BASE_TYPE::m_feature_name = _T("repeat");
CParamDefTab CDebugRepeat::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<int>(_T("repeat"),	'r', offsetof(CDebugRepeat, m_repeat) ) )
	);

const TCHAR CDebugRepeat::m_desc[] = _T("make repeat");

CDebugRepeat::CDebugRepeat(void)
	: m_repeat(1)
{
}

CDebugRepeat::~CDebugRepeat(void)
{
}

bool CDebugRepeat::Init(void)
{
	LOG_STACK_TRACE();
	m_repeat_count = m_repeat;
	return true;
}

bool CDebugRepeat::InternalInvoke(jcparam::IValue *, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();
	stdext::auto_interface<BLOCK_ROW> block_row(new BLOCK_ROW);
	outport->PushResult(block_row);

	LOG_DEBUG(_T("repeat %d"), m_repeat_count);

	if (m_repeat < 0) return true;
	m_repeat_count --;
	if (m_repeat_count <= 0)	return false;
	else						return true;
}

///////////////////////////////////////////////////////////////////////////////
// -- debug inject
LPCTSTR CDebugInject::_BASE_TYPE::m_feature_name = _T("inject");
CParamDefTab CDebugInject::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<JCSIZE>(_T("lba"),	'a', offsetof(CDebugInject, m_lba) ) )
	(new CTypedParamDef<JCSIZE>(_T("sectors"),	'n', offsetof(CDebugInject, m_sectors) ) )
	(new CTypedParamDef<UINT>(_T("cmd"),	'c', offsetof(CDebugInject, m_command) ) )
	// 当执行时间 > timeout时，记录;单位us
	(new CTypedParamDef<UINT>(_T("timeout"),	't', offsetof(CDebugInject, m_time_out), _T("time out for log")) )
	);

const TCHAR CDebugInject::m_desc[] = _T("Inject a command to device");


CDebugInject::CDebugInject() 
	: m_storage(NULL)
	, m_data_buf(NULL)
	, m_max_secs(0)
	, m_lba(0),	m_sectors(1), m_command(0)
	, m_time_out(UINT_MAX)
{
}

CDebugInject::~CDebugInject(void)
{
	if (m_storage) m_storage->Release();
	delete [] m_data_buf;
}

bool CDebugInject::Init(void)
{
	if (! m_storage)
	{
		stdext::auto_interface<ISmiDevice>	smi_dev;
		CSvtApplication::GetApp()->GetDevice(smi_dev);
		smi_dev->GetStorageDevice(m_storage);
	}
	return true;
}


bool CDebugInject::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	BYTE cmd = m_command;
	JCSIZE lba = m_lba;
	JCSIZE secs = m_sectors;

	CAtaTraceRow * trace_in = dynamic_cast<CAtaTraceRow*>(row);
	if (trace_in)
	{
		cmd = trace_in->m_cmd_code;
		lba = trace_in->m_lba;
		secs = trace_in->m_sectors;
	}

	if (secs > m_max_secs)
	{
		m_max_secs = secs;
		delete [] m_data_buf;
		m_data_buf = new BYTE[m_max_secs * SECTOR_SIZE];
	}
	
	stdext::auto_interface<CAtaTraceRow>	trace_out(new CAtaTraceRow);

	trace_out->m_cmd_code = cmd;
	trace_out->m_lba = lba;
	trace_out->m_sectors = secs;
	// us -> s
	trace_out->m_start_time = (stdext::GetTimeStamp() / CJCLogger::Instance()->GetTimeStampCycle() )	/ (1000.0 * 1000.0);
	
	switch (cmd)
	{
	case 0xCA:
		m_storage->SectorWrite(m_data_buf, lba, secs);
		break;

	case 0xC8:
	case 0x25:
		m_storage->SectorRead(m_data_buf, lba, secs);
		break;
	default :
		return false;
	}
	UINT invoke_time = m_storage->GetLastInvokeTime();
	if (invoke_time >= m_time_out)
	{
		LOG_WARNING(_T("invoke time out: cmd=%02X, lba=0x%08X, size=%d, busy=%dus"), cmd, lba, secs, invoke_time);
	}
	trace_out->m_busy_time = invoke_time / 1000.0;	// us -> ms

	outport->PushResult(trace_out);
	return false;
}



///////////////////////////////////////////////////////////////////////////////
// -- debug power cycle test

LPCTSTR CDebugPowerCycleTest::_BASE_TYPE::m_feature_name = _T("powercycle");
CParamDefTab CDebugPowerCycleTest::_BASE_TYPE::m_param_def_tab(	CParamDefTab::RULE()
	(new CTypedParamDef<UINT>(_T("reset"),	'r', offsetof(CDebugPowerCycleTest, m_reset_count), _T("one log per reset count") ) )
	(new CTypedParamDef<UINT>(_T("delay"),	'd', offsetof(CDebugPowerCycleTest, m_delay), _T("delay between power off/on") ) )
	(new CTypedParamDef<bool>(_T("power"),	'p', offsetof(CDebugPowerCycleTest, m_power), _T("using power instead of reset") ) )
	//(new CTypedParamDef<UINT>(_T("page"),	'p', offsetof(CDebugPowerCycleTest, m_page), _T("page id for dump, default dump all pages in block") ) )
	);
const TCHAR CDebugPowerCycleTest::m_desc[] = _T("make an address list for dump");

CDebugPowerCycleTest::CDebugPowerCycleTest(void)
	: m_smi_dev(NULL), m_pe(NULL)
	, m_col_list(NULL)
	, m_reset_count(30)
	, m_delay(100)
	, m_power(false)
{
}

CDebugPowerCycleTest::~CDebugPowerCycleTest(void)
{
	if (m_smi_dev) m_smi_dev->Release();
	if (m_col_list)	m_col_list->Release();
	delete [] m_pe;
}

bool CDebugPowerCycleTest::Init(void)
{
	LOG_STACK_TRACE();
	if ( !m_smi_dev )
	{
		CSvtApplication::GetApp()->GetDevice(m_smi_dev);
		CCardInfo card_info;
		m_smi_dev->GetCardInfo(card_info);
		m_chunk_size = card_info.m_f_spck + 1;

		m_block_num = card_info.m_f_block_num;
		m_pe = new int[m_block_num];
		m_smi_dev->GetBlockEraseCount(m_pe, m_block_num, m_base_pe);

		// 
		m_col_list = new jcparam::CColInfoList;		JCASSERT(m_col_list);
		m_col_list->AddInfo( new jcparam::COLUMN_INFO_BASE(0, jcparam::VT_UINT, 0, _T("cycle")));
		m_col_list->AddInfo( new jcparam::COLUMN_INFO_BASE(1, jcparam::VT_UINT, 0, _T("total_pe")));
		m_col_list->AddInfo( new jcparam::COLUMN_INFO_BASE(2, jcparam::VT_STRING, 0, _T("diff")));

		m_total_rst = 0;
	}
	return true;
}

#define CMD_BLOCK_SIZE	16

bool CDebugPowerCycleTest::InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	LOG_STACK_TRACE();
	JCASSERT(m_smi_dev);

	// reset	~ 30 times
	for (int ii = 0; ii < m_reset_count; ++ii)
	{
		if (m_power)
		{
			// power on off instead reset
			stdext::auto_array<BYTE>	_buf(SECTOR_SIZE);
			stdext::auto_interface<IStorageDevice> storage;
			m_smi_dev->GetStorageDevice(storage);	JCASSERT(storage.valid());
			BYTE cmd_block[CMD_BLOCK_SIZE];

			// power off
			LOG_DEBUG(_T("power off: %d"), (m_total_rst+ii));
			memset(cmd_block, 0, sizeof(BYTE) * CMD_BLOCK_SIZE);
			cmd_block[0] = 0x1B, cmd_block[1] = 0, cmd_block[4] = 0x02;
			storage->ScsiCommand(read, _buf, 0, cmd_block, CMD_BLOCK_SIZE, 60);

			Sleep(m_delay);

			// power on
			LOG_DEBUG(_T("power on: %d"), (m_total_rst+ii));
			memset(cmd_block, 0, sizeof(BYTE) * CMD_BLOCK_SIZE);
			cmd_block[0] = 0x1B, cmd_block[1] = 0, cmd_block[4] = 0;
			storage->ScsiCommand(read, _buf, 0, cmd_block, CMD_BLOCK_SIZE, 60);

			memset(cmd_block, 0, sizeof(BYTE) * CMD_BLOCK_SIZE);
			cmd_block[0] = 0x0, cmd_block[1] = 0, cmd_block[4] = 0;
			storage->ScsiCommand(read, _buf, 0, cmd_block, CMD_BLOCK_SIZE, 60);
		}
		else
		{
			m_smi_dev->ResetCpu();
		}
		Sleep(m_delay);
	}
	m_total_rst += m_reset_count;
	Sleep(m_delay);

	LOG_DEBUG(_T("get erase count"));

	// get w/l
	//stdext::auto_array<int> temp_pe(m_block_num);
	int * temp_pe = new int[m_block_num];
	m_smi_dev->GetBlockEraseCount(temp_pe, m_block_num, m_base_pe);

	stdext::auto_array<BYTE> buf(m_chunk_size * SECTOR_SIZE);
	// cal total erase count
	CJCStringT	pe_diff;
	TCHAR	str[32];
	JCSIZE total_pe = 0;
	for (JCSIZE bb=0; bb < m_block_num; ++bb)
	{
		if (temp_pe[bb] >=0) total_pe += temp_pe[bb];
		// cal erase count difference 
		if (temp_pe[bb] != m_pe[bb])
		{	// get block info of difference
			CFlashAddress add(bb, 0, 0);
			CSpareData spare;
			m_smi_dev->ReadFlashChunk(add, spare, buf, m_chunk_size);
			stdext::jc_sprintf(str, _T("%04X:%02X:%d;"), bb, spare.m_id, (temp_pe[bb] - m_pe[bb]));
			pe_diff += str;
		}
	}

	stdext::jc_sprintf(str, _T("%d,%d,"), m_total_rst, total_pe);
	CJCStringT	row(str);
	row += pe_diff;
	

	stdext::auto_interface<jcparam::CGeneralRow>	pe_row;
	jcparam::CGeneralRow::CreateGeneralRow(m_col_list, pe_row);		JCASSERT(pe_row);
	pe_row->SetValueText(row.c_str());
	outport->PushResult(pe_row);

	delete m_pe;
	m_pe = temp_pe;
	return false;
}