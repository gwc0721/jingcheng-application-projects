#pragma once

#include <SmiDevice.h>
#include <script_engine.h>
//#include "feature_base.h"

#include <list>
#include "ata_trace.h"

extern "C" bool RegisterPluginDebug(jcscript::IPluginContainer * plugin_cont);

class CPluginDebug 
	: virtual public jcscript::IPlugin
	, public CPluginBase2<CPluginDebug>
{
public:
	CPluginDebug(void);
	virtual ~CPluginDebug(void);

public:
	virtual bool Reset(void) {return true;} 

public:
	class CachePage;
	class LbaToHadd;
	class DelayPattern;
};


///////////////////////////////////////////////////////////////////////////////
// -- cache page

class CPluginDebug::CachePage
	: public virtual jcscript::IFeature
	, public CFeatureBase<CPluginDebug::CachePage, CPluginDebug>
	, public CJCInterfaceBase
{
	// f-block id
public:
	typedef CFeatureBase<CPluginDebug::CachePage, CPluginDebug>	_BASE;
public:
	CachePage(void);
	virtual ~CachePage(void);

public:
	virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const;
	virtual bool Init(void);
	virtual bool InvokeOnce(void);

protected:
	void ReadSpare(JCSIZE block, JCSIZE page, CFBlockInfo* info);

public:
	CJCStringT	m_file_name;

protected:
	typedef std::list<CFBlockInfo *> BLOCK_INFO_LIST;
	BLOCK_INFO_LIST	m_cache_list;

	ISmiDevice * m_smi_dev;
	JCSIZE	m_fblock_num;
	JCSIZE	m_page_per_block;
	JCSIZE	m_chunk_size;

	// dummy for save data
	BYTE * m_buf;

	// for loop output
	BLOCK_INFO_LIST::iterator	m_cur_cache;
	JCSIZE	m_cur_page;
};

///////////////////////////////////////////////////////////////////////////////
// -- convert h-address to lba 


///////////////////////////////////////////////////////////////////////////////
// -- convert lba to h-address

class CPluginDebug::LbaToHadd
	: public CFeatureBase<CPluginDebug::LbaToHadd, CPluginDebug>
	, public CJCInterfaceBase
{
	// f-block id
public:
	typedef CFeatureBase<CPluginDebug::LbaToHadd, CPluginDebug>	_BASE;

public:
	LbaToHadd(void);
	virtual ~LbaToHadd(void);

public:
	virtual bool GetResult(jcparam::IValue * & val);
	virtual bool Invoke(void);
public:
	virtual bool Init(void); 

public:
	FILESIZE	m_lba;

protected:
	BLOCK_ROW	* m_hadd;
	bool		m_init;
	JCSIZE		m_block_size, m_page_size;		// sector per block / page
};


///////////////////////////////////////////////////////////////////////////////
// -- delay pattern
class CPluginDebug::DelayPattern
	: public virtual jcscript::IFeature
	, public CFeatureBase<CPluginDebug::DelayPattern, CPluginDebug>
	, public CJCInterfaceBase
{
	// f-block id
public:
	typedef CFeatureBase<CPluginDebug::DelayPattern, CPluginDebug>	_BASE;
public:
	DelayPattern(void);
	virtual ~DelayPattern(void);

public:
	virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const;
	virtual bool Init(void);
	virtual bool InvokeOnce(void);

protected:
	JCSIZE H2LBA(JCSIZE block, JCSIZE page); 

public:
	UINT	m_latency;
	JCSIZE	m_offset;
	bool	m_first_page;

protected:
	JCSIZE m_block, m_page;
};


///////////////////////////////////////////////////////////////////////////////
// -- repeat
class CDebugRepeat
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDebugRepeat, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDebugRepeat(void);
	~CDebugRepeat(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	int m_repeat_count;

public:
	// parameters
	// repeat times, -1 means infinitive 
	int	m_repeat;
	static const TCHAR m_desc[];
};

///////////////////////////////////////////////////////////////////////////////
// -- debug inject
class CDebugInject
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDebugInject, CCategoryComm>
	, public CJCInterfaceBase
{
public:
	CDebugInject(void);
	virtual ~CDebugInject(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}
	//virtual bool GetResult(jcparam::IValue * & val);
	//virtual bool Invoke(void);
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

public:
	JCSIZE		m_lba;
	JCSIZE		m_sectors;
	UINT		m_command;
	UINT		m_time_out;
	static const TCHAR m_desc[];

protected:
	IStorageDevice * m_storage;

	BYTE *	m_data_buf;
	JCSIZE m_max_secs;

	CAtaTraceRow * m_trace;
};