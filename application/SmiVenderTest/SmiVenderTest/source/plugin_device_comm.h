#pragma once

#include <jcparam.h>
#include <SmiDevice.h>
#include <script_engine.h>

extern "C" bool RegisterPlugin(jcscript::IPluginContainer * plugin_cont);

///////////////////////////////////////////////////////////////////////////////
// -- read control data
class CDeviceReadCtrl
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceReadCtrl, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceReadCtrl(void);
	~CDeviceReadCtrl(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

public:
	CJCStringT	m_mode;
	JCSIZE		m_address;
	JCSIZE		m_length;
	JCSIZE		m_bank;
	static const TCHAR m_desc[];

protected:
	ISmiDevice * m_smi_dev;

};


///////////////////////////////////////////////////////////////////////////////
// -- read spare data
class CDeviceReadSpare
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceReadSpare, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceReadSpare(void);
	~CDeviceReadSpare(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	//bool	m_init;
	UINT	m_page_num;
	UINT	m_cur_page;
	JCSIZE	m_chunk_size;
	ISmiDevice * m_smi_dev;

	BYTE	m_channel_num;
	BYTE	m_chunk_num;

public:
	// parameters
	UINT	m_block, m_page;			// start page in multi page mode
	bool	m_read_all, m_read_ecc;
	BYTE	m_option;

	CJCStringT	m_mode;
	JCSIZE		m_address;
	JCSIZE		m_length;
	static const TCHAR m_desc[];
};

///////////////////////////////////////////////////////////////////////////////
// -- read flash data
class CDeviceReadFlash
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceReadFlash, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceReadFlash(void);
	~CDeviceReadFlash(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	ISmiDevice * m_smi_dev;
	CCardInfo	m_card_info;

public:
	// parameters
	UINT	m_block, m_page, m_chunk;
	UINT	m_count;
	bool	m_read_all, m_read_ecc;
	static const TCHAR m_desc[];

protected:
	UINT	m_cur_page, m_cur_chunk;
	UINT	m_last_chunk;

};



///////////////////////////////////////////////////////////////////////////////
// -- original bad block
class CDeviceOriginalBad
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceOriginalBad, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceOriginalBad(void);
	~CDeviceOriginalBad(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	ISmiDevice * m_smi_dev;
	CCardInfo	m_card_info;

public:
	// parameters
	UINT	m_block, m_page;
	static const TCHAR m_desc[];
};

///////////////////////////////////////////////////////////////////////////////
// -- physical original bad block
class CDevicePhOriginalBad
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDevicePhOriginalBad, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDevicePhOriginalBad(void);
	~CDevicePhOriginalBad(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	ISmiDevice * m_smi_dev;
	CCardInfo	m_card_info;

public:
	// parameters
	UINT	m_block, m_page;
	UINT	m_org_info;
	// 输出排序：false(default)按f-block排序，true，按physical block排序
	bool	m_order_phy;

	static const TCHAR m_desc[];
};

///////////////////////////////////////////////////////////////////////////////
// -- check new bad block list in info
class CDeviceNewBad
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceNewBad, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceNewBad(void);
	~CDeviceNewBad(void) {};

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);

public:
	// parameters
	UINT	m_block, m_page;
	static const TCHAR m_desc[];
};


///////////////////////////////////////////////////////////////////////////////
// -- difference address
class CDeviceDiffAdd
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceDiffAdd, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceDiffAdd(void);
	~CDeviceDiffAdd(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	ISmiDevice * m_smi_dev;
	CCardInfo	m_card_info;

	UINT	m_info_block, m_index_page, m_orphan_page;

public:
	// parameters
	UINT	m_block, m_page;
	UINT	m_org_info;

protected:
	void GetDiffAddress(JCSIZE index, WORD &type, WORD * add);
	BYTE	* m_orphan_buf;
	JCSIZE	m_start_index;

public:
	static const TCHAR m_desc[];
};

///////////////////////////////////////////////////////////////////////////////
// -- device info (card mode)
class CDeviceInfo
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceInfo, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceInfo(void);
	~CDeviceInfo(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	ISmiDevice * m_smi_dev;
	CCardInfo	m_card_info;

public:
	// parameters
	UINT	m_dummy;

public:
	static const TCHAR m_desc[];

protected:
	CAttributeRow * MakeAttributeRow(UINT id, const CJCStringT & name, UINT val, const CJCStringT & desc);
	bool PushAttribute(jcscript::IOutPort * outport, UINT id, const CJCStringT & name, UINT val, const CJCStringT & desc);
};

///////////////////////////////////////////////////////////////////////////////
// -- restore info
class CDeviceRestoreInfo
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceRestoreInfo, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceRestoreInfo(void);
	~CDeviceRestoreInfo(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	ISmiDevice * m_smi_dev;
	CCardInfo	m_card_info;

public:
	// parameters
	UINT	m_dummy;

public:
	static const TCHAR m_desc[];

protected:
	CAttributeRow * MakeAttributeRow(UINT id, const CJCStringT & name, UINT val, const CJCStringT & desc);
};

