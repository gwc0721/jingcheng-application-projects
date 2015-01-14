#include "stdafx.h"
#include <stdext.h>

#include "page_mode_driver.h"

#include <WinIoCtl.h>

LOCAL_LOGGER_ENABLE(_T("file_nand"), LOGGER_LEVEL_DEBUGINFO);

// 定义从文件的FLASH_INFO_OFFSET开始，记录一些和NAND有关的信息。
// 例如：Page大小，块的大小，每个块中写入的Page数，Erase Count等，用于验证F/W的正确性
// 这段数据通常占用Block 0的空间。实际应用中，Block 0不会全部使用，根据实际情况，调整
// FLASH_INOF的位置
#define FLASH_INFO_OFFSET	0

// 最大支持BLOCK数量
#define MAX_BLOCKS			16384
// 固定FLASH_INFO大小，每个BLOCK 6字节，其余作PARAMETER
#define FLASH_INFO_SIZE		(MAX_BLOCKS * 8)
#define BAD_BLOCK_OFFSET	(MAX_BLOCKS * 1)
#define LAST_PAGE_OFFSET	(MAX_BLOCKS * 2)
#define ERASE_COUNT_OFFSET	(MAX_BLOCKS * 4)

///////////////////////////////////////////////////////////////////////////////
//-- file nand flash simulator


CFileNand::CFileNand(const CJCStringT & file_name)
: m_nand_file(NULL), m_nand_mapping(NULL)
, m_info_ready(false)
, m_flash_info(NULL), m_erase_count_tab(NULL), m_last_page_tab(NULL)
, m_mapping_index(NULL)
, m_cache_swap_out(0)
{
	// open file
	m_nand_file = ::CreateFileW(file_name.c_str(), FILE_ALL_ACCESS, 0, 
		NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL );
	if(INVALID_HANDLE_VALUE == m_nand_file)
	{
		m_nand_file = NULL;
		THROW_WIN32_ERROR(_T("cannot open file %s"), file_name.c_str() );
	}

	m_info_ready = ReadFlashInfo();

	memset(m_mapping_tab, 0, sizeof(NAND_FILE_MAPPING) * MAX_CACHE_BLOCKS);
}

CFileNand::~CFileNand(void)
{
	if (m_flash_info)	UnmapViewOfFile(m_flash_info);
	for (int ii=0; ii<MAX_CACHE_BLOCKS; ++ii)
	{
		LPVOID cc = m_mapping_tab[ii].buf;
		if (cc)	UnmapViewOfFile(cc);
	}
	if (m_nand_mapping)	CloseHandle(m_nand_mapping);
	if (m_nand_file)	CloseHandle(m_nand_file);

	delete [] m_mapping_index;
}

