#include "stdafx.h"
#include "../include/smi_data_type.h"

LOCAL_LOGGER_ENABLE(_T("smi_data_type"), LOGGER_LEVEL_ERROR);

LOG_CLASS_SIZE(CBinaryBuffer);

///////////////////////////////////////////////////////////////////////////////
//----  CAttributeItem  ----------------------------------------------------------
#define __COL_CLASS_NAME	CAttributeItem
BEGIN_COLUMN_TABLE()
	COLINFO_HEXL( WORD, 2, 	0, m_id,	_T("ID") )
	COLINFO_STR(			1, m_name,	_T("Name") )
	COLINFO_HEX( UINT,		2, m_val,	_T("Value") )
	COLINFO_STR(			3, m_desc,	_T("Description") )
END_COLUMN_TABLE()

#undef __COL_CLASS_NAME

///////////////////////////////////////////////////////////////////////////////
// -- Block Erase Count  --------------------------------------------------------
#define __COL_CLASS_NAME	CBlockEraseCount
BEGIN_COLUMN_TABLE()
	COLINFO_HEXL(UINT, 4, 	0, m_id,			_T("FBlock") )
	COLINFO_DEC( int,		1, m_erase_count,	_T("PE") )
END_COLUMN_TABLE()

#undef __COL_CLASS_NAME

///////////////////////////////////////////////////////////////////////////////
//----  CFBlockInfo  ----------------------------------------------------------

//int CFBlockInfo::m_channel_num = 0;

#define __COL_CLASS_NAME	CFBlockInfo
BEGIN_COLUMN_TABLE()
	COLINFO_HEXL(JCSIZE, 4,	0, m_f_add.m_block,		_T("FBlock") )
	COLINFO_HEXL(JCSIZE, 3,	1, m_f_add.m_page,		_T("FPage") )
	COLINFO_HEXL(BYTE, 2,	2,	m_spare.m_id,		_T("ID") )
	COLINFO_HEXL(WORD, 4,	3,	m_spare.m_hblock,	_T("hblock") )
	COLINFO_HEXL(WORD, 4,	4,	m_spare.m_hpage,	_T("hpage") )
	COLINFO_HEXL(BYTE, 2,	5,	m_spare.m_serial_no,_T("sn") )
	COLINFO_HEXL(BYTE, 2,	6,	m_spare.m_ecc_code,	_T("ECC") )
	(new CFBlockInfo::CBitErrColInfo(7, _T("BitError") ) )
	(new CDataVector::CDataVectorInfo(8, _T("Data") ) )
END_COLUMN_TABLE()
#undef __COL_CLASS_NAME

LOG_CLASS_SIZE(CFBlockInfo)
LOG_CLASS_SIZE_T( jcparam::CTableRowBase<CFBlockInfo>, 0 )


CFBlockInfo::CFBlockInfo(void) 
	: m_f_add(), m_spare(), m_id(0), m_error_bit(NULL)	
	, m_data(NULL)
{ 
}

CFBlockInfo::CFBlockInfo(const CFlashAddress & fadd, const CSpareData &spare) 
	: m_f_add(fadd), m_spare(spare), m_id(spare.m_id)
	, m_error_bit(NULL)
	, m_data(NULL)
{
}

CFBlockInfo::~CFBlockInfo(void)
{
	delete [] ((BYTE*)m_error_bit);
	delete m_data;
}

void CFBlockInfo::CreateErrorBit(BYTE channels, BYTE chunks)
{
	JCSIZE size = channels* chunks + sizeof(CErrorBit) -1;
	BYTE * p = new BYTE[size];
	memset(p, 0, size);
	m_error_bit = (CErrorBit*)(p);
	m_error_bit->m_channel_num = channels;
	m_error_bit->m_chunk_num = chunks;
}

void CFBlockInfo::SetErrorBit(const CSpareData & spare, BYTE chunk)
{
	JCASSERT(m_error_bit);
	JCASSERT(chunk < m_error_bit->m_chunk_num);
	// 填写error bit buffer
	for (BYTE ii = 0; ii < m_error_bit->m_channel_num; ++ii)
		m_error_bit->m_error_bit[ii * m_error_bit->m_chunk_num + chunk] = spare.m_error_bit[ii];
	// ECC CODE取最坏值
	m_spare.m_ecc_code |= spare.m_ecc_code;
}

void CFBlockInfo::PushData(CSectorBuf * data)
{
	JCASSERT(data);
	if (!m_data)	m_data = new CDataVector;
	m_data->PushData(data);
}

/*
void CFBlockInfo::CBitErrColInfo::GetText(void * row, CJCStringT & str) const
{
	CFBlockInfo * block = reinterpret_cast<CFBlockInfo*>(row);
	CErrorBit * err = block->m_error_bit;
	if ( !err ) return;

	TCHAR _str[MAX_CHANNEL * MAX_CHUNK * 3];
	wmemset(_str, 0, MAX_CHANNEL * MAX_CHUNK * 3);
	_str[MAX_CHANNEL * MAX_CHUNK * 3-1] = 0;

	LPTSTR __str = _str;
	JCSIZE remain = MAX_CHANNEL * MAX_CHUNK * 3-1;

	for (int ii = 0; ii < err->m_channel_num; ++ii)
	{
		JCSIZE offset = ii * err->m_chunk_num;
		for (int jj = 0; jj < err->m_chunk_num; ++jj)
		{
			BYTE errd = err->m_error_bit[offset + jj];
			__str[0] = stdext::hex2char(errd >> 4);
			__str[1] = stdext::hex2char(errd & 0x0F);
			__str += 2;
			remain -= 2;
		}
		*__str = _T(' ');
		__str ++;	// one space
		remain --;
	}
	*__str = 0;
	str = _str;
}
*/

