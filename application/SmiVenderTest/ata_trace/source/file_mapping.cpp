#include "../include/file_mapping.h"
#include "stdafx.h"

LOCAL_LOGGER_ENABLE(_T("file_mapping"), LOGGER_LEVEL_WARNING);

///////////////////////////////////////////////////////////////////////////////
//----  file mapping
LOG_CLASS_SIZE(CFileMapping)

UINT64 CFileMapping::m_granularity = 0;
UINT64 CFileMapping::m_grn_mask = 0;

void CreateFileMappingObject(const CJCStringT & fn, CFileMapping * & mapping)
{
	JCASSERT(mapping == NULL);
	mapping = new CFileMapping(fn);
}
void CreateFileMappingObject(HANDLE file, FILESIZE set_size, CFileMapping * & mapping)
{
	JCASSERT(mapping == NULL);
	mapping = new CFileMapping(file, set_size);
}

CFileMapping::CFileMapping(HANDLE file, FILESIZE set_size)
	: m_src_file(file)
	, m_mapping(NULL)
	, m_ref(1)
{
	JCASSERT(m_src_file);
	JCASSERT(m_src_file != INVALID_HANDLE_VALUE);

	// Get file size
	LARGE_INTEGER file_size = {0,0};
    if (!::GetFileSizeEx(m_src_file, &file_size))	THROW_WIN32_ERROR(_T("failure on getting file size"));
	m_file_size = file_size.QuadPart;

	m_mapping = CreateFileMapping(m_src_file, 
		NULL, PAGE_READWRITE, HIDWORD(set_size), LODWORD(set_size), NULL);
	DWORD err = GetLastError();
	if (!m_mapping) THROW_WIN32_ERROR_(err, _T("failure on creating flash mapping."));
}

CFileMapping::CFileMapping(const CJCStringT & fn)
	: m_src_file(NULL)
	, m_mapping(NULL)
	, m_ref(1)
{
	// 获取系统对齐信息
	if (m_granularity == 0)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		m_granularity = si.dwAllocationGranularity;
		m_grn_mask = ~(m_granularity-1);
		LOG_DEBUG(_T("Allocation Granularity = 0x%08X, mask = 0x%08X"), m_granularity, m_grn_mask);
	}

	m_src_file = ::CreateFileW(fn.c_str(), FILE_ALL_ACCESS, 0, 
		NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, NULL );
	if(INVALID_HANDLE_VALUE == m_src_file)
	{
		m_src_file = NULL;
		THROW_WIN32_ERROR(_T("failure on opening data file %s"), fn.c_str() );
	}

	// Get file size
	LARGE_INTEGER file_size = {0,0};
    if (!::GetFileSizeEx(m_src_file, &file_size))	THROW_WIN32_ERROR(_T("failure on getting file size"));
	m_file_size = file_size.QuadPart;

	// 
	m_mapping = CreateFileMapping(m_src_file, 
		NULL, PAGE_READWRITE, 0, 0, NULL);
	if (!m_mapping) THROW_WIN32_ERROR(_T("failure on creating flash mapping."));
}

CFileMapping::~CFileMapping(void)
{
	if (m_mapping) CloseHandle(m_mapping);
	if (m_src_file) CloseHandle(m_src_file);
}

void CFileMapping::SetSize(FILESIZE size)
{
	if (m_mapping) CloseHandle(m_mapping);
	m_mapping = CreateFileMapping(m_src_file, NULL, PAGE_READWRITE, HIDWORD(size), LODWORD(size), NULL);
	if (!m_mapping) THROW_WIN32_ERROR(_T("failure on creating flash mapping."));
}

LPVOID CFileMapping::Mapping(FILESIZE offset, JCSIZE len)
{
	// 指针对齐
	LOG_STACK_TRACE();
	JCASSERT(m_mapping);

	LPVOID ptr = MapViewOfFile(m_mapping, FILE_MAP_ALL_ACCESS, 	
		HIDWORD(offset), LODWORD(offset), len);
	return ptr;

/*
	FILESIZE aligned_start = ((offset -1) & m_grn_mask) + m_granularity;
	JCSIZE aligned_len = (JCSIZE)(((len -1) & m_grn_mask) + m_granularity);
	LOG_DEBUG(_T("request start:0x%08I64X, len:0x%08X"), offset, len);
	LOG_DEBUG(_T("aligned start:0x%08I64X, len:0x%08X"), aligned_start, aligned_len);

	// 检查是否已经mapping，并且长度大于请求长度
	VIEW_NODE * view = FindView(aligned_start, aligned_len);
	BYTE * ptr = NULL;
	if (view)
	{
		ptr = view->ptr;
		InterlockedIncrement(&(view->ref_cnt));
	}
	else
	{
		ptr = (BYTE*) MapViewOfFile(m_mapping, FILE_MAP_ALL_ACCESS, 
			HIDWORD(aligned_start), LODWORD(aligned_start), aligned_len);
		if (!ptr) THROW_WIN32_ERROR(_T("failure on mapping flash info."));
		InsertView(ptr, aligned_start, aligned_len);
	}

	locked = len;
	// 调整指针
	return (ptr + (offset - aligned_start));
*/
}

void CFileMapping::Unmapping(LPVOID ptr)
{
	UnmapViewOfFile(ptr);
/*
	// 查找node
	VIEW_LIST::iterator it = m_view_list.begin();
	VIEW_LIST::iterator endit = m_view_list.end();
	for (; it!= endit; ++it)
	{
		VIEW_NODE & view = *it;
		if ( (ptr >= view.ptr) && ( (JCSIZE)(ptr - view.ptr) < view.length) )
		{
			if (InterlockedDecrement(&view.ref_cnt) == 0)
			{
				UnmapViewOfFile(view.ptr);
				m_view_list.erase(it);
			}
			break;
		}
	}
*/
}

void CFileMapping::InsertView(BYTE * ptr, FILESIZE start, JCSIZE len)
{
	m_view_list.push_back(VIEW_NODE(ptr, start, len));
}

CFileMapping::VIEW_NODE * CFileMapping::FindView(FILESIZE start, JCSIZE len)
{
	VIEW_LIST::iterator it = m_view_list.begin();
	VIEW_LIST::iterator endit = m_view_list.end();

	for (; it != endit; ++it)
	{
		if ( (it->start == start) &&  (it->length > len) ) 
			return &(*it);
	}
	return NULL;
}
