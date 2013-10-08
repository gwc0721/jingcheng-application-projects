#pragma once
#include <jcparam.h>


class CBinaryBuffer;
class ISmiDevice;

///////////////////////////////////////////////////////////////////////////////
//----  CCardInfo  --------------------------------------------------------
class CCardInfo
{
public:
	enum NAND_TYPE 
	{
		NT_SLC,	NT_MLC,	NT_TLC, NT_SLC_MODE,
	};

	NAND_TYPE	m_type;				// SLC, MLC, TLC, SLC-mode
	// Card mode
	BYTE		m_channel_num;		//
	BYTE		m_interleave;		// 1 or 2
	BYTE		m_plane;			// 1 or 2
	// physical parameter
	JCSIZE		m_p_spp;		// sectors per physical page	
	JCSIZE		m_p_ppb;		// physical page per physical block
	// flash parameter
	JCSIZE		m_f_block_num;
	JCSIZE		m_f_ppb;			// page per block
	JCSIZE		m_f_ckpp;			// chunk per page
	JCSIZE		m_f_spck;			// sector per chunck

	CJCStringT	m_controller_name;
};


const LPCTSTR	FN_BLOCK	= _T("block");
const LPCTSTR	FN_PAGE		= _T("page");
const LPCTSTR	FN_CHUNK	= _T("chunk");
const LPCTSTR	FN_SECTOR	= _T("sector");
const LPCTSTR	FN_SECTORS	= _T("sectors");
const LPCTSTR	FN_LOGIC_ADD= _T("address");


///////////////////////////////////////////////////////////////////////////////
//----  CAddress  --------------------------------------------------------
class CFlashAddress
{
public:
	CFlashAddress(JCSIZE b = 0, JCSIZE p = 0, JCSIZE c = 0)
		: m_block(b), m_page(p), m_chunk(c) {};

public:
	// f parameter
	JCSIZE		m_block;		// 
	JCSIZE		m_page;			// fpages per fblock
	JCSIZE		m_chunk;		// f-sectors per page
};

class IAddress : virtual public jcparam::IValue
{
public:
	enum ADDRESS_TYPE
	{
		ADD_MEMORY,
		ADD_BLOCK,
		ADD_FLASH,
	};
public:
	virtual ADDRESS_TYPE GetType() = 0;
	virtual void * GetValue() = 0;
	// offset: flash add: sector, return remain sectors
	//  lba: sector, memory: byte
	virtual void Offset(ISmiDevice * smi, JCSIZE & offset) = 0;
	virtual void Clone(IAddress * &) = 0;
};

class CFAddress 
	: virtual public IAddress
	, public CJCInterfaceBase
	, public CFlashAddress
{
protected:
	CFAddress(const CFlashAddress & add) : CFlashAddress(add) {}
	CFAddress(const CFAddress & add) : CFlashAddress(add) {}
	friend class CBinaryBuffer;

public:
	virtual ADDRESS_TYPE GetType() {return IAddress::ADD_FLASH;}
	virtual void * GetValue()
	{
		return static_cast<CFlashAddress*>(this);
	}
	virtual void GetSubValue(LPCTSTR name, jcparam::IValue * & val);
	virtual void SetSubValue(LPCTSTR name, jcparam::IValue * val) {}
	virtual void Offset(ISmiDevice * smi_dev, JCSIZE & offset);
	virtual void Clone(IAddress * & new_add);
};

template <class ATYPE>
class CTypedAddress 
	: virtual public IAddress
	, public CJCInterfaceBase
{
protected:
	CTypedAddress(ATYPE add)	: m_offset(add) {};
	friend class CBinaryBuffer;
public:
	virtual ADDRESS_TYPE GetType();
	virtual void * GetValue() {	return (void*)m_offset; }
	virtual void GetSubValue(LPCTSTR name, jcparam::IValue * & val)
	{	
		JCASSERT(NULL == val);
		if ( (FastCmpT(FN_LOGIC_ADD, name))
			|| (FastCmpT(_T("memory_add"), name) ) )
		{
			val = static_cast<jcparam::IValue*>(jcparam::CTypedValue<ATYPE>::Create(m_offset) );
		}
	}
	virtual void SetSubValue(LPCTSTR name, jcparam::IValue * val) {}
	virtual void Offset(ISmiDevice * smi_dev, JCSIZE & offset)
	{
		//JCASSERT(0);
		m_offset += offset;
		offset = 0;
	}
	virtual void Clone(IAddress * & new_add)
	{
		JCASSERT(NULL == new_add);
		CTypedAddress<ATYPE> * add1 = new CTypedAddress<ATYPE>(*this);
		new_add = static_cast<IAddress*>(add1);

	}
protected:
	ATYPE	m_offset;
};


