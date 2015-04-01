#pragma once

#include "../../CoreMntLib/include/mntImage.h"
#include "driver_factory.h"
#include "../../DeviceConfig/DeviceConfig.h"

#define LODWORD(a)			((ULONG32)(a & 0xFFFFFFFF))
#define HIDWORD(a)			((ULONG32)((a >> 32) & 0xFFFFFFFF))

///////////////////////////////////////////////////////////////////////////////
//-- 
#define SPARE_SIZE	32
#define INDEX_BLOCK	5

#define BLOCKID_DATA			0x20
#define	BLOCKID_EOB				0x28
#define BLOCKID_ZONE_INDEX		0x68
#define BLOCKID_ZONE_TAB		0x66
#define BLOCKID_GROUP_TAB		0x64
#define BLOCKID_INDEX			0xE5
#define BLOCKID_LINK			0xE6


#define FIRST_DATA_BLOCK	10


// 模拟NAND flash的文件
enum NAND_STATUS
{
	NS_OK,
	NS_PROGRAM_FAIL,
	NS_ERASE_FAIL,
	NS_ECC_FAIL,
	NS_OVER_PROGRAM,
	NS_EMPTY_PAGE,
	NS_WRONG_ADD,
};

struct FLASH_INFO
{
	JCSIZE page_size;		// 单位 sectors
	JCSIZE page_per_block;
	JCSIZE blocks;
	JCSIZE spare_len;		// 每个page的spare大小，单位byte
	JCSIZE spare_pages;
	JCSIZE block_len;		// 对齐后的block大小
};

struct NAND_FILE_MAPPING
{
	JCSIZE	block_id;
	UCHAR*	buf;
};

#define MAX_CACHE_BLOCKS		32

///////////////////////////////////////////////////////////////////////////////
//-- 
class CFileNand
{
public:
	CFileNand(const CJCStringT & file_name);
	~CFileNand(void);
	void Pretest(JCSIZE page_size, JCSIZE page_per_block, JCSIZE block_num, JCSIZE spare_len);
	bool IsInitialized(void) {return m_info_ready;}

public:
	virtual NAND_STATUS ProgramPage(JCSIZE block, JCSIZE page, LPVOID data, JCSIZE data_len, LPVOID spare);
	virtual NAND_STATUS ReadPage(JCSIZE block, JCSIZE page, LPVOID data, JCSIZE data_len, LPVOID spare);
	virtual NAND_STATUS EraseBlock(JCSIZE block);

public:
	const FLASH_INFO * GetFlashInfo(void) const {return m_flash_info;}

protected:
	bool ReadFlashInfo(void);
	void Initialize(void);
	// 把block的map到内存中
	BYTE MapNandBlock(JCSIZE block);

protected:
	// 模拟NAND flash的文件
	HANDLE	m_nand_file;
	HANDLE	m_nand_mapping;
	
	FLASH_INFO *	m_flash_info;
	// 用于记录original bad block信息
	BYTE	* m_org_bad;
	WORD	* m_last_page_tab;
	ULONG32	* m_erase_count_tab;

	JCSIZE	m_page_len;	// in byte
	JCSIZE	m_block_len;
	JCSIZE	m_spare_start;	// spare在block中的offset，单位byte
	JCSIZE	m_spare_len;

	// 用于标志可被淘汰的 cache 位置
	int	m_cache_swap_out;

	// 下标为block id，值表示该block在m_mapping_tab中的下标, 0xFF表示没有cache
	BYTE * m_mapping_index;
	NAND_FILE_MAPPING m_mapping_tab[MAX_CACHE_BLOCKS];

	// 从文件中读取到card mode
	bool m_info_ready;
};

///////////////////////////////////////////////////////////////////////////////
//--

class CPageModeDriver;

#define MAX_ZONE_NUM	64
#define	INVALID_BLOCK_NUM		0xFFFF


