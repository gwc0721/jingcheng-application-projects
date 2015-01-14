#include "../include/binary_buffer.h"

#include "stdafx.h"


LOCAL_LOGGER_ENABLE(_T("binbuf"), LOGGER_LEVEL_DEBUGINFO);

///////////////////////////////////////////////////////////////////////////////
//----  
void CBinaryBufferBase::ToStream(jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt, DWORD param) const
{
}

void CBinaryBufferBase::OutputText(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE len) const
{
}

void CBinaryBufferBase::OutputBinary(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE len) const
{
}


///////////////////////////////////////////////////////////////////////////////
//----  
LOG_CLASS_SIZE(CFileMappingBuf)

void CreateFileMappingBuf(CFileMapping * mapping, JCSIZE offset_sec, JCSIZE secs, jcparam::IBinaryBuffer * & buf)
{
	JCASSERT(buf == NULL);
	buf = new CFileMappingBuf(mapping, offset_sec, secs);
}

CFileMappingBuf::CFileMappingBuf(CFileMapping * mapping, JCSIZE offset_sec, JCSIZE secs)
	:m_ref(1), m_mapping(mapping), m_locked(0), m_ptr(NULL)
	
{
	if (!m_mapping) THROW_ERROR(ERR_PARAMETER, _T("mapping cannot be NULL"));
	m_mapping->AddRef();
	FILESIZE start = SECTOR_TO_BYTEL(offset_sec);
	JCSIZE data_len = SECTOR_TO_BYTE(secs);

	m_aligned_start = m_mapping->Aligne(start);		JCASSERT(m_aligned_start <= start);
	m_offset = (JCSIZE)(start - m_aligned_start);
	m_aligned_len = m_mapping->Aligne(data_len);

}

CFileMappingBuf::~CFileMappingBuf(void)
{
	JCASSERT(m_mapping);
	if (m_locked > 0)
	{
#ifdef _DEBUG
		THROW_ERROR(ERR_APP, _T("there are pointers have not been unlcked"));
#endif
		m_mapping->Unmapping(m_ptr);
	}
	m_mapping->Release();
}

BYTE * CFileMappingBuf::Lock(void)
{
	// 指针对齐
	if (!m_ptr)		m_ptr = (BYTE*)(m_mapping->Mapping(m_aligned_start, m_aligned_len));
	InterlockedIncrement(&m_locked);
	return m_ptr + m_offset;
}

void CFileMappingBuf::Unlock(BYTE * ptr)
{
	JCASSERT(ptr == m_ptr + m_offset);
	if (InterlockedDecrement(&m_locked) == 0)
	{
		m_mapping->Unmapping(m_ptr);
		m_ptr = NULL;
	}
}


