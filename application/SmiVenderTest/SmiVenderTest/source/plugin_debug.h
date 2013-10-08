#pragma once


#include <SmiDevice.h>
#include "feature_base.h"

#include <list>


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
};


///////////////////////////////////////////////////////////////////////////////
// -- cache page

class CPluginDebug::CachePage
	//: public CFeatureBase<CPluginDebug::CachePage, CPluginDebug>
	: virtual public jcscript::ILoopOperate
	, public CLoopFeatureBase<CFBlockInfo>
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
	//virtual bool GetResult(jcparam::IValue * & val);
	//virtual bool Invoke(void);
	virtual void GetProgress(JCSIZE &cur_prog, JCSIZE &total_prog) const;
	virtual void Init(void);
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

//class CPluginDebug::CachePage
//	: public CFeatureBase<CPluginDebug::CachePage, CPluginDebug>
//	, public CJCInterfaceBase
//{
//	// f-block id
//};


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
	void Init(void); 

public:
	FILESIZE	m_lba;

protected:
	BLOCK_ROW	* m_hadd;
	bool		m_init;
	JCSIZE		m_block_size, m_page_size;		// sector per block / page
};


