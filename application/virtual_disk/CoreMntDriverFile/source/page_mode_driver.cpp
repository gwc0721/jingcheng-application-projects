#include "stdafx.h"
#include <stdext.h>

#include "page_mode_driver.h"

LOCAL_LOGGER_ENABLE(_T("page_mode"), LOGGER_LEVEL_DEBUGINFO);

//--
JCSIZE	CBlockBase::m_page_per_block = 0;

CBlockBase::CBlockBase(JCSIZE block_num)
: m_block_num(block_num), m_last_page(0), m_block_id(0xFF)
{
}

bool CBlockBase::FindLastPage(CFileNand * nand, JCSIZE page_id, LPVOID buf, JCSIZE buf_len)
{
	JCASSERT(nand);
	META_DATA	spare;

	m_last_page = m_page_per_block;
	while (m_last_page >0)
	{
		NAND_STATUS st = nand->ReadPage(m_block_num, m_last_page -1, reinterpret_cast<UCHAR*>(buf), buf_len, reinterpret_cast<UCHAR*>(&spare) );
		if (spare.id == page_id) break;
		m_last_page --;
	}
	return (m_last_page != 0);
}

JCSIZE CBlockBase::Close(void)
{
	JCSIZE block = m_block_num;
	m_block_num = INVALID_BLOCK_NUM;
	m_last_page = 0;

	return block;
}

void CIndexBlock::Attach(CFileNand * nand, JCSIZE block_num)
{
	m_block_num = block_num;
	bool br = FindLastPage(nand, BLOCKID_INDEX, &m_index_page, sizeof(INDEX_PAGE) );
	if (!br) THROW_ERROR(ERR_USER, _T("no valid page in index block!"));
	if (m_index_page.zone_index == INVALID_BLOCK_NUM)	THROW_ERROR(ERR_USER, _T("zone map block is not valid"));
	m_updated = 0;
}

//--
void CZoneIndexBlock::Attach(CFileNand * nand, JCSIZE block_num/*, bool empty*/)
{
	m_block_num = block_num;
	bool br = FindLastPage(nand, BLOCKID_ZONE_INDEX, &m_zone_index_page, sizeof(ZONE_INDEX) );
	if (!br) THROW_ERROR(ERR_USER, _T("no valid page in zone map block!"));
	m_dirty = false;
	m_updated = 0;
}

void CZoneIndexBlock::Save(CFileNand * nand)
{
	META_DATA spare;
	spare.id = BLOCKID_ZONE_INDEX;
	spare.map.zone_id = 0xFF;
	NAND_STATUS st = nand->ProgramPage(m_block_num, m_last_page, &m_zone_index_page, sizeof(ZONE_INDEX), &spare);
	m_last_page ++;
}

//--
CMapBlock::CMapBlock(JCSIZE group_num, JCSIZE zone_id)
: m_zone_tab(NULL), m_zone_id(zone_id)
{
	m_zone_tab = new WORD[group_num];
}

CMapBlock::~CMapBlock(void)
{
	delete [] m_zone_tab;
}

void CMapBlock::Attach(CFileNand * nand, JCSIZE block_num)
{
	m_block_num = block_num;
	bool br = FindLastPage(nand, BLOCKID_ZONE_TAB, m_zone_tab, sizeof(WORD)*m_group_num );
	m_zone_tab_page = m_last_page -1;
	m_dirty = false;
}

JCSIZE CMapBlock::ReadNextPage(CFileNand * nand, LPVOID buf, JCSIZE buf_len, LPVOID spare)
{
	JCASSERT(nand);
	NAND_STATUS st = nand->ReadPage(m_block_num, m_last_page, reinterpret_cast<UCHAR*>(buf), buf_len, reinterpret_cast<UCHAR*>(spare) );
	if (st == NS_EMPTY_PAGE) return 0xFFFF;
	JCSIZE page = m_last_page;
	m_last_page ++;
	return page;
}

void CMapBlock::SaveZoneTable(CFileNand* nand)
{
	META_DATA spare;
	spare.id = BLOCKID_ZONE_TAB;
	spare.map.zone_id = m_zone_id;
	NAND_STATUS st = nand->ProgramPage(m_block_num, m_last_page, m_zone_tab, sizeof(WORD) * m_group_num, &spare);
	m_last_page ++;
}

JCSIZE CMapBlock::SaveGroupTable(CFileNand* nand, LPVOID buf, JCSIZE buf_len, JCSIZE group_id)
{
	if (m_last_page + 1 >= m_page_per_block)	return INVALID_BLOCK_NUM;
	META_DATA spare;
	spare.id = BLOCKID_GROUP_TAB;
	spare.map.zone_id = m_zone_id;
	spare.map.group_id = group_id;
	NAND_STATUS st = nand->ProgramPage(m_block_num, m_last_page, buf, buf_len, &spare);
	JCSIZE page = m_last_page;
	m_last_page ++;
	return page;
}

//--
//CActiveDataBlock::CActiveDataBlock(JCSIZE block_num, JCSIZE page_per_block)
//: m_page_per_block(page_per_block), m_block_num(block_num)
//, m_eob_page(NULL)
//{
//	m_eob_page = new DWORD[m_page_per_block];
//	memset(m_eob_page, 0xFF, sizeof(DWORD) * m_page_per_block);
//}