class CBlockBase
{
public:
	CBlockBase(JCSIZE block_num = INVALID_BLOCK_NUM);
	virtual void Attach(CFileNand * nand, JCSIZE block_num/*, bool empty = false*/) = 0;
	bool FindLastPage(CFileNand * nand, JCSIZE page_id, LPVOID buf, JCSIZE buf_len);
	bool IsFull() {return m_last_page >= m_page_per_block;}

	virtual JCSIZE Close(void);
	virtual void PopUp(JCSIZE block) {JCASSERT(m_block_num == INVALID_BLOCK_NUM); m_block_num = block;}


public:
	bool	m_dirty;
	WORD	m_block_num;
	BYTE	m_block_id;
	BYTE	m_erase_count;
	WORD	m_last_page;
	static JCSIZE m_page_per_block;
};



class CIndexBlock : public CBlockBase
{
public:
	virtual void Attach(CFileNand * nand, JCSIZE block_num);
	
	JCSIZE GetZoneIndex(void) {return m_index_page.zone_index;}
	void SetZoneIndex(JCSIZE zone_index)	{m_index_page.zone_index = zone_index; m_updated++;}
	JCSIZE GetLinkBlock(void) const {return m_index_page.link_block;}
	void SetLinkBlock(JCSIZE link_block)	{m_index_page.link_block = link_block; m_updated++;}
public:
	//bool		m_dirty;
	JCSIZE m_updated;
	struct INDEX_PAGE
	{
		BYTE dummu[16];
		WORD head_block;
		WORD link_block;
		WORD wpro_block;
		WORD zone_index;
		WORD log_block;
	} m_index_page;
};


class CLinkBlock : public CBlockBase
{
public:
	CLinkBlock(JCSIZE page_len, JCSIZE blocks);
	~CLinkBlock(void);
	virtual void Attach(CFileNand * nand, JCSIZE block_num);

	void ReadLinkTab(CFileNand * nand, CPageModeDriver * driver);
	void ReadPage(CFileNand * nand, JCSIZE page);
	void SavePage(CFileNand * nand, JCSIZE page);

public:
	struct LINK_ITEM {
		WORD	next_block;
		BYTE	valid_page;
		BYTE	erase_count;
	};
	LINK_ITEM * m_link_page;

	JCSIZE m_block_per_page;	// 一个page中可以记录的block个数
	JCSIZE m_page_num;			// 需要多少page记录来记录所有block

	// 保存有效link的第一个page，从此之后连续m_page_num为block link table
	JCSIZE m_group_start;
};


class CMapBlock: public CBlockBase
{
public:
	CMapBlock(JCSIZE group_num, JCSIZE zone_id);
	~CMapBlock(void);
	virtual void Attach(CFileNand * nand, JCSIZE block_num);

	JCSIZE GetGroupPage(JCSIZE group)	{return m_zone_tab[group];}
	void SetGroupPage(JCSIZE group, JCSIZE page)	{m_zone_tab[group] = page; m_updated++;};

	// 读取map block的下一页，并且更新last page，如果下一页为空，返回0xFFFF，否则返回page num
	JCSIZE ReadNextPage(CFileNand * nand, LPVOID buf, JCSIZE buf_len, LPVOID spare);

	void SaveZoneTable(CFileNand* nand);
	JCSIZE SaveGroupTable(CFileNand* nand, LPVOID buf, JCSIZE buf_len, JCSIZE group_id);

public:
	JCSIZE	m_zone_id;

	JCSIZE	m_updated;
	WORD	* m_zone_tab;
	JCSIZE	m_group_num;
	JCSIZE	m_zone_tab_page;
};

class CActiveDataBlock: public CBlockBase
{
public:
	//CActiveDataBlock(JCSIZE block_num, JCSIZE page_per_block);
	CActiveDataBlock(DWORD sn);
	~CActiveDataBlock(void);
	virtual void Attach(CFileNand * nand, JCSIZE block_num/*, bool empty = false*/);
	// 返回写入的page
	JCSIZE WriteData(CFileNand * nand, JCSIZE hpage, UCHAR * buf, JCSIZE buf_len);
	void SaveEOB(CFileNand * nand);