void CFileNand::Pretest(JCSIZE page_size, JCSIZE page_per_block, JCSIZE block_num, JCSIZE spare_len)
{
	// 取得系统对齐大小 
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	DWORD granularity = si.dwAllocationGranularity;
	JCSIZE mask = ~(granularity-1);
	LOG_DEBUG(_T("Allocation Granularity = 0x%08X, mask = 0x%08X"), granularity, mask);

	// 计算FLASH_INFO大小：每个Block有一个2字节Last Page表示最后写入的Page，同时表示该Block是否Erase
	//  每Block有一个4字节的Erase Count
	JCASSERT(m_nand_file);
	JCASSERT(false == m_info_ready);
	// 计算spare page数量
	JCSIZE spare_in_block = spare_len * page_per_block;
	JCSIZE spare_pages = (spare_in_block-1) / SECTOR_TO_BYTE(page_size) +1;
	// 计算块大小
	JCSIZE block_len = (page_per_block + spare_pages) * SECTOR_TO_BYTE(page_size);
	// block大小对齐系统颗粒度，(假设granularity是2的整次幂）
	block_len = (block_len -1);
	block_len = (block_len & mask) + granularity;
	// 计算文件大小
	FILESIZE file_size = block_len * block_num;
	LOG_DEBUG(_T("allined block size = 0x%08X"), block_len);

	USHORT comp = COMPRESSION_FORMAT_LZNT1;
	DWORD dr = 0;
	//BOOL br = DeviceIoControl(m_nand_file, FSCTL_SET_COMPRESSION,
	//	&comp, sizeof(comp), NULL, 0, &dr, NULL);
	//if (!br) THROW_WIN32_ERROR(_T("failure on setting file compression"));

	m_nand_mapping = CreateFileMapping(m_nand_file, 
		NULL, PAGE_READWRITE, HIDWORD(file_size), LODWORD(file_size), NULL);
	if (!m_nand_mapping) THROW_WIN32_ERROR(_T("failure on creating flash mapping."));
	UCHAR * dummy = (UCHAR*) MapViewOfFile(m_nand_mapping, 
		FILE_MAP_ALL_ACCESS, 0, FLASH_INFO_OFFSET, FLASH_INFO_SIZE);
	if (!dummy) THROW_WIN32_ERROR(_T("failure on mapping flash info."));

	memset(dummy, 0, FLASH_INFO_SIZE);
	m_flash_info = reinterpret_cast<FLASH_INFO*>(dummy);
	m_flash_info->page_size = page_size;
	m_flash_info->page_per_block = page_per_block;
	m_flash_info->blocks = block_num;
	m_flash_info->spare_len = spare_len;
	m_flash_info->spare_pages = spare_pages;
	m_flash_info->block_len = block_len;

	FlushViewOfFile(m_flash_info, FLASH_INFO_SIZE);
	m_info_ready = true;

	Initialize();
}

bool CFileNand::ReadFlashInfo(void)
{	// 利用Block 0 保存nand info (card mode)
	JCASSERT(m_nand_file);
	LARGE_INTEGER file_size = {0,0};
    if (!::GetFileSizeEx(m_nand_file, &file_size))	THROW_WIN32_ERROR(_T("failure on getting file size"));
	if (file_size.QuadPart < FLASH_INFO_SIZE) return false;
	
	JCASSERT(NULL == m_nand_mapping);
	m_nand_mapping = CreateFileMapping(m_nand_file, 
		NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!m_nand_mapping) THROW_WIN32_ERROR(_T("failure on creating flash mapping."));
	UCHAR * dummy = (UCHAR*) MapViewOfFile(m_nand_mapping, 
		FILE_MAP_ALL_ACCESS, 0, FLASH_INFO_OFFSET, FLASH_INFO_SIZE);
	if (!dummy) THROW_WIN32_ERROR(_T("failure on mapping flash info."));

	m_flash_info = reinterpret_cast<FLASH_INFO*>(dummy);

	Initialize();

	return true;
}

void CFileNand::Initialize(void)
{
	JCASSERT(m_flash_info);
	m_page_len = SECTOR_TO_BYTE(m_flash_info->page_size);
	m_block_len = m_flash_info->block_len;
	m_spare_start = SECTOR_TO_BYTE(m_flash_info->page_per_block * m_flash_info->page_size);
	m_spare_len = m_flash_info->spare_len;

	UCHAR * dummy = reinterpret_cast<UCHAR*>(m_flash_info);
	m_org_bad = reinterpret_cast<BYTE*>(dummy + BAD_BLOCK_OFFSET);
	m_last_page_tab = reinterpret_cast<WORD *>(dummy + LAST_PAGE_OFFSET);
	m_erase_count_tab = reinterpret_cast<ULONG32*>(dummy + ERASE_COUNT_OFFSET);

	m_mapping_index = new BYTE[m_flash_info->blocks];
	memset(m_mapping_index, 0xFF, m_flash_info->blocks);
}