CActiveDataBlock::CActiveDataBlock(DWORD sn)
: m_eob_page(NULL), m_data_sn(sn)
{
	m_eob_page = new DWORD[m_page_per_block];
	memset(m_eob_page, 0xFF, sizeof(DWORD) * m_page_per_block);
}

CActiveDataBlock::~CActiveDataBlock(void)
{
	delete m_eob_page;
}

void CActiveDataBlock::Attach(CFileNand * nand, JCSIZE block_num/*, bool empty*/)
{
	JCASSERT(m_block_num == INVALID_BLOCK_NUM);
	m_block_num = block_num;
	// 找到最后一个EOB
	bool br = FindLastPage(nand, BLOCKID_EOB, m_eob_page, sizeof(WORD)*m_page_per_block );
	m_dirty = false;
	// 搜索EOB以后的 page，并且update eob
	for (; m_last_page < m_page_per_block; ++m_last_page)
	{
		META_DATA spare;
		NAND_STATUS st = nand->ReadPage(m_block_num, m_last_page, NULL, 0, &spare);
		if (st == NS_EMPTY_PAGE) break;
		m_eob_page[m_last_page] = spare.data.hpage;
		m_dirty = true;
	}
}

JCSIZE CActiveDataBlock::WriteData(CFileNand * nand, JCSIZE hpage, UCHAR * buf, JCSIZE buf_len)
{
	LOG_STACK_TRACE();
	LOG_NOTICE(_T("[NAND] write hpage 0x%08X to block:0x%04X, page:0x%04X"), hpage, m_block_num, m_last_page);

	META_DATA spare;
	spare.id= BLOCKID_DATA;
	spare.data.hpage = hpage;
	spare.data.sn = m_data_sn;
	NAND_STATUS st = nand->ProgramPage(m_block_num, m_last_page, buf, buf_len, &spare);
	JCASSERT(st == NS_OK);

	JCSIZE page = m_last_page;	m_last_page ++;
	m_eob_page[page] = hpage;		// update eob

	// auto eob
	if ( (m_last_page + 1) == m_page_per_block)	SaveEOB(nand);
	return page;
}

void CActiveDataBlock::SaveEOB(CFileNand * nand)
{
	LOG_STACK_TRACE();
	LOG_DEBUG(_T("save EOB to block:0x%04X, page:0x%04X"), m_block_num, m_last_page);

	META_DATA spare;
	spare.id= BLOCKID_EOB;
	spare.data.sn = m_data_sn;
	
	nand->ProgramPage(m_block_num, m_last_page, m_eob_page, sizeof(DWORD) * m_page_per_block, &spare);
	m_last_page ++;
}

JCSIZE CActiveDataBlock::Close(void)
{
	m_data_sn = 0xFFFFFFFF;
	memset(m_eob_page, 0xFF, sizeof(DWORD) * m_page_per_block);
	return __super::Close();
}


//--
CLinkBlock::CLinkBlock(JCSIZE page_len, JCSIZE blocks)
: m_link_page(NULL), m_block_per_page(0), m_page_num(0)
, m_group_start(0)
{
	m_block_per_page = page_len / sizeof(LINK_ITEM);
	m_page_num = (blocks -1) / m_block_per_page + 1; 
	m_link_page = new LINK_ITEM[m_block_per_page];
}

CLinkBlock::~CLinkBlock(void)
{
	delete [] m_link_page;
}

void CLinkBlock::Attach(CFileNand * nand, JCSIZE block_num)
{
	JCASSERT(m_block_num == INVALID_BLOCK_NUM);
	m_block_num = block_num;

	// 找到最后一个 LINK PAGE
	bool br = FindLastPage(nand, BLOCKID_LINK, m_link_page, sizeof(WORD)*m_page_per_block );
	m_group_start = m_last_page-1;
}

void CLinkBlock::ReadPage(CFileNand * nand, JCSIZE page)
{
	META_DATA	spare;
	NAND_STATUS st = nand->ReadPage(m_block_num, m_group_start + page, m_link_page, sizeof(LINK_ITEM) * m_block_per_page, &spare);
}

void CLinkBlock::SavePage(CFileNand * nand, JCSIZE page)
{
	META_DATA	spare;
	if (page == 0) spare.id = BLOCKID_LINK;
	else			spare.id = BLOCKID_LINK + 1;
	NAND_STATUS st = nand->ProgramPage(m_block_num, m_group_start + page, m_link_page, sizeof(LINK_ITEM) * m_block_per_page, &spare);
}

//--
CPageModeDriver::CPageModeDriver(const CJCStringT & config)
: m_file_nand(NULL)
, m_mapping_tab(NULL)
, m_active_0(NULL), m_block_chain(NULL), m_link_block(NULL)
{
	//try 
	//{
	m_config.LoadFromFile(config);
	m_file_nand = new CFileNand(m_config.m_storage_file);

	if ( !m_file_nand->IsInitialized() )	Pretest(m_config);
	LoadFlashInfo(m_file_nand);
	//}
	//catch (stdext::CJCException & err)
	//{
	//	_tprintf(_T("err, %s\n"), err.WhatT() );
	//}
}

