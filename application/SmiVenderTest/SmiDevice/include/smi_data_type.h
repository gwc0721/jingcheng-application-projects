#pragma once
#include <jcparam.h>
#include <vector>

#define SECTOR_SIZE	(512)

#define MAX_CHANNEL		4
#define MAX_CHUNK		32

class CBinaryBuffer;
class ISmiDevice;

enum ISP_MODE
{
	ISPM_UNKNOWN = 0,	ISPM_ROM_CODE = 1,	ISPM_ISP = 2,
	ISPM_MPISP = 3,
};

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
	JCSIZE		m_p_bpd;		// physical block number per die
	JCSIZE		m_p_die;		// die number of NAND flash
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
	virtual void GetValueText(CJCStringT & str) const {};
	virtual void SetValueText(LPCTSTR str)  {};
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
	virtual void GetValueText(CJCStringT & str) const {};
	virtual void SetValueText(LPCTSTR str)  {};
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
	BYTE	m_error_bit[MAX_CHANNEL];		// error bits for each channel, 0xFF means uncorrectable

	// cache block / mother child / cache info 的序列号
	BYTE	m_serial_no;		
	// cache block index
	BYTE	m_index;
	BYTE	m_raw[16];
};

class CSpareRawInfo : public jcparam::COLUMN_INFO_BASE
{
public:
	CSpareRawInfo(int id, LPCTSTR name, JCSIZE width)
		: jcparam::COLUMN_INFO_BASE(id, jcparam::VT_OTHERS, 0, name) 
		, m_width(width){}
	virtual void ToStream(void * row, jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt) const;
	virtual void CreateValue(BYTE * src, jcparam::IValue * & val) const;
protected:
	JCSIZE m_width;

};

extern const TCHAR __SPACE[128];
extern void _local_itohex(LPTSTR str, JCSIZE dig, UINT d);
extern const TCHAR * SPACE;
const JCSIZE STR_BUF_LEN  = 80;

extern const JCSIZE ADD_DIGS; 
extern const JCSIZE HEX_OFFSET;
extern const JCSIZE ASCII_OFFSET;

///////////////////////////////////////////////////////////////////////////////
//----  CBinaryBuffer  --------------------------------------------------------

// structure
//	binary_buffer
//		- address
//			- type (normal, lba, flash)
//			- value
//		- length	(read only)