typedef CTypedAddress<JCSIZE> CMemoryAddress;
typedef CTypedAddress<FILESIZE> CBlockAddress;

class CSpareData
{
public:
	CSpareData()
	{
		memset(this, 0, sizeof(CSpareData) );
	}
public:
	BYTE	m_id;
	WORD	m_hblock;
	WORD	m_hpage;
	BYTE	m_erase_count;

	BYTE	m_ecc_code;
	BYTE	m_error_bit[8];		// error bits for each channel, 0xFF means uncorrectable

	// cache block / mother child / cache info 的序列号
	BYTE	m_serial_no;		
};


///////////////////////////////////////////////////////////////////////////////
//----  CBinaryBuffer  --------------------------------------------------------

// structure
//	binary_buffer
//		- address
//			- type (normal, lba, flash)
//			- value
//		- length	(read only)

class CBinaryBuffer
	: virtual public jcparam::IValueFormat, virtual public jcparam::IValue
	, public CJCInterfaceBase
	, public Factory1<JCSIZE, CBinaryBuffer>
{
	friend class Factory1<JCSIZE, CBinaryBuffer>;
protected:
	CBinaryBuffer(JCSIZE count);
	~CBinaryBuffer();

public:
	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr);
	virtual void Format(FILE * file, LPCTSTR format);
	virtual void WriteHeader(FILE * file) {}

	virtual void GetSubValue(LPCTSTR name, jcparam::IValue * & val);
	virtual void SetSubValue(LPCTSTR name, jcparam::IValue * val);

public:
	BYTE* Lock(void)	{ return m_array; }
	const BYTE* Lock(void) const {return m_array;}
	void Unlock(void) const {};

	JCSIZE GetSize(void) const { return m_size; }
	void GetAddress(IAddress * &add)
	{
		JCASSERT(NULL == add);
		add = m_address;
		if (add) add->AddRef();
	}
	void SetAddress(IAddress * add)
	{
		m_address = add;
		if (add) add->AddRef();
	}
	void SetMemoryAddress(JCSIZE add);
	void SetFlashAddress(const CFlashAddress & add);
	void SetBlockAddress(FILESIZE add);

protected:
	BYTE *		m_array;
	IAddress *	m_address;
	JCSIZE		m_size;		// always in byte
};

///////////////////////////////////////////////////////////////////////////////
//----    ----------------------------------------------------------
class CAttributeItem
{
public:
	CAttributeItem() : m_id(-1), m_val(0) {}
	CAttributeItem(UINT id, const CJCStringT & name, UINT val) 
		: m_id(id), m_name(name), m_val(val) {}
	CAttributeItem(UINT id, const CJCStringT & name, UINT val, const CJCStringT & desc) 
		: m_id(id), m_name(name), m_val(val), m_desc(desc) {}
public:
	WORD		m_id;
	UINT		m_val;
	CJCStringT	m_name;
	CJCStringT	m_desc;
};

typedef jcparam::CTypedTable<CAttributeItem>		CAttributeTable;


///////////////////////////////////////////////////////////////////////////////
//----  CFBlockInfo  ----------------------------------------------------------
class CFBlockInfo
{
public:
	class CBitErrColInfo;
	enum FBLOCK_TYPE
	{
		FBT_UNKNOW = 0,
		FBT_ORIGINAL_BAD,	FBT_INITIAL_BAD,	FBT_RUNTIME_BAD,
		FBT_MOTHER,			FBT_CHILD,			FBT_TEMP,
		FBT_FAT1,			FBT_FAT2,			
		FBT_SYSTEM,			FBT_INFO,			FBT_ISP,
		FBT_WROP,			
		FBT_SPARE1,			FBT_SPARE2,
	};

public:
	CFBlockInfo(void) : m_f_add(), m_spare(), m_id(0)	{ }

