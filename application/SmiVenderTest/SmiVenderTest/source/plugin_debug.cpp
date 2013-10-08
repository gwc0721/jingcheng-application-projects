#include "stdafx.h"
#include "plugin_manager.h"
#include "plugin_debug.h"

#include "application.h"

LOCAL_LOGGER_ENABLE(_T("debug_pi"), LOGGER_LEVEL_WARNING);

///////////////////////////////////////////////////////////////////////////////
// -- cache mode simulator
CJCStringT CPluginBase2<CPluginDebug>::m_name(_T("debug"));

FEATURE_LIST CPluginBase2<CPluginDebug>::m_feature_list(
	FEATURE_LIST::RULE()
	(new CTypedFtInfo< CPluginDebug::CachePage::_BASE >( _T("") ) ) 
	(new CTypedFtInfo< CPluginDebug::LbaToHadd::_BASE >( _T("convert lba to h address") ) )
	//(new CTypedFtInfo< CPluginDebug::Connect::_BASE>(_T("connect to simulator device") ) )
	);

CPluginDebug::CPluginDebug(void)
{
};

CPluginDebug::~CPluginDebug(void)
{
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

void CPluginDebug::CachePage::Init(void)
{
	if (!m_smi_dev)
	{
		CSvtApplication::GetApp()->GetDevice(m_smi_dev);
	}

	// search and store cache blocks
	CCardInfo card_info;
	m_smi_dev->GetCardInfo(card_info);

	m_fblock_num = card_info.m_f_block_num;
	m_page_per_block = card_info.m_f_ppb;
	m_chunk_size = card_info.m_f_spck + 1;

	m_buf = new BYTE[m_chunk_size * SECTOR_SIZE];

	CFBlockInfo * cache_block = NULL;
	CFBlockInfo::m_channel_num = card_info.m_channel_num;
	
	for (JCSIZE bb = 0; bb < m_fblock_num; ++bb)
	{
		if (NULL == cache_block) cache_block = new CFBlockInfo;
		ReadSpare(bb, 0, cache_block);
		if (0x40 == (cache_block->m_id & 0xF0) )
		{
			m_cache_list.push_back(cache_block);
			cache_block = NULL;
		}
	}
	// 排序
	m_cache_list.sort( struct sort_cache() );

	// for debug
	BLOCK_INFO_LIST::iterator it, endit = m_cache_list.end();
	for (it = m_cache_list.begin(); it != endit; ++it)
	{
		JCASSERT(*it);
		LOG_DEBUG(_T("fb=%04X, sn=%02X"), (*it)->m_f_add.m_block, (*it)->m_spare.m_serial_no );
	}

	delete [] cache_block;

	// preapre for start
	m_cur_cache = m_cache_list.begin();
	m_cur_page = 0;
}

bool CPluginDebug::CachePage::InvokeOnce(void)
{
	if ( m_cur_cache == m_cache_list.end() )		return false;
	if (m_output)	m_output->Release(), m_output = NULL;
	
	CFBlockInfo * cache = (*m_cur_cache);
	JCASSERT(cache);

	stdext::auto_interface<BLOCK_ROW> row(new BLOCK_ROW);
	ReadSpare(cache->m_f_add.m_block, m_cur_page, static_cast<CFBlockInfo*>(row)/*, buf*/);
	row.detach(m_output);

	++ m_cur_page;
	if (m_cur_page >= m_page_per_block)	 ++ m_cur_cache, m_cur_page = 0;

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

void CPluginDebug::LbaToHadd::Init(void)
{
	stdext::auto_interface<ISmiDevice>	dev;
	CSvtApplication::GetApp()->GetDevice(dev);

	CCardInfo card_info;
	dev->GetCardInfo(card_info);

	m_page_size = card_info.m_f_spck * card_info.m_f_ckpp;
	m_block_size = card_info.m_f_ppb * m_page_size;
}

bool CPluginDebug::LbaToHadd::Invoke(void)
{
	if (!m_init)	Init();

	if (m_hadd)	m_hadd->Release(),	m_hadd= NULL;

	if (!m_src_op) return false;
	stdext::auto_interface<jcparam::IValue> val;
	m_src_op->GetResult(val);
	if ( !val ) return false;

	jcparam::GetSubVal(val, _T("lba"), m_lba);
	m_hadd = new BLOCK_ROW;

	m_hadd->m_spare.m_hblock = (WORD)(m_lba / m_block_size);
	m_hadd->m_spare.m_hpage = (WORD)((m_lba % m_block_size) / m_page_size);

	return true;
}