class CBinaryBuffer
	: virtual public jcparam::IVisibleValue
	, public CJCInterfaceBase
	, public Factory1<JCSIZE, CBinaryBuffer>
{
	friend class Factory1<JCSIZE, CBinaryBuffer>;
protected:
	CBinaryBuffer(JCSIZE count);
	~CBinaryBuffer();

public:
	virtual void GetValueText(CJCStringT & str) const {};
	virtual void SetValueText(LPCTSTR str)  {};

	virtual void GetSubValue(LPCTSTR name, jcparam::IValue * & val);
	virtual void SetSubValue(LPCTSTR name, jcparam::IValue * val);

	virtual void ToStream(jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt, DWORD param) const;
	virtual void FromStream(jcparam::IJCStream * str, jcparam::VAL_FORMAT param) { NOT_SUPPORT0; };

protected:
	// 按照文本格式输出，
	//  offset：输出偏移量，字节为单位，行对齐（仅16的整数倍有效）
	//  len：输出长度，字节为单位，行对齐
	void OutputText(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE len) const;
	void OutputBinary(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE len) const;

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
//----  CSectorBuf  --------------------------------------------------------
class CSectorBuf;


class CSectorBuf
	: virtual public jcparam::IBinaryBuffer
	, public CJCInterfaceBase
{
	friend bool CreateSectorBuf(FILESIZE sec_add);
// 固定512 byte
public:
	CSectorBuf(FILESIZE sec_add = 0);
	~CSectorBuf();

public:
	virtual void GetValueText(CJCStringT & str) const { NOT_SUPPORT0; };
	virtual void SetValueText(LPCTSTR str)  { NOT_SUPPORT0; };

	virtual void GetSubValue(LPCTSTR name, jcparam::IValue * & val);
	virtual void SetSubValue(LPCTSTR name, jcparam::IValue * val);

	virtual void ToStream(jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt, DWORD param) const;
	virtual void FromStream(jcparam::IJCStream * str, jcparam::VAL_FORMAT param) { NOT_SUPPORT0; };

protected:
	// 按照文本格式输出，
	//  offset：输出偏移量，字节为单位，行对齐（仅16的整数倍有效）
	//  len：输出长度，字节为单位，行对齐
	void OutputText(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE len) const;
	void OutputBinary(jcparam::IJCStream * stream, JCSIZE offset, JCSIZE len) const;

public:
	virtual BYTE* Lock(void)	{ return m_array; }
	const BYTE* Lock(void) const {return m_array;}
	virtual void Unlock(BYTE * ptr) {};
	virtual JCSIZE GetSize(void) const {return SECTOR_SIZE;}

protected:
	BYTE 		m_array[SECTOR_SIZE];
	FILESIZE	m_sec_add;		// always in byte
};

bool CreateSectorBuf(FILESIZE sec_add, CSectorBuf * & buf);

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

typedef jcparam::CTableRowBase<CAttributeItem>		CAttributeRow;

typedef jcparam::CTypedTable<CAttributeItem>		CAttributeTable;



///////////////////////////////////////////////////////////////////////////////
//----  Error bit  ------------------------------------------------------------
class CErrorBit
{	// 每个Page的ErrorBit数
public:
	BYTE m_channel_num;
	BYTE m_chunk_num;

	BYTE m_error_bit[1];
};

///////////////////////////////////////////////////////////////////////////////
//----  CFBlockInfo  ----------------------------------------------------------
class CDataVector
{
public:
	class CDataVectorInfo;
	typedef std::vector<CSectorBuf*>	VDATA;

public:
	~CDataVector(void);
	void PushData(CSectorBuf * data);
	JCSIZE GetSectors(void) {return m_data.size();};
	JCSIZE GetOffset(void) {return m_offset;};
	void SetOffset(JCSIZE offset)	{m_offset = offset;};
	void ToStream(jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt);

protected:
	JCSIZE	m_offset;	// offset in file
	VDATA	m_data;
};

class CDataVector::CDataVectorInfo	: public jcparam::COLUMN_INFO_BASE
{
public:
	CDataVectorInfo(int id, LPCTSTR name)
		: jcparam::COLUMN_INFO_BASE(id, jcparam::VT_OTHERS, 0, name) {}

	virtual void ToStream(void * row, jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt) const;
	virtual void CreateValue(BYTE * src, jcparam::IValue * & val) const;
};

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
	CFBlockInfo(void);
	CFBlockInfo(const CFlashAddress & fadd, const CSpareData &spare);
	~CFBlockInfo(void);

	void SetSpare(const CSpareData & spare) {m_spare = spare;}
	void SetAddress(const CFlashAddress & add)	{m_f_add = add;}

	void CreateErrorBit(BYTE channels, BYTE chunks);
	void SetErrorBit(const CSpareData & spare, BYTE chunk);
	void PushData(CSectorBuf * data);

public:
	WORD			m_id;
	CFlashAddress	m_f_add;
	CSpareData		m_spare;
	CErrorBit *		m_error_bit;
	CDataVector *	m_data;
};

typedef jcparam::CTableRowBase<CFBlockInfo>		BLOCK_ROW;
typedef jcparam::CTypedTable<CFBlockInfo>		CBlockTable;

class CFBlockInfo::CBitErrColInfo	: public jcparam::COLUMN_INFO_BASE
{
public:
	CBitErrColInfo(int id, LPCTSTR name)
		: jcparam::COLUMN_INFO_BASE(id, jcparam::VT_OTHERS, 0, name) {}


	virtual void ToStream(void * row, jcparam::IJCStream * stream, jcparam::VAL_FORMAT fmt) const;
	//virtual void GetText(void * row, CJCStringT & str) const;
	virtual void CreateValue(BYTE * src, jcparam::IValue * & val) const;
};

///////////////////////////////////////////////////////////////////////////////
// -- Block Erase Count  --------------------------------------------------------
class CBlockEraseCount
{
public:
	UINT m_id;		// block id
	int m_erase_count;
};

typedef jcparam::CTableRowBase<CBlockEraseCount> CBlockEraseCountRow;

///////////////////////////////////////////////////////////////////////////////
//----  CBadBlockInfo  --------------------------------------------------------


class CNewBadBlock
{
public:
	CNewBadBlock(WORD block, WORD page, BYTE code) : m_id(block), m_page(page), m_code(code)
		{}
public:
	WORD	m_id;		// F Block ID
	WORD	m_page;		// ECC failed page
	BYTE	m_code;		// Error Code
};

typedef jcparam::CTableRowBase<CNewBadBlock>		CNewBadBlockRow;

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
