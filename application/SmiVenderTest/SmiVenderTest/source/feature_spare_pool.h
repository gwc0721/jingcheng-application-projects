#pragma once

#include <jcparam.h>
#include <SmiDevice.h>
#include <script_engine.h>
//#include "feature_base.h"
//#include "category_comm.h"


class CFeatureSparePool
	: virtual public jcscript::IFeature
	, public CFeatureBase<CFeatureSparePool, CCategoryComm>
	, public CJCInterfaceBase
{
public:
	typedef CFeatureBase<CFeatureSparePool, CCategoryComm>  _BASE;

public:
	CFeatureSparePool(void);
	~CFeatureSparePool(void);

public:
	virtual bool Invoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	//virtual bool IsRunning(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

protected:
	virtual bool Init(void);

public:
	CJCStringT m_file_name;

protected:
	bool m_init;
	ISmiDevice * m_smi_dev;

	BYTE * m_read_count;

	JCSIZE m_block_count;

	static const TCHAR m_desc[];

};