CPageModeDriver::~CPageModeDriver(void)
{
	//delete [] m_data_chain;
	delete [] m_block_chain;
	delete m_mapping_tab;
	delete m_file_nand;
	delete m_link_block;
}

void CPageModeDriver::Pretest(CDeviceConfig & config)
{
	JCSIZE page_size = config.m_chunk_size * config.m_chunk_per_page;
	m_file_nand->Pretest(page_size, config.m_page_per_block, config.m_block_num, SPARE_SIZE);

	const FLASH_INFO * flash_info = m_file_nand->GetFlashInfo();
	m_page_size = flash_info->page_size;
	m_page_len = SECTOR_TO_BYTE(m_page_size);
	m_page_per_block = flash_info->page_per_block;
	CBlockBase::m_page_per_block = m_page_per_block;
	m_blocks = flash_info->blocks;

	m_base_erase_count = 0;

	META_DATA spare;
	// initlaize and create link block 
	// 初始化 block chain，设置所有block为empty block
	m_block_chain = new CBlockChainNode[m_blocks];
	memset(m_block_chain, 0xFF, sizeof(CBlockChainNode) * m_blocks);
	for (JCSIZE bb = FIRST_DATA_BLOCK; bb < m_blocks; ++bb)
	{
		m_block_chain[bb].m_erase_count = 0;
		m_block_chain[bb].m_next_block = bb + 1;
		m_block_chain[bb].m_pre_block = bb -1;
	}
	m_spare_head = FIRST_DATA_BLOCK;	m_block_chain[m_spare_head].m_pre_block = 0;
	m_spare_tail = m_blocks - 1;		m_block_chain[m_spare_tail].m_next_block = 0;

	m_head_block = INVALID_BLOCK_NUM;
	m_tail_block = INVALID_BLOCK_NUM;
	m_serial_number = 0;

	CIndexBlock::INDEX_PAGE index_page;
	memset(&index_page, 0xFF, sizeof(CIndexBlock::INDEX_PAGE) );

	// initlaize and create zone index block
	CZoneIndexBlock::ZONE_INDEX zone_index;
	memset(&zone_index, 0xFF, sizeof(CZoneIndexBlock::ZONE_INDEX) );
	JCSIZE block = PopupEmptyBlock(BLOCKID_ZONE_INDEX);

	memset(&spare, 0, SPARE_SIZE);
	spare.id = BLOCKID_ZONE_INDEX;
	spare.map.zone_id = 0xFF;

	NAND_STATUS st = m_file_nand->ProgramPage(block, 0, 
		reinterpret_cast<UCHAR*>(&zone_index), sizeof(CZoneIndexBlock::ZONE_INDEX), reinterpret_cast<UCHAR*>(&spare) );
	JCASSERT(st == NS_OK);
	m_block_chain[block].m_valid_page = 1;

	index_page.zone_index = block;

	// save link
	block = PopupEmptyBlock(BLOCKID_LINK);
	m_block_chain[block].m_valid_page = 0;
	CLinkBlock link_block(m_page_len, m_blocks);
	link_block.m_block_num = block;
	SaveLinkTable(&link_block);
	m_block_chain[block].m_valid_page = link_block.m_group_start;
	//delete link_block;
	index_page.link_block = block;

	// update index block
	memset(&spare, 0, SPARE_SIZE);
	spare.id = BLOCKID_INDEX;
	st = m_file_nand->ProgramPage(INDEX_BLOCK, 0, reinterpret_cast<UCHAR*>(&index_page), sizeof(CIndexBlock::INDEX_PAGE), reinterpret_cast<UCHAR*>(&spare) );
	JCASSERT(st == NS_OK);

	delete [] m_block_chain; m_block_chain = NULL;
}

