#include "stdafx.h"

#include "../include/smi_data_type.h"
#include "../include/ismi_device.h"

LOCAL_LOGGER_ENABLE(_T("sector_buf"), LOGGER_LEVEL_DEBUGINFO);

LOG_CLASS_SIZE(CSectorBuf)

bool CreateSectorBuf(FILESIZE sec_add, CSectorBuf * & buf)
{
	JCASSERT(NULL == buf);
	buf = new CSectorBuf(sec_add);
	return true;
}

CSectorBuf::CSectorBuf(FILESIZE sec_add)
	:m_sec_add(sec_add)
{
}

CSectorBuf::~CSectorBuf(void)
{
}

const JCSIZE ADD_DIGS = 6; 
const JCSIZE HEX_OFFSET = ADD_DIGS + 3;
const JCSIZE ASCII_OFFSET = HEX_OFFSET + 51;

void CSectorBuf::ToStream(jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt, DWORD param) const
{
	JCSIZE offset = 0, len = 0;
	if ( fmt & jcparam::VF_PARTIAL )	len = HIWORD(param), offset = LOWORD(param);
	switch ( fmt & jcparam::VF_FORMAT_MASK )
	{
	case jcparam::VF_TEXT:
		OutputText(stream, offset, len);
		break;
	case jcparam::VF_BINARY:
	case jcparam::VF_DEFAULT:
		OutputBinary(stream, offset, len);
		break;
	}
}

void CSectorBuf::OutputBinary(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE out_len) const
{
	JCASSERT(stream);
	JCASSERT(m_array);
	Lock();
	JCSIZE len = SECTOR_SIZE;

	stream->Put((const wchar_t*)(m_array), len /2 );
	Unlock();
}

void CSectorBuf::OutputText(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE out_len) const
{
	JCASSERT(stream);
	JCASSERT(m_array);
	TCHAR	str_buf[STR_BUF_LEN];
	str_buf[STR_BUF_LEN - 3] = _T('\n');
	str_buf[STR_BUF_LEN - 2] = 0;
	str_buf[STR_BUF_LEN - 1] = 0;

	JCSIZE ii = 0;

	// output head line
	wmemset(str_buf, _T(' '), STR_BUF_LEN - 3);
	// sector address
	_local_itohex(str_buf, ADD_DIGS, (UINT)m_sec_add);
	for (ii=0; ii < 16; ++ii)
	{
		LPTSTR _str = str_buf + (ii * 3) + HEX_OFFSET-1;
		_str[0] = _T('-');
		_str[1] = _T('-');
		_str[2] = stdext::hex2char(ii);
	}
	
	stream->Put(str_buf, STR_BUF_LEN-2);

	// output data
	Lock();

	// 仅支持行对齐
	JCSIZE add = (m_sec_add * SECTOR_SIZE) & 0xFFFFFF;

	ii = 0;
	JCSIZE len = SECTOR_SIZE;

	while (ii < len)
	{	// loop for line
		wmemset(str_buf, _T(' '), STR_BUF_LEN - 3);
		wmemset(str_buf + ASCII_OFFSET, _T('.'), 16);

		// output address
		_local_itohex(str_buf, ADD_DIGS, add);
		LPTSTR  str1 = str_buf + HEX_OFFSET;
		LPTSTR	str2 = str_buf + ASCII_OFFSET;

		for (int cc = 0; (cc<0x10) && (ii<len) ; ++ii, ++cc)
		{
			BYTE dd = m_array[ii];
			_local_itohex(str1, 2, dd);	str1 += 3;
			if ( (0x20 <= dd) && (dd < 0x7F) )	str2[0] = dd;
			str2++;
		}
		add += 16;
		stream->Put(str_buf, STR_BUF_LEN-2);
	}
	stream->Put(_T('\n'));
	Unlock();
}


void CSectorBuf::GetSubValue(LPCTSTR name, jcparam::IValue * & val)
{
}

void CSectorBuf::SetSubValue(LPCTSTR name, jcparam::IValue * val)
{
}