	virtual JCSIZE Close(void);

public:
	//JCSIZE /*m_page_per_block,*/ m_block_num;
	DWORD * m_eob_page;
	DWORD	m_data_sn;
};


// 定义一个group大小的(h-page to f-page) table, 每个item 4 byte，一个table大小为一个page， 
// 对应NAND中的一个map page

class CGroupTable
{
public:
	CGroupTable(JCSIZE group, JCSIZE page_per_group);
	~CGroupTable(void);
	void GetNandAddress(JCSIZE hpage, JCSIZE & fblock, JCSIZE & fpage);
	void SetNandAddress(JCSIZE hpage, JCSIZE fblock, JCSIZE fpage);

	void BuildTable(CFileNand * nand, JCSIZE map_block, JCSIZE page);
	void BuildTable(UCHAR * buf, JCSIZE page = 0);

	JCSIZE Flush(CFileNand * nand, CMapBlock * block);

public:
	JCSIZE	m_group_id;
	JCSIZE	m_page_per_group;
	bool	m_dirty;
	DWORD	* m_nand_add;


};

class CZoneTable
{
public:
	CZoneTable(JCSIZE group_num, JCSIZE page_per_group, JCSIZE zone_id);
	~CZoneTable(void);

	void GetNandAddress(JCSIZE group, JCSIZE hpage, JCSIZE & fblock, JCSIZE & fpage);
	void SetNandAddress(JCSIZE group, JCSIZE hpage, JCSIZE fblock, JCSIZE fpage);

	void BuildTable(CFileNand * nand, JCSIZE map_block);
	JCSIZE	Flush(CFileNand * nand, CPageModeDriver * driver);

protected:
	CMapBlock	m_map_block;

	CGroupTable * * m_group_tab;
	// 记录有多少group被更新过。
	JCSIZE m_updated_group;
	//bool m_dirty;
	JCSIZE m_group_num;
	JCSIZE m_page_per_group;
	JCSIZE m_zone_id;
};


class CZoneIndexBlock: public CBlockBase
{
public:
	virtual void Attach(CFileNand * nand, JCSIZE block_num/*, bool empty=false*/);

	JCSIZE GetMapBlock(JCSIZE zone) {return m_zone_index_page.map_block[zone];};
	void SetMapBlock(JCSIZE zone, JCSIZE block) {m_zone_index_page.map_block[zone] = block; m_updated ++;}
	void Save(CFileNand * nand);

public:
	JCSIZE m_updated;
	struct ZONE_INDEX
	{
		BYTE dummy[12];
		WORD map_block[MAX_ZONE_NUM];
	}	m_zone_index_page;
};

class CMappingTable
{
public:
	CMappingTable(JCSIZE zone_num, JCSIZE group_per_zone, JCSIZE page_per_group);
	~CMappingTable(void);

	void GetNandAddress(JCSIZE hpage, JCSIZE & fblock, JCSIZE & fpage);
	void SetNandAddress(JCSIZE hpage, JCSIZE fblock, JCSIZE fpage);

	// zone_index: zone index block number
	void BuildTable(CFileNand * nand, JCSIZE zone_index);
	// 如果zone index block被换过，返回新的block id，否则返回invalid block num。错误返回0
	JCSIZE Flush(CFileNand* nand, CPageModeDriver * driver);

protected:
	void HpageDecode(JCSIZE hpage, JCSIZE &zone, JCSIZE &group, JCSIZE &page);

protected:
	CZoneTable * *	m_zone_tab;
	CZoneIndexBlock	m_zone_block;

	//bool m_dirty;
	JCSIZE m_zone_num;
	JCSIZE m_group_per_zone;
	JCSIZE m_page_per_group;
};

