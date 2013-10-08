#include "stdafx.h"


#include "../include/sim_cache_mode.h"

LOCAL_LOGGER_ENABLE(_T("SimLT2244"), LOGGER_LEVEL_WARNING);

#define HPAGE(hadd)		(hadd & m_hpage_mask)
#define HBLOCK(hadd)	(hadd >> m_hpage_bits)

///////////////////////////////////////////////////////////////////////////////
//-- CCacheBlock

JCSIZE CCacheBlock::m_page_per_block = 256;

CCacheBlock::CCacheBlock()
	: m_valid_pages(0)
	, m_last_page(0)
	, m_pages(NULL)
{
	m_pages = new JCSIZE[m_page_per_block];
	memset(m_pages, 0xFF, sizeof(JCSIZE) * m_page_per_block );
}

CCacheBlock::~CCacheBlock(void)
{
	delete [] m_pages;
}


void CCacheBlock::Erase(void)
{
	memset(m_pages, 0xFF, sizeof(JCSIZE)* m_page_per_block);
	m_valid_pages = 0;
	m_last_page = 0;
}

WORD CCacheBlock::Program(JCSIZE hadd)
{
	JCASSERT(m_last_page < m_page_per_block);
	m_pages[m_last_page] = hadd;
	m_last_page ++;
	m_valid_pages ++;
	return  m_last_page; 
}

WORD CCacheBlock::InvalidPage(WORD pg)
{
	JCASSERT( pg < m_page_per_block);
	m_pages[pg] = INVALID_HPAGE;
	m_valid_pages --;
	return m_valid_pages;
}

JCSIZE CCacheBlock::FirstValidPage(void)
{
	WORD pp;
	for (pp = 0; (pp < m_last_page); ++pp)
	{
		if ( INVALID_HPAGE != m_pages[pp]) break;
	}
	JCASSERT(pp < m_page_per_block);
	return m_pages[pp];
}

///////////////////////////////////////////////////////////////////////////////
//-- CLinkTable
CLinkTable::CLinkTable(JCSIZE hblock_num, JCSIZE page_bits)
	: m_link_tab(NULL)
	, m_hblock_map(NULL)
	, m_write_range(0)
	, m_hpage_bits(page_bits)
{
	m_page_per_block = (1<<page_bits);
	m_hpage_mask = m_page_per_block - 1;

	m_page_cnt = hblock_num * m_page_per_block;
	m_link_tab = new JCSIZE[m_page_cnt];
	memset(m_link_tab, 0xFF, sizeof(JCSIZE) * m_page_cnt);
	m_hblock_map = new BYTE[hblock_num];
	memset(m_hblock_map, 0, sizeof(BYTE) * hblock_num);

}

CLinkTable::~CLinkTable(void)
{
	delete [] m_link_tab;
	delete [] m_hblock_map;
}

void CLinkTable::SetCacheData(JCSIZE add, WORD cache_block, WORD cache_page)
{
	JCASSERT(add < m_page_cnt);
	m_link_tab[add]=MAKELONG(cache_page, cache_block);
	WORD hbk = add / m_page_per_block;
	if (0 == m_hblock_map[hbk])
	{
		m_hblock_map[hbk] = 1;
		m_write_range ++;
		LOG_NOTICE(_T("h-page write, cover range = %d"), m_write_range);
	}
}

void CLinkTable::GetCacheData(JCSIZE add, WORD &cache_block, WORD &cache_page)
{
	JCASSERT(add < m_page_cnt);
	cache_block = HIWORD(m_link_tab[add]);
	cache_page = LOWORD(m_link_tab[add]);
	JCASSERT( MAKELONG(cache_page, cache_block) == m_link_tab[add]);
}

JCSIZE CLinkTable::GetWriteRange(void)
{
	return m_write_range;
}

