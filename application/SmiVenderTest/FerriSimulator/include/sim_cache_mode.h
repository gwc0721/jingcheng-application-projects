#pragma once

#include <stdext.h>
#include <SmiDevice.h>
#include <list>

#define	LEAST_SPARE		3

#define INVALID_HPAGE	0xFFFFFFFF

// host写一个page时间
#define	T_WRITE_PAGE	500
#define T_CLEAN_PAGE	(180000/256)

class CCacheBlock
{
public:
	CCacheBlock(void);
	~CCacheBlock(void);

public:
	JCSIZE * m_pages;
	WORD m_valid_pages;
	// last page to program
	WORD m_last_page;

public:
	JCSIZE PageData(WORD pg)
	{
		JCASSERT(pg < m_page_per_block);
		return m_pages[pg];
	}
	// 使page无效，如果此block的所有page都无效，则返回false
	WORD InvalidPage(WORD pg);
	// 将hadd开始的1page写入Cache Block的最后一个page,
	// 如果写满，则返回false
	WORD Program(JCSIZE hadd);
	void Erase(void);
	// 返回第一个有效page的值
	JCSIZE FirstValidPage(void);

public:
	static JCSIZE m_page_per_block;
};

// h地址采用一个整数表示，hadd = hblock * page_per_block + hpage
class CLinkTable
{
public:
	CLinkTable(JCSIZE hblock_num, JCSIZE page_bits);
	~CLinkTable(void);
	
public:
	void SetCacheData(JCSIZE add, WORD cache_block, WORD cache_page);
	void GetCacheData(JCSIZE add, WORD &cache_block, WORD &cache_page);

	void Clean(JCSIZE hadd);
	JCSIZE GetWriteRange(void);

protected:
	JCSIZE m_write_range;
	JCSIZE m_page_cnt;	// h page count
	JCSIZE * m_link_tab;
	BYTE * m_hblock_map;

	JCSIZE	m_page_per_block;
	JCSIZE	m_hpage_mask, m_hpage_bits;
};

typedef std::list<WORD>	CacheBlockList;

class CSimCacheMode
	: /*virtual public IStorageDevice
	, */virtual public ISmiDevice
	, public CStorageDeviceBase
	, public CSmiDeviceBase
	/*, public CJCInterfaceBase*/
{
public:
	CSimCacheMode(void);
	~CSimCacheMode(void);

public:
	virtual bool SectorWrite(const BYTE * buf, FILESIZE lba, JCSIZE sectors);
	// 取得上次指令执行时间，单位 us
	virtual UINT GetLastInvokeTime(void);

	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr);
	virtual bool GetProperty(LPCTSTR prop_name, UINT & val);
	virtual bool SetProperty(LPCTSTR prop_name, UINT val);

protected:
	// 开始仿真前，准备各种buffer
	void Prepare(void);
	void InvalidCachePage(WORD cbk, WORD cpg);
	// 清h-block，从m_clean_add开始，pages:清除page数 
	void CleanPage(JCSIZE hadd);
	JCSIZE FindHblockToClean(void);


protected:
	FILESIZE	m_max_hpage;
	JCSIZE		m_clean_factor;
	JCSIZE		m_least_spare;

	CLinkTable		* m_link_tab;
	CCacheBlock		*m_caches_array;

	// cache block 列表
	CacheBlockList	m_caches;
	// spare block 列表
	CacheBlockList	m_spares;

	// 当前正在clean的h-block和h-page
	JCSIZE		m_clean_hadd;
	enum CLEAN_TASK
	{
		CLEAN_NON = 0,		CLEAN_PARTIAL = 1, 
		CLEAN_HBLOCK = 2,	CLEAN_CACHE_BLOCK = 3, 
	} m_clean_task;

	WORD	m_last_cbk;
	UINT	m_last_busy_time;
	WORD	m_quarter_block;

	JCSIZE	m_erase_count;

	// parameters
protected:
	// spare block number
	JCSIZE	m_spare_count;
	JCSIZE	m_sector_per_page;
	// 考虑到page number一定是2的整数次幂，从hadd中分离h-block和h-page的采用位运算，
	// 用于分离page的屏蔽位；分离block用的右移位数 
	JCSIZE	m_hpage_mask, m_hpage_bits;
	JCSIZE  m_page_per_block;
	JCSIZE	m_hblock_cnt;

	bool m_prepared;
};