void CFBlockInfo::CBitErrColInfo::ToStream(void * row, jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt) const
{
	JCASSERT(stream);
	CFBlockInfo * block = reinterpret_cast<CFBlockInfo*>(row);
	CErrorBit * err = block->m_error_bit;
	if ( !err ) return;
	for (int ii = 0; ii < err->m_channel_num; ++ii)
	{
		JCSIZE offset = ii * err->m_chunk_num;
		for (int jj = 0; jj < err->m_chunk_num; ++jj)
		{
			BYTE errd = err->m_error_bit[offset + jj];
			stream->Put( stdext::hex2char(errd >> 4) );
			stream->Put( stdext::hex2char(errd & 0x0F) );
		}
		stream->Put(_T(' '));
	}
}

void CFBlockInfo::CBitErrColInfo::CreateValue(BYTE * src, jcparam::IValue * & val) const
{
	CJCStringT str;
	GetText(src, str);
	val = jcparam::CTypedValue<CJCStringT>::Create(str);
}


CDataVector::~CDataVector(void)
{
	VDATA::iterator endit = m_data.end();
	VDATA::iterator it = m_data.begin();
	for ( ; it != endit; ++ it)		{JCASSERT(*it);	(*it)->Release();}
}

void CDataVector::PushData(CSectorBuf * data)
{
	JCASSERT(data);
	m_data.push_back(data);
	data->AddRef();
}

void CDataVector::ToStream(jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt)
{
	JCASSERT(stream);
	VDATA::iterator endit = m_data.end();
	VDATA::iterator it = m_data.begin();
	for ( ; it != endit; ++ it)
	{
		CSectorBuf * buf = (*it);
		JCASSERT(buf);
		buf->ToStream(stream, fmt, 0);
	}
}

void CDataVector::CDataVectorInfo::ToStream(void * row, jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt) const
{
	JCASSERT(stream);
	CFBlockInfo * block = reinterpret_cast<CFBlockInfo*>(row);
	CDataVector * data = block->m_data;

	LPCTSTR fn = stream->GetName();
	if ( data && (fmt == jcparam::VF_DEFAULT) &&  (NULL != fn) && (_T('0') != fn[0]) && (_T('#') != fn[0]) )
	{	// save binary data to file
		CJCStringT binfn = fn;
		binfn += _T(".bin");

		FILE * file = NULL;
		stdext::jc_fopen(&file, binfn.c_str(), _T("a+b"));
		if (NULL == file) THROW_ERROR(ERR_USER, _T("failure on openning %s"), binfn);

		_fseeki64(file, 0, SEEK_END);
		FILESIZE flen = _ftelli64(file);
		JCSIZE fsecs = (JCSIZE)(flen / SECTOR_SIZE);

		jcparam::IJCStream * bin_stream = NULL;
		CreateStreamBinaryFile(file, jcparam::WRITE, bin_stream);

		data->ToStream(bin_stream, jcparam::VF_BINARY);
		data->SetOffset(fsecs);

		bin_stream->Release();
	}

	if (data)	stream->Format(_T("<#%d,%d>"), data->GetOffset(), data->GetSectors());
	else		stream->Format(_T("no data"));
}

void CDataVector::CDataVectorInfo::CreateValue(BYTE * src, jcparam::IValue * & val) const
{
	val = jcparam::CTypedValue<CJCStringT>::Create(_T("<data>"));
}

///////////////////////////////////////////////////////////////////////////////
//----  CBadBlockInfo  --------------------------------------------------------
#define __COL_CLASS_NAME	CBadBlockInfo
BEGIN_COLUMN_TABLE()
	COLINFO_HEXL(WORD, 4, 	0, m_id,	_T("FBlock") )
	COLINFO_HEXL(WORD, 2,	1, m_page,	_T("ErrorPage") )
	COLINFO_HEXL(BYTE, 2,	2, m_code,	_T("ErrorCode") )
END_COLUMN_TABLE()	
#undef __COL_CLASS_NAME

template <> void jcparam::CConvertor<CBadBlockInfo::BAD_TYPE>::S2T(LPCTSTR, CBadBlockInfo::BAD_TYPE &t)
{
}

template <> void jcparam::CConvertor<CBadBlockInfo::BAD_TYPE>::T2S(const CBadBlockInfo::BAD_TYPE &t, CJCStringT &v)
{
}

LOG_CLASS_SIZE(CBadBlockInfo)
LOG_CLASS_SIZE_T( jcparam::CTableRowBase<CBadBlockInfo>, 1 )

///////////////////////////////////////////////////////////////////////////////
//----  CSmartAttribute  ------------------------------------------------------
#define __COL_CLASS_NAME	CSmartAttribute
BEGIN_COLUMN_TABLE()
	COLINFO_HEXL(BYTE, 2,	0, m_id,			_T("AttributeID") )
	COLINFO_TYPE(LPCTSTR,	1, m_name,			_T("Name") )
	COLINFO_DEC( BYTE,		2, m_current_val,	_T("Current") )
	COLINFO_DEC( BYTE,		3, m_worst_val,		_T("Worst") )
	COLINFO_DEC( BYTE,		4, m_threshold,		_T("Threashold") )
	COLINFO_HEXL(UINT64, 12,5, m_raw,			_T("Raw") )
END_COLUMN_TABLE()
#undef __COL_CLASS_NAME

LOG_CLASS_SIZE(CSmartAttribute)
LOG_CLASS_SIZE_T( jcparam::CTableRowBase<CSmartAttribute>, 2 )