BYTE CFileNand::MapNandBlock(JCSIZE block)
{
	// 寻找空的cache
	BYTE ii = 0;
	for (; ii < MAX_CACHE_BLOCKS; ++ii)	if (m_mapping_tab[ii].buf == 0) break;
	if (ii >= MAX_CACHE_BLOCKS)
	{	// 淘汰一个block
		ii = m_cache_swap_out;
		JCASSERT(m_mapping_tab[ii].buf);
		UnmapViewOfFile(m_mapping_tab[ii].buf);
		m_mapping_index[m_mapping_tab[ii].block_id] = 0xFF;
		m_mapping_tab[ii].buf = NULL;

		m_cache_swap_out ++;
		if (m_cache_swap_out >= MAX_CACHE_BLOCKS)	m_cache_swap_out = 0;
	}

	// 计算block在文件中的位置
	FILESIZE offset = (FILESIZE)(block) * m_block_len;
	UCHAR * _buf = (UCHAR*)MapViewOfFile(m_nand_mapping, FILE_MAP_ALL_ACCESS, 
		HIDWORD(offset), LODWORD(offset), m_block_len);
	if (!_buf) THROW_WIN32_ERROR(_T("failure on mapping file, block=%d, offset=%I64d"), block, offset);
	m_mapping_tab[ii].buf = _buf;
	m_mapping_tab[ii].block_id = block;
	m_mapping_index[block] = ii;
	return ii;
}

NAND_STATUS CFileNand::ProgramPage(JCSIZE block, JCSIZE page, LPVOID data, JCSIZE data_len, LPVOID spare)
{
	if (block >= m_flash_info->blocks || page >= m_flash_info->page_per_block)	return NS_WRONG_ADD;
	if (page < m_last_page_tab[block])	return NS_OVER_PROGRAM;
	JCASSERT(data_len <= m_page_len);

	//<TODO> 实现over program造成的data破坏
	BYTE mapping_index = m_mapping_index[block];
	if (0xFF == mapping_index) mapping_index = MapNandBlock(block);
	// program data
	UCHAR * page_buf = m_mapping_tab[mapping_index].buf + page * m_page_len;
	memcpy_s(page_buf, data_len, data, data_len);
	// program spare
	UCHAR * spare_buf = m_mapping_tab[mapping_index].buf + m_spare_start + (m_spare_len * page);
	memcpy_s(spare_buf, m_spare_len, spare, m_spare_len);
	// update last page
	m_last_page_tab[block] = page +1;
	return NS_OK;
}

NAND_STATUS CFileNand::ReadPage(JCSIZE block, JCSIZE page, LPVOID data, JCSIZE data_len, LPVOID spare)
{
	if (block >= m_flash_info->blocks || page >= m_flash_info->page_per_block)	return NS_WRONG_ADD;
	JCASSERT(data_len <= m_page_len);
	//if (data_len > m_page_len)	m_data_len = m_page_len;
	if (page >= m_last_page_tab[block])
	{	// 未写入page
		memset(data, 0xFF, data_len);
		memset(spare, 0xFF, m_spare_len);
		return NS_EMPTY_PAGE;
	}

	BYTE mapping_index = m_mapping_index[block];
	if (0xFF == mapping_index) mapping_index = MapNandBlock(block);
	// read data
	if (data && data_len)
	{
		UCHAR * page_buf = m_mapping_tab[mapping_index].buf + page * m_page_len;
		memcpy_s(data, data_len, page_buf, data_len);
	}
	// read spare
	UCHAR * spare_buf = m_mapping_tab[mapping_index].buf + m_spare_start + (m_spare_len * page);
	memcpy_s(spare, m_spare_len, spare_buf, m_spare_len);
	return NS_OK;
}

NAND_STATUS CFileNand::EraseBlock(JCSIZE block)
{
	if (block >= m_flash_info->blocks)	return NS_WRONG_ADD;
	BYTE mapping_index = m_mapping_index[block];
	//<TODO>优化：erase的block不需要mapping
	if (0xFF == mapping_index) mapping_index = MapNandBlock(block);
	memset(m_mapping_tab[mapping_index].buf, 0xFF, m_block_len);
	m_last_page_tab[block] = 0;
	m_erase_count_tab[block] ++;

	return NS_OK;
}