///////////////////////////////////////////////////////////////////////////////
// -- dump page
class CDeviceDumpPage
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceDumpPage, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceDumpPage(void);
	~CDeviceDumpPage(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	ISmiDevice * m_smi_dev;
	CCardInfo	m_card_info;
	UINT	m_cur_page, m_end_page;
	JCSIZE	m_chunk_size;

// parameters
public:
	UINT	m_block, m_page;
	bool	m_read_all, m_read_data;

public:
	static const TCHAR m_desc[];
};

///////////////////////////////////////////////////////////////////////////////
// -- simple features (本身不需要循环，不需要多次进入）

///////////////////////////////////////////////////////////////////////////////
// -- dump filter (simple feature)
class CDeviceDumpFilter
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceDumpFilter, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceDumpFilter(void)	:m_block(0) {};
	~CDeviceDumpFilter(void)	{};

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	enum DUMP {
		DUMP_NON = 0,	// 不保存这一类型block的任何data
		DUMP_BLOCK,		// 仅保存block的meta data
		DUMP_PAGE,		// 仅保存各page的meta data
		DUMP_DATA,		// 保存所有block的data
	};
	
	DUMP	m_dump_type[256];	// block id - 保存方法的对应表
// parameters
public:
	UINT		m_block;
	CJCStringT	m_dump_data;
	CJCStringT	m_dump_page_id;
	CJCStringT	m_dump_block_id;

public:
	static const TCHAR m_desc[];
};




///////////////////////////////////////////////////////////////////////////////
// -- erase count
class CDeviceEraseCount
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceEraseCount, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceEraseCount(void) : m_vendor(false) {}
	~CDeviceEraseCount(void) {}

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);

protected:

// parameters
public:
	bool m_vendor;
public:
	static const TCHAR m_desc[];
};


///////////////////////////////////////////////////////////////////////////////
// -- smart decode
class CDeviceSmartDecode
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceSmartDecode, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceSmartDecode(void);
	~CDeviceSmartDecode(void);

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);
	virtual bool Init(void);

protected:
	ISmiDevice * m_smi_dev;
	const CSmartAttrDefTab * m_smart_def_tab;
	jcparam::CColInfoList * m_col_list;

// parameters
public:
	CJCStringT	m_rev;	
	bool		m_by_id;

public:
	static const TCHAR m_desc[];

protected:
	CAttributeRow * MakeAttributeRow(UINT id, const CJCStringT & name, UINT val, const CJCStringT & desc);
};

///////////////////////////////////////////////////////////////////////////////
// -- identify device
class CDeviceIdentify
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceIdentify, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceIdentify(void) : m_vendor(false) {}
	~CDeviceIdentify(void) {};

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);

public:
	// parameters
	bool	m_vendor;	// vendor cmd or ata cmd
	static const TCHAR m_desc[];
};

///////////////////////////////////////////////////////////////////////////////
// -- smart
class CDeviceReadSmart
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceReadSmart, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceReadSmart(void) 	: m_vendor(false) {}
	~CDeviceReadSmart(void) {};

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);

public:
	// parameters
	bool	m_vendor;	// vendor cmd or ata cmd
	static const TCHAR m_desc[];
};

///////////////////////////////////////////////////////////////////////////////
// -- clean cache
class CDeviceCleanCache
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceCleanCache, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceCleanCache(void) : m_clean(false) {}
	~CDeviceCleanCache(void) {};

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);

public:
	// parameters
	bool m_clean;	// vendor cmd or ata cmd
	static const TCHAR m_desc[];
};

///////////////////////////////////////////////////////////////////////////////
// -- cpu reset
class CDeviceReset
	: virtual public jcscript::IFeature
	, public CFeatureBase<CDeviceReset, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CDeviceReset(void) {}
	~CDeviceReset(void) {};

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);

public:
	// parameters
	UINT m_dummy;	// vendor cmd or ata cmd
	static const TCHAR m_desc[];
};

///////////////////////////////////////////////////////////////////////////////
// -- sleep, temporally, this should be related to system plugin. 
class CSystemSleep
	: virtual public jcscript::IFeature
	, public CFeatureBase<CSystemSleep, CCategoryComm>
	, public CJCInterfaceBase
{
public:
public:
	CSystemSleep(void) : m_sleep(1) {}
	~CSystemSleep(void) {};

public:
	static LPCTSTR	desc(void) {return m_desc;}

public:
	virtual bool InternalInvoke(jcparam::IValue * row, jcscript::IOutPort * outport);

public:
	// parameters
	UINT m_sleep;	// vendor cmd or ata cmd
	static const TCHAR m_desc[];
};