void CPageModeDriver::LoadFlashInfo(CFileNand * nand)
{
	LOG_STACK_TRACE();
	const FLASH_INFO * flash_info = nand->GetFlashInfo();
	m_page_size = flash_info->page_size;
	m_page_len = SECTOR_TO_BYTE(m_page_size);
	m_page_per_block = flash_info->page_per_block;
	CBlockBase::m_page_per_block = m_page_per_block;
	m_blocks = flash_info->blocks;

	m_head_block = INVALID_BLOCK_NUM;
	m_tail_block = INVALID_BLOCK_NUM;
	m_spare_head = 0;		m_spare_tail = 0;
	m_base_erase_count = 0;
	m_serial_number = 0;

	// 
	JCASSERT (NULL == m_block_chain);	
	m_block_chain = new CBlockChainNode[m_blocks];
	memset(m_block_chain, 0xFF, sizeof(CBlockChainNode) * m_blocks);

	JCASSERT(NULL == m_link_block);
	m_link_block = new CLinkBlock(m_page_len, m_blocks);
	//

	m_page_per_group = SECTOR_TO_BYTE(m_page_size) / 4;
	// zone num = hblock / 256 = (total_sector / block_size) / 256
	JCSIZE hblock_num = (m_config.m_total_sec-1) / (m_page_size * m_page_per_block) +1;
	LOG_DEBUG(_T("h-block num = %d"), hblock_num);
	m_zone_num = (hblock_num -1) /256 +1;
	// group per zone = total_group / zone num = (total sec / group size)
	JCSIZE total_group = ( m_config.m_total_sec / (m_page_size * m_page_per_group) );
	m_group_per_zone = (total_group-1) / m_zone_num + 1;
	LOG_DEBUG(_T("total group = %d"), total_group);
	LOG_DEBUG(_T("page/gp=%d, gp/zone=%d, zone=%d"), m_page_per_group, m_group_per_zone, m_zone_num );
	LOG_DEBUG(_T("total sec = %d"), m_page_per_group * m_group_per_zone * m_zone_num * m_page_size);
	JCASSERT(m_zone_num <= MAX_ZONE_NUM);

	m_mapping_tab = new CMappingTable(m_zone_num, m_group_per_zone, m_page_per_group);
	m_index_block.Attach(m_file_nand, INDEX_BLOCK);
	m_mapping_tab->BuildTable(nand, m_index_block.GetZoneIndex() );

	// restore link block
	m_link_block->Attach(m_file_nand, m_index_block.GetLinkBlock());
	ReadLinkTable();
}

void CPageModeDriver::ReadLinkTable()
{
	JCSIZE bb = FIRST_DATA_BLOCK, page = 0;
	m_link_block->ReadPage(m_file_nand, page);
	for (JCSIZE block = FIRST_DATA_BLOCK; block < m_blocks; ++block)
	{
		CLinkBlock::LINK_ITEM * link_item= m_link_block->m_link_page + bb;
		WORD next = link_item->next_block;
		m_block_chain[block].m_erase_count = m_base_erase_count + link_item->erase_count; 
		// empty block, insert to spare block chain
		if (next == INVALID_BLOCK_NUM)	InsertEmptyBlock(block);
		// data blocks, inser to data chain
		else if (next < 0xFF00)	InsertDataBlock(block, link_item->next_block, link_item->valid_page );
		//else if // original bad
		//else if // new bad
		else // system blocks
		{
			m_block_chain[block].m_next_block = link_item->next_block;
			m_block_chain[block].m_pre_block = 0;
			m_block_chain[block].m_valid_page = link_item->valid_page;
		}
		++bb;
		if (bb >= m_link_block->m_block_per_page)
		{
			m_link_block->ReadPage(m_file_nand, page);
			bb = 0;	page ++;
		}
	}

	//JCSIZE block = 0;	// block
	//for (JCSIZE ii =0; ii < m_link_block->m_page_num; ++ii)
	//{
	//	m_link_block->ReadPage(m_file_nand, ii);
	//	for (JCSIZE bb=0; bb < m_link_block->m_block_per_page; ++bb, ++block)
	//	{
	//		if (block < FIRST_DATA_BLOCK)	continue;
	//		CLinkBlock::LINK_ITEM * link_item= m_link_block->m_link_page + bb;
	//		WORD next = link_item->next_block;
	//		m_block_chain[block].m_erase_count = m_base_erase_count + link_item->erase_count; 
	//		// empty block, insert to spare block chain
	//		if (next == INVALID_BLOCK_NUM)	InsertEmptyBlock(block);
	//		// data blocks, inser to data chain
	//		else if (next < 0xFF00)	InsertDataBlock(block, link_item->next_block, link_item->valid_page );
	//		//else if // original bad
	//		//else if // new bad
	//		//else // system blocks
	//	}
	//}
	m_link_block->m_group_start += m_link_block->m_page_num;
}

void CPageModeDriver::SaveLinkTable(CLinkBlock * link_block)
{
	JCSIZE block = 0;	// block
	// 考虑一个link page可保存的block数量大于总block数量
	memset(link_block->m_link_page, 0, sizeof(CLinkBlock::LINK_ITEM) * link_block->m_block_per_page);
	JCSIZE bb = 0;		// block:中block个数，bb：一个link page中的offset
	JCSIZE page = 0;
	for (JCSIZE block = 0; block < m_blocks; ++block)
	{
		CLinkBlock::LINK_ITEM * link_item= link_block->m_link_page + bb;
		link_item->erase_count = m_block_chain[block].m_erase_count;
		link_item->valid_page = m_block_chain[block].m_valid_page;
		if (m_block_chain[block].m_valid_page == INVALID_BLOCK_NUM)
		{	// empty block
			link_item->next_block = INVALID_BLOCK_NUM;
		}
		else
		{
			link_item->next_block = m_block_chain[block].m_next_block;
		}

		++bb;
		if (bb >= link_block->m_block_per_page)
		{
			link_block->SavePage(m_file_nand, page);
			memset(link_block->m_link_page, 0, sizeof(CLinkBlock::LINK_ITEM) * link_block->m_block_per_page);
			bb = 0;	page ++;
		}
	}
	if (bb != 0)	link_block->SavePage(m_file_nand, page);

	//for (JCSIZE ii =0; ii < link_block->m_page_num; ++ii)
	//{
	//	memset(link_block->m_link_page, 0, sizeof(CLinkBlock::LINK_ITEM) * link_block->m_block_per_page);
	//	for (JCSIZE bb=0; bb < link_block->m_block_per_page; ++bb, ++block)
	//	{
	//		CLinkBlock::LINK_ITEM * link_item= link_block->m_link_page + bb;
	//		link_item->erase_count = m_block_chain[block].m_erase_count;
	//		link_item->valid_page = m_block_chain[block].m_valid_page;

	//		if (m_block_chain[block].m_valid_page == INVALID_BLOCK_NUM)
	//		{	// empty block
	//			link_item->next_block = INVALID_BLOCK_NUM;
	//		}
	//		else
	//		{
	//			link_item->next_block = m_block_chain[block].m_next_block;
	//		}
	//	}
	//	link_block->SavePage(m_file_nand, ii);
	//}
	link_block->m_group_start += link_block->m_page_num;
}

