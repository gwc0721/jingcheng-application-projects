#include "stdafx.h"
#include "../include/smi_data_type.h"

LOCAL_LOGGER_ENABLE(_T("smi_data_type"), LOGGER_LEVEL_ERROR);
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
//----  CFBlockInfo  ----------------------------------------------------------

int CFBlockInfo::m_channel_num = 0;

#define __COL_CLASS_NAME	CFBlockInfo
BEGIN_COLUMN_TABLE()
	COLINFO_HEXL(JCSIZE, 4,	0, m_f_add.m_block,		_T("FBlock") )
	COLINFO_HEXL(JCSIZE, 3,	1, m_f_add.m_page,		_T("FPage") )
	COLINFO_HEX(JCSIZE, 	2, m_f_add.m_chunk,		_T("FChunk") )
	COLINFO_HEXL(BYTE, 2,	3,	m_spare.m_id,		_T("ID") )
	COLINFO_HEXL(WORD, 4,	4,	m_spare.m_hblock,	_T("hblock") )
	COLINFO_HEXL(WORD, 3,	5,	m_spare.m_hpage,	_T("hpage") )
	COLINFO_HEXL(BYTE, 2,	6,	m_spare.m_serial_no,_T("sn") )
	COLINFO_HEX( BYTE,		7,	m_spare.m_ecc_code,	_T("ECC") )
	(new CFBlockInfo::CBitErrColInfo(8, _T("BitError") ) )
END_COLUMN_TABLE()
#undef __COL_CLASS_NAME

LOG_CLASS_SIZE(CFBlockInfo)
LOG_CLASS_SIZE_T( jcparam::CTableRowBase<CFBlockInfo>, 0 )

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