void CLinkTable::Clean(JCSIZE hadd)
{
	JCASSERT(hadd < m_page_cnt);

	m_link_tab[hadd]=INVALID_HPAGE;

	if ( m_hpage_mask == HPAGE(hadd) )
	{
		WORD hbk = HBLOCK(hadd);
		if (0 != m_hblock_map[hbk])
		{
			m_hblock_map[hbk] = 0;
			m_write_range --;
			LOG_NOTICE(_T("h-page clean, cover range = %d"), m_write_range);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
//-- 


///////////////////////////////////////////////////////////////////////////////
//-- 

CSimCacheMode::CSimCacheMode(void)
	: m_caches_array(NULL)
	, m_clean_hadd(INVALID_HPAGE)
	, m_link_tab(NULL)
	, m_prepared(false)
	, m_erase_count(0)
{
	// default values
	m_spare_count = 255;
	m_sector_per_page = 64;
	m_page_per_block = 256;
	m_hpage_mask = m_page_per_block -1;
	m_hpage_bits = 8;
	m_hblock_cnt = 3776;

	m_least_spare = LEAST_SPARE;
}

CSimCacheMode::~CSimCacheMode(void)
{
	delete [] m_caches_array;
	delete m_link_tab;
}

void CSimCacheMode::Prepare(void)
{
	delete m_link_tab;
	delete [] m_caches_array;
	m_spares.clear();

	CCacheBlock::m_page_per_block = m_page_per_block;
	m_caches_array = new CCacheBlock[m_spare_count];
	for (JCSIZE ii = 0; ii<m_spare_count; ++ii)		m_spares.push_back(ii);
	m_last_cbk = m_spares.front(), m_spares.pop_front();
	m_caches.push_back(m_last_cbk);

	m_link_tab = new CLinkTable(m_hblock_cnt, m_hpage_bits);

	m_max_hpage = m_page_per_block * m_hblock_cnt;
	m_clean_factor = m_page_per_block / 4;
	m_quarter_block = m_page_per_block /4;
	m_erase_count = 0;
	m_clean_task = CLEAN_NON;
	m_prepared = true;
}

bool CSimCacheMode::SectorWrite(const BYTE * buf, FILESIZE lba, JCSIZE sectors)
{
	LOG_STACK_TRACE();
	//if (!m_prepared)	THROW_ERROR(ERR_USER, _T("need to run prepare first."));
	if (!m_prepared) Prepare();
	// lba -> hblock, hpage
	FILESIZE hpage0 = lba / m_sector_per_page;
	FILESIZE hpage1 = (lba + sectors -1) / m_sector_per_page;
	if (hpage1 >= m_max_hpage) THROW_ERROR(ERR_PARAMETER, _T("lba over range"));

	JCSIZE pages = (JCSIZE)(hpage1 - hpage0 + 1);
	JCSIZE hadd = (JCSIZE)(hpage0);

	m_last_busy_time = 0;

	// get cache info
	for (JCSIZE pp = 0; pp<pages; ++pp, ++hadd)
	{
		WORD cbk=0, cpg=0;
		m_link_tab->GetCacheData(hadd, cbk, cpg);

		// program to cache block
		CCacheBlock & last_cbk = m_caches_array[m_last_cbk];
		WORD last_cpg = last_cbk.Program(hadd);
		LOG_DEBUG(_T("write hpage=0x%06X to cache=%d, page=%02X"), hadd, m_last_cbk, last_cpg-1);

		// page在cache中，首先是cache中的page无效，
		if (0xFFFF != cbk)		
		{
			LOG_DEBUG(_T("invalid page hpage=0x%06X to cache=%d, page=%02X"), hadd, cbk, cpg);
			InvalidCachePage(cbk, cpg);
		}

		// update link table
		m_link_tab->SetCacheData(hadd, m_last_cbk, last_cpg -1);

		if ( last_cpg >= m_page_per_block )
		{	// pop up a new block
			if (m_spares.empty() )	THROW_ERROR(ERR_USER, _T("cache full !!"));

			m_last_cbk = m_spares.front();
			m_spares.pop_front();
			m_caches.push_back(m_last_cbk);
			LOG_NOTICE(_T("pop-up spare, cache=%d, spare=%d"), m_last_cbk, m_spares.size());
			LOG_NOTICE(_T("valid page = %d"), m_caches_array[m_last_cbk].m_valid_pages);
			JCASSERT(m_last_cbk < m_spare_count);
		}
		m_last_busy_time += T_WRITE_PAGE;
	}

	// clean
	JCSIZE spare_cnt = m_spares.size();
	// DataInCacheHblockCnt
	JCSIZE write_range = m_link_tab->GetWriteRange();
	//LOG_DEBUG(_T("write range = %d"), write_range);
	JCSIZE s2 = m_least_spare * 3;
	JCSIZE s1 = write_range / m_clean_factor + s2;

	if (spare_cnt <= m_least_spare)
	{
		// Force clean one cache block
		LOG_WARNING(_T("{!!} force clean one cache block"));
		//while ( m_least_spare >= m_spares.size() )
		//{	// clean 

		//}
	}
	else if (spare_cnt <= s2)
	{
		// Force clean one mother block
		LOG_WARNING(_T("{!!} force clean one mother block"));
		if (m_clean_task < CLEAN_HBLOCK)	m_clean_task = CLEAN_HBLOCK;
	}
	else if (spare_cnt <= s1)
	{
		LOG_NOTICE(_T("{!!} partial clean"));
		if (m_clean_task < CLEAN_PARTIAL) m_clean_task = CLEAN_PARTIAL;
	}

	// clean
	JCSIZE clean_pages = 0;

	switch (m_clean_task)
	{
	case CLEAN_PARTIAL:
		clean_pages = m_quarter_block;
		if (INVALID_HPAGE == m_clean_hadd) m_clean_hadd = FindHblockToClean();
		// else	
		break;

	case CLEAN_HBLOCK:
		if (INVALID_HPAGE == m_clean_hadd)
		{
			m_clean_hadd = FindHblockToClean();
			clean_pages = m_page_per_block;
		}
		else	clean_pages = m_page_per_block - HPAGE(m_clean_hadd);
		break;
	};

	while (clean_pages > 0)
	{
		CleanPage(m_clean_hadd);
		clean_pages --;
		m_clean_hadd ++;

	}
	if (0 == HPAGE(m_clean_hadd))
	{
		m_clean_hadd = INVALID_HPAGE;
		m_clean_task = CLEAN_NON;
	}

	// save time
	return true;
}


UINT CSimCacheMode::GetLastInvokeTime(void)
{
	return m_last_busy_time;
}

void CSimCacheMode::InvalidCachePage(WORD cbk, WORD cpg)
{
	LOG_STACK_TRACE();

	JCASSERT(cbk < m_spare_count);
	JCASSERT(cpg < m_page_per_block);

	CCacheBlock & cache = m_caches_array[cbk];
	WORD valid_page = cache.InvalidPage(cpg);
	LOG_DEBUG(_T("invalid cache=%d, page=%d, valid=%02X"), cbk, cpg, valid_page);
	if ( (0 == valid_page) && (cbk != m_last_cbk) )
	{	// 回收cache block
		LOG_NOTICE(_T("erase cache=%d, spare=%d"), cbk, m_spares.size());
		cache.Erase();
		m_caches.remove(cbk);
		m_spares.push_back(cbk);

		m_erase_count ++;
	}
}

void CSimCacheMode::CleanPage(JCSIZE hadd)
{
	LOG_SIMPLE_TRACE(_T(""));
	// invalid data in cache,
	WORD cbk=0, cpg=0;
	m_link_tab->GetCacheData(hadd, cbk, cpg);
	if (cbk != 0xFFFF)		InvalidCachePage(cbk, cpg);

	// clean in link table
	m_link_tab->Clean(hadd);
	m_last_busy_time += T_CLEAN_PAGE;
}

JCSIZE CSimCacheMode::FindHblockToClean(void)
{
	WORD cache_id = m_caches.front();
	LOG_NOTICE(_T("search clean mother in cache=%d"), cache_id);
	CCacheBlock & cache = m_caches_array[cache_id];
	
	// find first valiable page in cache block
	JCSIZE hadd = cache.FirstValidPage();
	JCASSERT(INVALID_HPAGE != hadd);
	return (hadd - HPAGE(hadd) );
}

bool CSimCacheMode::QueryInterface(const char * if_name, IJCInterface * &if_ptr)
{
	JCASSERT(NULL == if_ptr);
	bool br = false;
	if (if_name == IF_NAME_STORAGE_DEVICE ||
		strcmp(if_name, IF_NAME_STORAGE_DEVICE) == 0)
	{
		if_ptr = static_cast<IJCInterface*>(this);
		AddRef();
		br = true;
	}
	return br;
}

bool CSimCacheMode::GetProperty(LPCTSTR prop_name, UINT & val)
{
	if ( FastCmpT(prop_name, CSmiDeviceBase::PROP_CACHE_NUM ) )
	{
		val = m_caches.size();
		return true;
	}
	else return __super::GetProperty(prop_name, val);
	//_T("TOTAL_ERASE_COUNT");
}

bool CSimCacheMode::SetProperty(LPCTSTR prop_name, UINT val)
{
	if ( FastCmpT( prop_name, 	_T("SPARE_CNT") ) )
	{
		m_spare_count = val;
		m_prepared = false;
		return true;
	}
	else if ( FastCmpT( prop_name, 	_T("PAGE_SIZE") ) )
	{
		if (0 == val) return false;
		m_sector_per_page = val;
		m_prepared = false;
		return true;
	}
	else if ( FastCmpT( prop_name, 	_T("PAGE_NUM") ) )
	{
		UINT pages = val;
		int bits = 0;

		// 检查 page per block是2的整数次幂
		for (; (pages & 1) != 0; pages >>= 1, bits++);
		if (pages != 1)	return false;
		m_page_per_block = val;
		m_hpage_mask = m_page_per_block - 1;
		m_hpage_bits = bits;
		m_prepared = false;
		return true;
	}
	else if  ( FastCmpT( prop_name, 	_T("HBLOCK_CNT") ) )
	{
		m_hblock_cnt = val;
		m_prepared = false;
		return true;
	}
	else if ( FastCmpT(prop_name, _T("PREPARE")) )
	{
		Prepare();
		return true;
	}



	//_T("CLEAN_TH1");
	//_T("CLEAN_TH2");
	//_T("CLEAN_TH3");
	//
	//_T("T_WRITE_PAGE");
	//_T("T_CLEAN_PAGE");

	else return false;
}