void CPageModeDriver::InsertEmptyBlock(JCSIZE block)
{
	if (!m_spare_tail)	m_spare_head = m_spare_tail = block;	// chain is empty
	else
	{
		m_block_chain[block].m_valid_page = INVALID_BLOCK_NUM;
		m_block_chain[m_spare_tail].m_next_block = block;
		m_block_chain[block].m_pre_block = m_spare_tail;
		m_spare_tail = block;
	}
}

void CPageModeDriver::InsertDataBlock(JCSIZE block, JCSIZE next, JCSIZE valid_page)
{
	JCASSERT(m_head_block != 0);
	JCASSERT(m_tail_block != 0);

	m_block_chain[block].m_next_block = next;
	m_block_chain[next].m_pre_block = block;
	m_block_chain[block].m_valid_page = valid_page;
}

ULONG32	CPageModeDriver::Read(UCHAR * buf, ULONG64 lba, ULONG32 secs)
{
	LOG_STACK_TRACE();
	LOG_NOTICE(_T("[ATA] read lba=0x%08X, secs=%d"), (UINT)lba, secs);
	JCASSERT(m_file_nand);
	if ( (lba + secs) >= m_config.m_total_sec )		return STATUS_INVALID_PARAMETER;

	stdext::auto_array<UCHAR> _page_buf( SECTOR_TO_BYTE(m_page_size) );
	UCHAR * page_buf = _page_buf;

	JCSIZE remain_secs = secs;
	JCSIZE hpage, offset;
	LBA2HPage(lba, hpage, offset);
	
	while (remain_secs)
	{
		JCSIZE proc_sec = 0;
		ReadHPage(hpage, page_buf);
		if (offset != 0)					proc_sec = min((m_page_size - offset), remain_secs);	// 补头page
		else if (remain_secs < m_page_size)	proc_sec = (remain_secs);
		else								proc_sec = (m_page_size);

		JCSIZE proc_len = SECTOR_TO_BYTE(proc_sec);
		memcpy_s(buf, proc_len, page_buf + SECTOR_TO_BYTE(offset), proc_len );
		buf += proc_len;	remain_secs -= proc_sec;		hpage ++;
		offset = 0;
	}
	return STATUS_SUCCESS;
}

ULONG32	CPageModeDriver::Write(const UCHAR * buf, ULONG64 lba, ULONG32 secs)
{
	// lba to h-page
	LOG_STACK_TRACE();
	LOG_NOTICE(_T("[ATA] write lba=0x%08X, secs=%d"), (UINT)lba, secs);
	if ( (lba + secs) >= m_config.m_total_sec )		return STATUS_INVALID_PARAMETER;

	JCSIZE remain_secs = secs;
	JCSIZE hpage, offset;
	LBA2HPage(lba, hpage, offset);

	JCSIZE prog_secs = min( (m_page_size-offset), remain_secs);

	stdext::auto_array<UCHAR>	_page_buf( SECTOR_TO_BYTE(m_page_size) );
	UCHAR * page_buf = _page_buf;
	while (1)
	{	// 针对每个 page
		if (offset != 0 || prog_secs < m_page_size)
		{	// 补头补尾
			// read data to page buffer
			ReadHPage(hpage, page_buf);
		}
		memcpy_s(page_buf + SECTOR_TO_BYTE(offset), SECTOR_TO_BYTE(prog_secs), buf, SECTOR_TO_BYTE(prog_secs));
		// program data to flash
		WriteHPage(hpage, page_buf);
		// update table
		//if (prog_secs > remain_secs)	break;
		remain_secs -= prog_secs;
		if (remain_secs == 0) break;
		buf += SECTOR_TO_BYTE(prog_secs);
		hpage ++;
		prog_secs = min(m_page_size, remain_secs);
		offset = 0;	// 除了补头的page以外，offset都是0
	}
	return STATUS_SUCCESS;
}

