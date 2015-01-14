
//#include "../include/smi_data_type.h"
//#include "../include/ismi_device.h"
#include "../include/binary_buffer.h"
#include "stdafx.h"

LOCAL_LOGGER_ENABLE(_T("binary_buf"), LOGGER_LEVEL_DEBUGINFO);

//LOG_CLASS_SIZE(CBinaryBuffer)

CBinaryBuffer::CBinaryBuffer(JCSIZE count)
	: m_array(NULL)
	, m_address(NULL)
	, m_size(0)
{
	m_array = new BYTE[count];
	m_size = count;
}

CBinaryBuffer::~CBinaryBuffer(void)
{
	delete [] m_array;
	if (m_address)	m_address->Release();
}

void _local_itohex(LPTSTR str, JCSIZE dig, UINT d)
{
	JCSIZE ii = dig;
	do
	{
		str[--ii] = stdext::hex2char(d & 0x0F);
		d >>= 4;
	}
	while (ii > 0);
}

const TCHAR __SPACE[128] = {
	_T("        ")_T("        ")_T("        ")_T("        ")
	_T("        ")_T("        ")_T("        ")_T("        ")
	_T("        ")_T("        ")_T("        ")_T("        ")
	_T("        ")_T("        ")_T("        ")_T("       ")
};

const TCHAR * SPACE = __SPACE + 127;


//void CBinaryBuffer::Format(FILE * file, LPCTSTR format)
//{
//	if ( NULL == format || FastCmpT(EMPTY, format) )	format = _T("text");
//	if ( FastCmpT(_T("bin"), format) )
//	{
//		JCSIZE len = GetSize();
//		BYTE * data = Lock();
//		JCASSERT(data);
//		fwrite (data, len, 1, file);
//		Unlock();
//	}
//	else if ( FastCmpT(_T("text"), format) )
//	{
//		//
//		JCSIZE offset = 0;
//		if ( m_address && m_address->GetType() == IAddress::ADD_MEMORY)
//		{
//			offset = (JCSIZE)(m_address->GetValue() );
//		}
//		BYTE column_offset = (BYTE)(offset & 0xF);
//		JCSIZE add_offset = offset & 0xFFFFFFF0;
//
//		_ftprintf(file, SPACE - 5);
//		JCSIZE ii = 0;
//		for (ii=0; ii < 16; ++ii)	_ftprintf(file, _T("--%01X"), ii);
//		JCSIZE len = GetSize();
//		BYTE* data = Lock();
//		JCASSERT(data);
//		stdext::jc_fprintf(file, _T("\n") );
//
//		ii = 0;		// total count
//		JCSIZE  add = offset;
//		TCHAR	str_buf[STR_BUF_LEN];
//
//		str_buf[STR_BUF_LEN - 3] = _T('\r');
//		str_buf[STR_BUF_LEN - 2] = _T('\n');
//		str_buf[STR_BUF_LEN - 1] = 0;
//
//
//
//		if ( column_offset != 0)
//		{		// Write first line
//			wmemset(str_buf, _T(' '), STR_BUF_LEN - 3);
//			_local_itohex(str_buf, 4, (offset & 0xFFF0));
//			LPTSTR  str1 = str_buf + HEX_OFFSET + column_offset * 3;
//			LPTSTR	str2 = str_buf + ASCII_OFFSET + column_offset;
//
//			for (int cc = column_offset; (cc<0x10) && (ii<len) ; ++ii, ++add, ++cc)
//			{
//				BYTE dd = data[ii];
//				_local_itohex(str1, 2, dd);	str1 += 3;
//				if ( (0x20 <= dd) && (dd < 0x7F) )	str2[0] = dd;
//				else								str2[0] = _T('.');
//				str2++;
//			}
//			stdext::jc_fprintf(file, str_buf);
//		}
//
//		while (ii < len)
//		{	// loop for line
//			wmemset(str_buf, _T(' '), STR_BUF_LEN - 3);
//			wmemset(str_buf + ASCII_OFFSET, _T('.'), 16);
//			_local_itohex(str_buf, 4, add);
//			LPTSTR  str1 = str_buf + HEX_OFFSET;
//			LPTSTR	str2 = str_buf + ASCII_OFFSET;
//
//			for (int cc = 0; (cc<0x10) && (ii<len) ; ++ii, ++add, ++cc)
//			{
//				BYTE dd = data[ii];
//				_local_itohex(str1, 2, dd);	str1 += 3;
//				if ( (0x20 <= dd) && (dd < 0x7F) )	str2[0] = dd;
//				str2++;
//			}
//			stdext::jc_fprintf(file, _T("%s"), str_buf);
//		}
//		Unlock();
//		_ftprintf(file, _T("\n"));
//	}
//}

void CBinaryBuffer::ToStream(jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt, DWORD param) const
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