	CFBlockInfo(const CFlashAddress & fadd, const CSpareData &spare) 
		: m_f_add(fadd), m_spare(spare), m_id(spare.m_id)
	{}

public:
	WORD		m_id;
	CFlashAddress	m_f_add;
	CSpareData		m_spare;
	static int		m_channel_num;
};
typedef jcparam::CTableRowBase<CFBlockInfo>		BLOCK_ROW;
typedef jcparam::CTypedTable<CFBlockInfo>		CBlockTable;

class CFBlockInfo::CBitErrColInfo	: public jcparam::CColInfoBase
{
public:
	CBitErrColInfo(int id, LPCTSTR name)
		: jcparam::CColInfoBase(id, jcparam::VT_OTHERS, 0, name) {}

	virtual void GetText(void * row, CJCStringT & str) const
	{
		CFBlockInfo * block = reinterpret_cast<CFBlockInfo*>(row);
		TCHAR _str[8];
		for (int ii = 0; ii < CFBlockInfo::m_channel_num; ++ii)
		{
			stdext::jc_sprintf(_str, _T("%02X "), block->m_spare.m_error_bit[ii]);
			str += _str;
		}
	}

	virtual void CreateValue(BYTE * src, jcparam::IValue * & val) const
	{
		CJCStringT str;
		GetText(src, str);
		val = jcparam::CTypedValue<CJCStringT>::Create(str);
	}
};

///////////////////////////////////////////////////////////////////////////////
//----  CBadBlockInfo  --------------------------------------------------------

class CBadBlockInfo
{
public:
	enum BAD_TYPE
	{
		BT_ORIGINAL,	BT_INITIAL,	BT_RUNTIME,
		BT_ERASE_FAIL,	BT_ECC_FAIL, BT_NON_BAD_BLOCK,
	};
	class CBadTypeCov;
	class CEccMapCov;

public:
	CBadBlockInfo () : m_id(0), m_page(0xFFFF), m_code(0), m_type(BT_RUNTIME)	{}
	CBadBlockInfo (const CBadBlockInfo & bad) : m_id(bad.m_id), m_page(bad.m_page)
		, m_code(bad.m_code), m_type(bad.m_type)	{}
	CBadBlockInfo (WORD block, WORD page, BAD_TYPE type, BYTE code) : m_id(block), m_page(page), m_code(code), m_type(type)
		{}

public:
	WORD	m_id;		// F Block ID
	WORD	m_page;		// ECC failed page
	BAD_TYPE	m_type;	//
	BYTE	m_code;		// Error Code
	CJCStringT	m_ecc_map;	
};

class CBadBlockInfo::CBadTypeCov
{
public:
	static void T2S(const CBadBlockInfo::BAD_TYPE &t, CJCStringT &v)	{}
	static void S2T(LPCTSTR, CBadBlockInfo::BAD_TYPE &t)				{}
};

///////////////////////////////////////////////////////////////////////////////
//----  CSmartAttribute  ------------------------------------------------------
class CSmartAttribute
{
public:
	CSmartAttribute(LPCTSTR name, BYTE * buf) : m_name(name)
	{
		m_id = buf[0];
		m_current_val = buf[3];
		m_worst_val = buf[4];
		m_threshold = 0;
		DWORD lo = *(DWORD*) (buf + 5);
		WORD hi = *(WORD*) (buf + 9);
		m_raw = MAKEQWORD(lo, hi);
	}
public:
	BYTE		m_id;		// Attribute ID
	LPCTSTR		m_name;		
	BYTE		m_current_val;
	BYTE		m_worst_val;
	BYTE		m_threshold;
	UINT64		m_raw;
};

typedef jcparam::CTypedTable<CSmartAttribute>	CSmartAttrTab;

typedef CSmartAttrTab CSmartValue;

///////////////////////////////////////////////////////////////////////////////
//----  Smart Defination  ------------------------------------------------------
class CSmartAttributeDefine
{
public:
	CSmartAttributeDefine(BYTE id, LPCTSTR name, bool use = true) 
		: m_name(name), m_using(use)
	{
		stdext::itohex(m_str_id, 2, id);
	}
public:
	LPCTSTR	m_name;
	TCHAR	m_str_id[3];	// 将ID转换成16进制字符串用作索引
	bool	m_using;

public:
	LPCTSTR name(void) const {return m_str_id;};
};

typedef jcparam::CStringTable<CSmartAttributeDefine, std::vector<const CSmartAttributeDefine*> >	CSmartAttrDefTab;