void CPageModeDriver::WriteHPage(JCSIZE hpage, UCHAR * buf)
{
	LOG_STACK_TRACE();
	//LOG_NOTICE(_T("[NAND] write hpage 0x%08X"), hpage);

	// find write posotion
	if (!m_active_0)
	{
		JCSIZE block = PopupEmptyBlock(BLOCKID_DATA);
		LOG_NOTICE(_T("[NAND] popup block 0x%04X"), block);
		// add to data block chain
		if (m_head_block == INVALID_BLOCK_NUM)
		{	// data block chain is empty
			m_head_block = block;
		}
		else
		{
			m_block_chain[m_tail_block].m_next_block = block;
			m_block_chain[block].m_pre_block = m_tail_block;
		}
		m_block_chain[block].m_next_block = 0;
		m_tail_block = block;

		m_active_0 = new CActiveDataBlock(m_serial_number);
		m_active_0->PopUp(block);
		m_serial_number ++;
	}

	JCSIZE page = m_active_0->WriteData(m_file_nand, hpage, buf, m_page_len);
	m_block_chain[m_active_0->m_block_num].m_valid_page ++;
	m_mapping_tab->SetNandAddress(hpage, m_active_0->m_block_num, page);
	if (m_active_0->IsFull() )
	{	//
		LOG_NOTICE(_T("[NAND] save table"));
		// save table (save eob is auto prosseced by active block)
		m_mapping_tab->Flush(m_file_nand, this);
		// update link block
		SaveLinkTable(m_link_block);
		// release active 0
		m_active_0->Close();
		delete m_active_0;
		m_active_0 = NULL;
	}
}

void CPageModeDriver::ReadHPage(JCSIZE hpage, UCHAR * buf)
{
	LOG_STACK_TRACE();

	JCASSERT(m_file_nand);
	// h -> f from current table
	JCSIZE fblock=0, fpage=0;
	m_mapping_tab->GetNandAddress(hpage, fblock, fpage);
	if (fblock == INVALID_BLOCK_NUM)
	{	// LINK不存在，DATA从来没有被写过，回0XFF
		memset(buf, 0xFF, m_page_len);
		return;
	}
	UCHAR spare[SPARE_SIZE];
	LOG_NOTICE(_T("[NAND] read hpage 0x%08X from block:0x%04X, page:0x%04X"), hpage, fblock, fpage);
	NAND_STATUS st = m_file_nand->ReadPage(fblock, fpage, buf, m_page_len, spare);
	JCASSERT(st == NS_OK);
}

JCSIZE CPageModeDriver::PopupEmptyBlock(BYTE block_id)
{
	//JCASSERT(m_data_chain);
	// find a empty block
	if (m_spare_head == 0)	THROW_ERROR(ERR_USER, _T("no empty block"));
	JCSIZE block = m_spare_head;
	// pop up
	m_spare_head = m_block_chain[block].m_next_block;
	if (m_spare_head) m_block_chain[m_spare_head].m_pre_block = 0;
	m_block_chain[block].m_next_block = MAKEWORD(block_id, 0xFF);
	m_block_chain[block].m_valid_page = 0;
	return block;
}

void CPageModeDriver::EraseBlock(JCSIZE block)
{
	m_file_nand->EraseBlock(block);
	m_block_chain[block].m_erase_count ++;
	m_block_chain[block].m_valid_page = INVALID_BLOCK_NUM;
	// if (data block), remove from data block chain
	InsertEmptyBlock(block);
}

ULONG32	CPageModeDriver::FlushCache(void)
{
	m_mapping_tab->Flush(m_file_nand, this);
	SaveLinkTable(m_link_block);
	return STATUS_SUCCESS;
}

ULONG32	CPageModeDriver::DeviceControl(ULONG code, READ_WRITE read_write, UCHAR * buf, ULONG32 & data_size, ULONG32 buf_size)
{
	return STATUS_SUCCESS;
}

ULONG64	CPageModeDriver::GetSize(void) const
{
	return m_config.m_total_sec;
}

void CPageModeDriver::LBA2HPage(ULONG64 lba, JCSIZE & hpage, JCSIZE &offset)
{
	hpage = lba / m_page_size;
	offset = lba % m_page_size;
}

void CPageModeDriver::HpageDecode(JCSIZE hpage, JCSIZE& zone, JCSIZE &group, JCSIZE &page)
{
	LOG_STACK_TRACE();
	JCSIZE _group = hpage / m_page_per_group;
	page = hpage % m_page_per_group;	
	zone = _group / m_group_per_zone;
	group = _group % m_group_per_zone;
	JCASSERT(zone < m_zone_num);
	LOG_DEBUG(_T("%d - %d,%d,%d"), zone, group, page);
}



///////////////////////////////////////////////////////////////////////////////
//--
CGroupTable::CGroupTable(JCSIZE group, JCSIZE page_per_group)
: m_page_per_group(page_per_group), m_dirty(false), m_nand_add(NULL)
, m_group_id(group)
{
	m_nand_add = new DWORD[page_per_group];
	memset(m_nand_add, 0xFF, sizeof(DWORD) * page_per_group);
}

CGroupTable::~CGroupTable(void)
{
	delete [] m_nand_add;
}

void CGroupTable::GetNandAddress(JCSIZE hpage, JCSIZE & fblock, JCSIZE & fpage)
{
	DWORD add = m_nand_add[hpage];
	fblock = HIWORD(add);
	fpage = LOWORD(add);
}