void CBinaryBuffer::OutputBinary(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE out_len) const
{
	JCASSERT(stream);
	JCASSERT(m_array);
	Lock();
	JCSIZE len = GetSize();

	stream->Put((const wchar_t*)(m_array), len /2 );
	Unlock();
}

void CBinaryBuffer::OutputText(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE out_len) const
{
	JCASSERT(stream);
	JCASSERT(m_array);

	// output head line
	stream->Put(SPACE - HEX_OFFSET + 1, HEX_OFFSET - 1);
	JCSIZE ii = 0;
	for (ii=0; ii < 16; ++ii)	stream->Format(_T("--%01X"), ii);
	stream->Put(_T('\n'));

	// output data
	Lock();

	TCHAR	str_buf[STR_BUF_LEN];
	str_buf[STR_BUF_LEN - 3] = _T('\n');
	str_buf[STR_BUF_LEN - 2] = 0;
	str_buf[STR_BUF_LEN - 1] = 0;

	// 仅支持行对齐
	JCSIZE add_offset = offset & 0xFFFFFFF0;
	JCSIZE add = add_offset;

	ii = 0;
	JCSIZE len = min (GetSize() - add_offset, out_len);

	while (ii < len)
	{	// loop for line
		wmemset(str_buf, _T(' '), STR_BUF_LEN - 3);
		wmemset(str_buf + ASCII_OFFSET, _T('.'), 16);

		// output address
		_local_itohex(str_buf, 4, add);
		LPTSTR  str1 = str_buf + HEX_OFFSET;
		LPTSTR	str2 = str_buf + ASCII_OFFSET;

		for (int cc = 0; (cc<0x10) && (ii<len) ; ++ii, ++add, ++cc)
		{
			BYTE dd = m_array[ii];
			_local_itohex(str1, 2, dd);	str1 += 3;
			if ( (0x20 <= dd) && (dd < 0x7F) )	str2[0] = dd;
			str2++;
		}
		stream->Put(str_buf, STR_BUF_LEN-2);
	}
	stream->Put(_T('\n'));

	Unlock();
}


void CBinaryBuffer::GetSubValue(LPCTSTR name, jcparam::IValue * & val)
{
	JCASSERT(NULL == val);

	if ( FastCmpT(_T("address"), name) )
	{
		IAddress * add = NULL;
		GetAddress(add);
		val = static_cast<jcparam::IValue*>(add);
	}
	else if ( FastCmpT(_T("length"), name) )
	{
		val = static_cast<jcparam::IValue*>(jcparam::CTypedValue<JCSIZE>::Create(m_size) );
	}
}

void CBinaryBuffer::SetSubValue(LPCTSTR name, jcparam::IValue * val)
{
	if ( FastCmpT(_T("address"), name) )	SetAddress(dynamic_cast<IAddress*>(val) );
}

void CBinaryBuffer::SetMemoryAddress(JCSIZE add)
{
	m_address = new CMemoryAddress(add);
}

void CBinaryBuffer::SetFlashAddress(const CFlashAddress & add)
{
	m_address = new CFAddress(add);
}

void CBinaryBuffer::SetBlockAddress(FILESIZE add)
{
	m_address = new CBlockAddress(add);
}



template <> IAddress::ADDRESS_TYPE CTypedAddress<JCSIZE>::GetType()
{
	return ADD_MEMORY;
}

template <> IAddress::ADDRESS_TYPE CTypedAddress<FILESIZE>::GetType()
{
	return ADD_BLOCK;
}


///////////////////////////////////////////////////////////////////////////////
//----  CAddress  --------------------------------------------------------
void CFAddress::GetSubValue(LPCTSTR name, jcparam::IValue *& val)
{
	JCASSERT(NULL == val);
	if ( FastCmpT(FN_BLOCK, name) )			val = jcparam::CTypedValue<JCSIZE>::Create(m_block);
	else if ( FastCmpT(FN_PAGE, name) )	val = jcparam::CTypedValue<JCSIZE>::Create(m_page);
	else if ( FastCmpT(FN_CHUNK, name) )	val = jcparam::CTypedValue<JCSIZE>::Create(m_chunk);
}

void CFAddress::Clone(IAddress * & new_add)
{
	JCASSERT(NULL == new_add);
	CFAddress *add1 = new CFAddress(*this);
	new_add = static_cast<IAddress*>(add1);
}

void CFAddress::Offset(ISmiDevice * smi_dev, JCSIZE & offset)
{
	JCASSERT(smi_dev);
	CCardInfo info;
	smi_dev->GetCardInfo(info);

	JCSIZE chunk = offset / info.m_f_spck;
	offset -= info.m_f_spck * chunk;		// offset % sector per chunk
	m_chunk += chunk;

	JCSIZE page = m_chunk / info.m_f_ckpp;
	m_chunk -= page * info.m_f_ckpp;		// chunk % chunk per page
	m_page += page;

	JCSIZE block = m_page / info.m_f_ppb;
	m_page -= block * info.m_f_ppb;
	m_block += block;
}