// page spare area (meta data)
struct META_DATA
{
	META_DATA(void) {memset(this, 0, sizeof(META_DATA));}
	void clear(void)	{memset(this, 0, sizeof(META_DATA));}
	BYTE id;	// block id
	BYTE pe;	// erase count
	union	{
		struct	_ZONE	{// for map block and zone index (0xFF)
			BYTE zone_id;	
			WORD group_id;
		}	map;
		struct	_DATA	{
			DWORD hpage;
			DWORD sn;
		}	data;
	};

	BYTE dummy[32];
};

// 用于DataBlock和SpareBlock块管理
class CBlockChainNode
{
public:
	WORD m_pre_block;
	WORD m_next_block;
	DWORD m_erase_count;
	WORD m_valid_page;		// data block的有效page
};


///////////////////////////////////////////////////////////////////////////////
// -- block management
//  主要任务：管理各block信息，各Block的用途，empty block管理(pop up and erase)，
//  block erase count管理，data chain管理



///////////////////////////////////////////////////////////////////////////////
// -- page mode driver
class CPageModeDriver : public IImage
{
public:
	CPageModeDriver(const CJCStringT & config);
	~CPageModeDriver(void);

	IMPLEMENT_INTERFACE;

public:
	// interface
	virtual ULONG32	Read(UCHAR * buf, ULONG64 lba, ULONG32 secs);
	virtual ULONG32	Write(const UCHAR * buf, ULONG64 lba, ULONG32 secs);
	virtual ULONG32	FlushCache(void);
	virtual ULONG32	DeviceControl(ULONG code, READ_WRITE read_write, UCHAR * buf, ULONG32 & data_size, ULONG32 buf_size);
	virtual ULONG64	GetSize(void) const;

	// 说明用途
	JCSIZE PopupEmptyBlock(BYTE block_id);
	void EraseBlock(JCSIZE block);

	//void InsertEmptyBlock(JCSIZE block);
	//void InsertDataBlock(JCSIZE block

protected:
	void Pretest(CDeviceConfig & config);
	void LoadFlashInfo(CFileNand * nand);
	//void BuildLink(CFileNand * nand);

	void LBA2HPage(ULONG64 lba, JCSIZE & hpage, JCSIZE &offset);
	void HpageDecode(JCSIZE hpage, JCSIZE& zone, JCSIZE &group, JCSIZE &page);

	//bool FindLastPage(JCSIZE block, JCSIZE page_id, JCSIZE & last_page, UCHAR * buf, JCSIZE buf_len);
	void WriteHPage(JCSIZE hpage, UCHAR * buf);
	void ReadHPage(JCSIZE hpage, UCHAR * buf);

	void InsertEmptyBlock(JCSIZE block);
	void InsertDataBlock(JCSIZE block, JCSIZE next, JCSIZE valid_page);

	void ReadLinkTable();
	void SaveLinkTable(CLinkBlock * link_block);


protected:
	CFileNand *		m_file_nand;
	CDeviceConfig	m_config;

	JCSIZE m_page_size, m_page_len;
	JCSIZE m_page_per_block;
	JCSIZE m_blocks;

	// index and tables
	CMappingTable	* m_mapping_tab;
	CIndexBlock		m_index_block;

	CActiveDataBlock	* m_active_0;

	// 一个group中包含hpage数量，= page len / 4;
	JCSIZE m_page_per_group;
	JCSIZE m_group_per_zone;
	JCSIZE m_zone_num;


	JCSIZE m_link_lastpage;
	// data block link table
	// data block的链表，下标为f-block number，0x0000表示active 0, 0xFFFF表示empty block，
	//	0xFFXX表示id为XX的block，其他表示data chain
	// 
	//WORD * m_data_chain;
	CLinkBlock	* m_link_block;

	// for data blocks
	WORD m_head_block, m_tail_block;	// block num of active 0
	WORD m_spare_head, m_spare_tail;		// 指向第一个spare block

	CBlockChainNode * m_block_chain;

	//<TODO> save and load below var
	JCSIZE	m_base_erase_count;
	DWORD	m_serial_number;
	
};