void CGroupTable::SetNandAddress(JCSIZE hpage, JCSIZE fblock, JCSIZE fpage)
{
	DWORD add = MAKELONG(fpage, fblock);
	m_nand_add[hpage] = add;
	m_dirty = true;
}

void CGroupTable::BuildTable(CFileNand * nand, JCSIZE map_block, JCSIZE page)
{
	META_DATA spare;
	JCASSERT(nand);
	JCSIZE page_len = m_page_per_group * sizeof(DWORD);
	NAND_STATUS st = nand->ReadPage(map_block, page, reinterpret_cast<UCHAR*>(m_nand_add), page_len, reinterpret_cast<UCHAR*>(&spare) );
	JCASSERT(st == NS_OK);
	JCASSERT(spare.map.group_id == m_group_id);
	m_dirty = false;
}

void CGroupTable::BuildTable(UCHAR * buf, JCSIZE page)
{
	JCASSERT(m_nand_add);
	JCSIZE page_len = m_page_per_group * sizeof(DWORD);
	memcpy_s(m_nand_add, page_len, buf, page_len);
	m_dirty = false;
}

JCSIZE CGroupTable::Flush(CFileNand * nand, CMapBlock * block)
{
	JCSIZE page = block->SaveGroupTable(nand, m_nand_add, sizeof(DWORD) * m_page_per_group, m_group_id);
	return page;
}


//--
CZoneTable::CZoneTable(JCSIZE group_num, JCSIZE page_per_group, JCSIZE zone_id)
: m_group_tab(NULL)
, m_group_num(group_num), m_page_per_group(page_per_group)
, m_map_block(group_num, zone_id)
, m_updated_group(0)
, m_zone_id(zone_id)
{
	m_group_tab = new CGroupTable*[group_num];
	memset(m_group_tab, 0, sizeof(CGroupTable*) * group_num);
}

CZoneTable::~CZoneTable(void)
{
	for (JCSIZE gg = 0; gg<m_group_num; ++gg)
	{
		if (m_group_tab[gg])	delete m_group_tab[gg];
	}
	delete [] m_group_tab;
}

void CZoneTable::GetNandAddress(JCSIZE group, JCSIZE hpage, JCSIZE & fblock, JCSIZE & fpage)
{
	JCASSERT(group < m_group_num);
	CGroupTable * group_tab = m_group_tab[group];
	if (!group_tab)	return;
	group_tab->GetNandAddress(hpage, fblock, fpage);
}

void CZoneTable::SetNandAddress(JCSIZE group, JCSIZE hpage, JCSIZE fblock, JCSIZE fpage)
{
	JCASSERT(group < m_group_num);
	CGroupTable * group_tab = m_group_tab[group];
	if (!group_tab)
	{
		group_tab = new CGroupTable(group, m_page_per_group);
		m_group_tab[group] = group_tab;
	}
	group_tab->SetNandAddress(hpage, fblock, fpage);
	m_updated_group ++;
}

void CZoneTable::BuildTable(CFileNand * nand, JCSIZE map_block)
{
	m_map_block.Attach(nand, map_block);
	JCSIZE page_len = SECTOR_TO_BYTE(nand->GetFlashInfo()->page_size);

	for (JCSIZE gg = 0; gg < m_group_num; ++gg)
	{
		JCSIZE group_page = m_map_block.GetGroupPage(gg);
		if (group_page != INVALID_BLOCK_NUM)
		{
			if (m_group_tab[gg] == NULL)	m_group_tab[gg] = new CGroupTable(gg, m_page_per_group);
			m_group_tab[gg]->BuildTable(nand, map_block, group_page);
		}
	}
	//	// 重构last index page之后的group table

	META_DATA spare;
	stdext::auto_array<UCHAR> page_buf(page_len);

	while (1)
	{
		memset(&spare, 0, sizeof(META_DATA) );
		JCSIZE page = m_map_block.ReadNextPage(nand, page_buf, page_len, &spare);
		if ( (page != INVALID_BLOCK_NUM) || (spare.id == 0xFF) ) break;
		JCASSERT(spare.id == BLOCKID_GROUP_TAB);
		//JCASSERT(spare.map.zone_id == );
		JCSIZE gg = spare.map.group_id;
		if (m_group_tab[gg] == NULL)	m_group_tab[gg] = new CGroupTable(gg, m_page_per_group);
		m_group_tab[gg]->BuildTable(page_buf);
		m_map_block.SetGroupPage(gg, page);
	}
	// update zone table
}

JCSIZE	CZoneTable::Flush(CFileNand * nand, CPageModeDriver * driver)
{
	JCSIZE new_block = INVALID_BLOCK_NUM;
	JCSIZE cur_block = INVALID_BLOCK_NUM;
	bool force_flush = false;
	if ( m_map_block.IsFull() && (m_updated_group > 0) )
	{	// 如果map block已满，并且有group被更新过
		//		则pop一个新的block，并且设置所有的group为dirty
		cur_block = m_map_block.Close();
		new_block = driver->PopupEmptyBlock(BLOCKID_GROUP_TAB);
		m_map_block.PopUp(new_block);
		force_flush = true;
	}

	for (JCSIZE gg = 0; gg < m_group_num; ++gg)
	{
		CGroupTable * group_tab = m_group_tab[gg];
		if ( (group_tab) && (group_tab->m_dirty || force_flush) )
		{
			JCSIZE page = group_tab->Flush(nand, &m_map_block);
			if (page == INVALID_BLOCK_NUM)	break; // block 已满，不再继续更新
			else m_map_block.SetGroupPage(gg, page);
		}
	}

	if (m_map_block.m_updated > 0)
	{
		m_map_block.SaveZoneTable(nand);
	}
	if (cur_block != INVALID_BLOCK_NUM)	driver->EraseBlock(cur_block);
	return new_block;
}


