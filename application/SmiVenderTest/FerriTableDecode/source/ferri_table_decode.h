#pragma once

#include <jcapp.h>

///////////////////////////////////////////////////////////////////////////////
//-- Logical to Physical table


#define HPAGE_PER_TABLE		1024
#define TABLE_SIZE			(4*1024)
#define MAX_CACHE_NUM		256

struct FLASH_ADDRESS
{
	WORD fblock;
	WORD fpage;		// 0xFFFF表示data in mother, f-block为mother block
	bool operator == (const FLASH_ADDRESS & x)
	{
		return (fblock == x.fblock) && (fpage == x.fpage);
	}
};

// 一个h-block的link table
class CHblockLink
{
public:
	CHblockLink(WORD page_per_block);
	~CHblockLink(void);

public:
	void SetLinkTable(BYTE * tab, JCSIZE offset, WORD *cache_index, JCSIZE index_size);
	void SetHblockInfo(WORD hblock, WORD mother)
	{
		m_hblock = hblock, m_mother = mother;
	}
	void Reset(void);

public:
	WORD m_page_per_block;
	WORD m_hblock;
	WORD m_mother;
	FLASH_ADDRESS * m_link;
};

// 1024 h-page的Link Table
class CSubLinkTable
{
public:
	CSubLinkTable(void);

public:
	void SetTable(BYTE * buf, JCSIZE buf_len);	// buf leng must = 4K

public:
	JCSIZE m_start_hpage;	// gloable h-page
	//WORD m_mother_list[4];
	FLASH_ADDRESS m_sub_table[HPAGE_PER_TABLE];
};

typedef CSubLinkTable *	P_SEGMENT;

class CL2PhTable
{
public:
	CL2PhTable(void);
	~CL2PhTable(void);

public:
	void Initialize(WORD hblock_num, WORD page_per_block);
	void Merge(CHblockLink * hlink, FILE * outfile);
	//void Difference(CSubLinkTable * sub_tab);
	//void SetSubTable(CSubLinkTable * sub_tab);

protected:
	//JCSIZE m_sub_tab_num;
	//P_SEGMENT * m_table;
	WORD m_hblock_num;
	//WORD m_page_per_block;
	JCSIZE m_total_pages;
	FLASH_ADDRESS * m_table;

};


///////////////////////////////////////////////////////////////////////////////
//--
class CTableDecodeApp : public jcapp::CJCAppSupport<jcapp::AppArguSupport>
{
public:
	static const TCHAR LOG_CONFIG_FN[];
	CTableDecodeApp(void);

public:
	virtual int Initialize(void);
	virtual int Run(void);
	virtual void CleanUp(void);
	virtual LPCTSTR AppDescription(void) const {
		return _T("");
	};

public:
	void AnalyseCacheInfo(FILE * src_file);
	void ReadCacheIndex(FILE * src_file, WORD * index_tab, JCSIZE max_index);

public:
	CJCStringT	m_config;
	CJCStringT	m_dps_fn;
	CJCStringT	m_data_fn;
	CJCStringT	m_cache_index_fn;

protected:
	CFileMapping	* m_file_mapping;
	CL2PhTable	m_table;

	JCSIZE		m_table_per_page;

	WORD m_cache_index[MAX_CACHE_NUM];
};

//const TCHAR CTableDecodeApp::LOG_CONFIG_FN[] = _T("cache_info.cfg");
