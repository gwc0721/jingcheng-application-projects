#include "stdafx.h"

#include "../feature_codec.h"
#include "encode.h"

LOCAL_LOGGER_ENABLE(_T("codec.en_8_10"), LOGGER_LEVEL_DEBUGINFO);

using namespace codec;
void feature_codec_register(jcscript::IPluginContainer * plugin_container)
{
	CCategoryComm * cat = NULL;
	cat = new CCategoryComm(_T("codec"));
	cat->RegisterFeatureT<CEncoder_8_10>();
	plugin_container->RegistPlugin(cat);
	cat->Release();
}

///////////////////////////////////////////////////////////////////////////////
// --  encoder 8-10 bit

LPCTSTR CEncoder_8_10::_BASE_TYPE::m_feature_name = _T("encode8_10");

CParamDefTab CEncoder_8_10::_BASE_TYPE::m_param_def_tab(
	CParamDefTab::RULE()
	(new CTypedParamDef<JCSIZE>(_T("source"), _T('s'), offsetof(CEncoder_8_10, m_source), _T("data source for encoder") ) )
	(new CTypedParamDef<TCHAR>(_T("disparity"), _T('r'), offsetof(CEncoder_8_10, m_input_rd), _T("current running disparity") ) ) 
	(new CTypedParamDef<CJCStringT>(_T("format"), _T('f'), offsetof(CEncoder_8_10, m_format), _T("output format") ) ) 
	);

#define _N	0x4000
#define _P	0x0000

// bit 15 (I / J): current rd为正或负是的输出是否一致
// bit 14 (y) : output running disparity是否要取反。  
#define _I(y, d)	(0x8000 | (y<<14) | 0x##d)	// current rd为负时，输出取反
#define _J(y, d)	(0x0000 | (y<<14) | 0x##d)	// current rd为负时，输出不变

const WORD CEncoder_8_10::SUB_BLOCK_6[] ={
	// current running disparity: +
	//			0		 1			2		  3			4		  5			6		  7
	/*D0~D7*/	_I(1,18), _I(1,22),	_I(1,12), _J(0,31),	_I(1,0A), _J(0,29),	_J(0,19), _I(0,07), 
	/*D8~D15*/	_I(1,06), _J(0,25),	_J(0,15), _J(0,34),	_J(0,0D), _J(0,2C),	_J(0,1C), _I(1,28),
	/*D16~D23*/	_I(1,24), _J(0,23),	_J(0,13), _J(0,32),	_J(0,0B), _J(0,2A),	_J(0,1A), _I(1,05),
	/*D24~D31*/	_I(1,0C), _J(0,26),	_J(0,16), _I(1,09),	_J(0,0E), _I(1,11),	_I(1,21), _I(1,14),
};

const WORD CEncoder_8_10::SUB_BLOCK_4[] ={
	//				0		 1		  2		   3		4		 5		  6		   7
	/*Dx.0~Dx.7*/	_I(1,4), _J(0,9), _J(0,5), _I(0,3), _I(1,2), _J(0,A), _J(0,6), _I(1,1),
	/*Dx.A7*/		_I(1,8)
};

CEncoder_8_10::CEncoder_8_10(void)
	: m_init(false)
	, m_input_rd(_T('P'))
	, m_current_rd(0)
	, m_source(0)
{
}
		
CEncoder_8_10::~CEncoder_8_10(void)
{

}

bool CEncoder_8_10::Init(void)
{
	m_init = true;
	if ( m_input_rd == _T('N') )	m_current_rd = _N;
	return true;
}

static inline TCHAR Rd2Symb(WORD rd)
{
	return (rd?_T('-'):_T('+'));
}

WORD Bit8To10(BYTE val, WORD & rd)
{
	BYTE vh = (val >> 5) & 0x7;		// 3 bit
	BYTE vl = val & 0x1F;				// 5 bit

	LOG_DEBUG(_T("%02X : D%d.%d, %c"), val, vl, vh, Rd2Symb(rd) );

	WORD sblk6 = CEncoder_8_10::SUB_BLOCK_6[vl];
	WORD sblk6_out = 0;
	if ( (sblk6 & 0x8000) && rd )		sblk6_out = (~sblk6) & 0x3F;
	else								sblk6_out = sblk6 & 0x3F;
	if ( (sblk6 & _N) )					rd ^= _N;

	LOG_DEBUG(_T("sub block 6: %02X, %c"), sblk6, Rd2Symb(rd))
	
	WORD sblk4 = CEncoder_8_10::SUB_BLOCK_4[vh];
	WORD sblk4_out = 0;
	if ( (sblk4 & 0x8000) && rd )		sblk4_out = (~sblk4) & 0xF;
	else								sblk4_out = sblk4 & 0xF;
	if ( (sblk4 & _N) )					rd ^= _N;

	LOG_DEBUG(_T("sub block 4: %02X, %c"), sblk4, Rd2Symb(rd));
	return (sblk6_out << 4 | sblk4_out); 
}

bool CEncoder_8_10::Invoke(jcparam::IValue * row, jcscript::IOutPort * outport)
{
	if (!m_init)	Init();
	DWORD val = 0;
	if ( row )		GetVal(row, val);
	else			val = m_source;
	
	LOG_DEBUG(_T("source = 0x%08X"), m_source);
	for (int ii = 0; ii < 4; ++ii)
	{
		BYTE v = (BYTE)(val & 0xFF);
		val >>= 8;

		WORD res = Bit8To10(v, m_current_rd);
		//BYTE vh = (v >> 5) & 0x7;		// 3 bit
		//BYTE vl = v & 0x1F;				// 5 bit

		//LOG_DEBUG(_T("%02X : D%d.%d, %c"), v, vl, vh, Rd2Symb(m_current_rd) );

		//WORD sblk6 = SUB_BLOCK_6[vl];
		//WORD sblk6_out = 0;
		//if ( (sblk6 & 0x8000) && m_current_rd )		sblk6_out = (~sblk6) & 0x3F;
		//else										sblk6_out = sblk6 & 0x3F;
		//if ( (sblk6 & _N) )						m_current_rd ^= _N;

		//LOG_DEBUG(_T("sub block 6: %02X, %c"), sblk6, Rd2Symb(m_current_rd))
		//
		//WORD sblk4 = SUB_BLOCK_4[vh];
		//WORD sblk4_out = 0;
		//if ( (sblk4 & 0x8000) && m_current_rd )		sblk4_out = (~sblk4) & 0xF;
		//else										sblk4_out = sblk4 & 0xF;
		//if ( (sblk4 & _N) )						m_current_rd ^= _N;

		//LOG_DEBUG(_T("sub block 4: %02X, %c"), sblk4, Rd2Symb(m_current_rd))
		_tprintf(_T("%03X%c"), res, RD2SYMB(m_current_rd) );
	}
	_tprintf(_T("(%c)\n"), Rd2Symb(m_current_rd));
	return false;
}