//--
CMappingTable::CMappingTable(JCSIZE zone_num, JCSIZE group_per_zone, JCSIZE page_per_group)
: m_zone_tab(NULL)
, m_zone_num(zone_num), m_group_per_zone(group_per_zone), m_page_per_group(page_per_group)
//, m_dirty(false)
{
	JCASSERT(m_zone_num > 0);
	m_zone_tab = new CZoneTable*[m_zone_num];
	memset(m_zone_tab, 0, sizeof(CZoneTable*) * m_zone_num);
}

CMappingTable::~CMappingTable(void)
{
	for (JCSIZE zz = 0; zz < m_zone_num; ++zz)
	{
		if (m_zone_tab[zz]) delete m_zone_tab[zz];
	}
	delete [] m_zone_tab;
}

void CMappingTable::GetNandAddress(JCSIZE hpage, JCSIZE & fblock, JCSIZE & fpage)
{
	JCASSERT(m_zone_tab);
	JCSIZE zone =0, group =0, page =0;
	HpageDecode(hpage, zone, group, page);

	fblock = INVALID_BLOCK_NUM, fpage = INVALID_BLOCK_NUM;
	CZoneTable * zone_tab = m_zone_tab[zone];
	if (!zone_tab) return;
	zone_tab->GetNandAddress(group, page, fblock, fpage);
}

void CMappingTable::SetNandAddress(JCSIZE hpage, JCSIZE fblock, JCSIZE fpage)
{
	JCASSERT(m_zone_tab);
	JCSIZE zone =0, group =0, page =0;
	HpageDecode(hpage, zone, group, page);

	CZoneTable * zone_tab = m_zone_tab[zone];
	if (!zone_tab)
	{
		zone_tab = new CZoneTable(m_group_per_zone, m_page_per_group, zone);
		m_zone_tab[zone] = zone_tab;
	}
	
	zone_tab->SetNandAddress(group, page, fblock, fpage);
	//m_dirty = true;
}

void CMappingTable::HpageDecode(JCSIZE hpage, JCSIZE& zone, JCSIZE &group, JCSIZE &page)
{
	LOG_STACK_TRACE();
	JCSIZE _group = hpage / m_page_per_group;
	page = hpage % m_page_per_group;	
	zone = _group / m_group_per_zone;
	group = _group % m_group_per_zone;
	LOG_DEBUG_(+1, _T("%d - %d,%d,%d"), hpage, zone, group, page);
	JCASSERT(page < m_page_per_group);	JCASSERT(group < m_group_per_zone);		JCASSERT(zone < m_zone_num);
}

void CMappingTable::BuildTable(CFileNand * nand, JCSIZE zone_index)
{
	JCASSERT(m_zone_block.m_block_num == INVALID_BLOCK_NUM);
	m_zone_block.Attach(nand, zone_index);

	// load each zone table
	for (JCSIZE zz = 0; zz < m_zone_num; ++zz)
	{
		JCSIZE map_block = m_zone_block.GetMapBlock(zz);
		if (map_block != INVALID_BLOCK_NUM)
		{
			if (NULL == m_zone_tab[zz])		m_zone_tab[zz] = new CZoneTable(m_group_per_zone, m_page_per_group, zz);
			CZoneTable * zone_tab = m_zone_tab[zz];
			zone_tab->BuildTable(nand, map_block);
		}
	}
}

JCSIZE CMappingTable::Flush(CFileNand * nand, CPageModeDriver * driver)
{
	for (JCSIZE zz = 0; zz < m_zone_num; ++zz)
	{
		CZoneTable * zone_tab = m_zone_tab[zz];
		if (zone_tab)
		{
			JCSIZE block = zone_tab->Flush(nand, driver);
			if (block == 0) return 0;
			if (block != INVALID_BLOCK_NUM)		m_zone_block.SetMapBlock(zz, block);
		}
	}

	JCSIZE new_block = INVALID_BLOCK_NUM;
	JCSIZE cur_block = INVALID_BLOCK_NUM;
	if (m_zone_block.m_updated > 0) 
	{ // update zone index block
		if ( m_zone_block.IsFull() )
		{
			cur_block = m_zone_block.Close();
			new_block = driver->PopupEmptyBlock(BLOCKID_ZONE_INDEX);
			m_zone_block.PopUp(new_block);
		}
		m_zone_block.Save(nand);
		if (cur_block != INVALID_BLOCK_NUM) driver->EraseBlock(cur_block);
	}
	return new_block;

}