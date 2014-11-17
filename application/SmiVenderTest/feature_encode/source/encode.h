#pragma once

//#include <jcparam.h>
#include <script_engine.h>

namespace codec
{

///////////////////////////////////////////////////////////////////////////////
// 8bit -> 10bit encoder

	class CEncoder_8_10
		: virtual public jcscript::IFeature
		, public CFeatureBase<CEncoder_8_10, CCategoryComm>
		, public CJCInterfaceBase
	{
	public:
		CEncoder_8_10(void);
		virtual ~CEncoder_8_10(void);

	public:
		virtual bool Invoke(jcparam::IValue * row, jcscript::IOutPort * outport);
		static LPCTSTR	desc(void) {return _T("8-10bit encoder");}
		virtual bool Init(void);

	protected:
		bool	m_init;
		WORD	m_current_rd;

	public:
		static const WORD SUB_BLOCK_6[32];
		static const WORD SUB_BLOCK_4[9];

	public:
		JCSIZE m_source;
		// current running disparity. p: plus, n: negative
		TCHAR	m_input_rd;
		// output format: hex, binary
		CJCStringT	m_format;	
	};

};
