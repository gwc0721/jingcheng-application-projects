#pragma once

#include <jcparam.h>
#include <SmiDevice.h>
#include <script_engine.h>

class CBlockReadCount
{
public:
	CBlockReadCount(void) {memset(this, 0, sizeof(CBlockReadCount)); };
	CBlockReadCount(const CBlockReadCount &rc) 
		: m_id(rc.m_id), m_read_count_1(rc.m_read_count_1), 
		m_read_count_2(rc.m_read_count_2), m_total_read_count(rc.m_total_read_count)
	{ };
	~CBlockReadCount(void) { };

public:
	JCSIZE m_id;	// block id
	JCSIZE m_read_count_1;
	JCSIZE m_read_count_2;
	JCSIZE m_total_read_count;
};

typedef jcparam::CTableRowBase<CBlockReadCount>	CBlockReadCountRow;

// read count result in sector
#define READ_COUNT_LENGTH (0x42)
// 32 block / sector
//#define BLOCK_COUNT	(READ_COUNT_LENGTH * 32)
#define BLOCK_COUNT	(0x82C)

class CFeature2246ReadCount
	: virtual public jcscript::IFeature
	, public CFeatureBase<CFeature2246ReadCount, CCategoryComm>
	, public CJCInterfaceBase
{
public:
	typedef CFeatureBase<CFeature2246ReadCount, CCategoryComm>  _BASE;

public:
	CFeature2246ReadCount(void);
	~CFeature2246ReadCount(void);

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