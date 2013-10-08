#pragma once

#include "istorage_device.h"

#include "smi_data_type.h"

#include "smi_command.h"

#define		_UNSUPPORT_		THROW_ERROR(ERR_UNSUPPORT, _T("This feature is not supported."));


enum SMI_DEVICE_TYPE
{	
	DEV_UNKNOWN = 0,
	DEV_SM2241, DEV_SM2242, DEV_SM224, DEV_SM223, DEV_SM2231AB
};

enum BLOCK_ID
{
	BID_SYSTEM = 0xFF000000,
	BID_ISP =	0xFF000100,
	BID_INFO =	0xFF000200,
	BID_WPRO =	0xFF000300,
	BID_LINK =	0xFF000400,
	BID_CHILDINFO = 0xFF000500,
	BID_SYSTEM_MASK = 0xFFFFFF00,
};

class ICidTab : virtual public jcparam::IValue, virtual public jcparam::IValueFormat
{
public:
	virtual DWORD GetValue(const CJCStringT & name) = 0;
	virtual void SetValue(const CJCStringT & name, DWORD val) = 0;
	virtual void Save(void) = 0;
};

class IIspBuf : virtual public jcparam::IValue, virtual public jcparam::IValueFormat
{
public:
	virtual void Save(void) = 0;
};

class CCIDDef
{
public:
	CCIDDef(LPCTSTR name, DWORD id, JCSIZE byte_offset, BYTE bit_offset, JCSIZE bit_len, LPCTSTR desc = NULL)
		: m_name(name), m_id(id), m_byte_offset(byte_offset), m_bit_offset(bit_offset)
		, m_bit_length(bit_len), m_desc(desc)
	{}

public:
	CJCStringT	m_name;
	DWORD		m_id;				// Example of bit offset and bit length, in case of length < 8
	JCSIZE		m_byte_offset;		// 7  6  5  4  3  2  1  0		: data = 6:3
	BYTE		m_bit_offset;		//			   ^ bit_offset		: offset = 3
	JCSIZE		m_bit_length;		//	  | <-- -->| bit_length		: length = 4
	LPCTSTR		m_desc;
public:
	LPCTSTR name(void) const {return m_name.c_str(); }
};

typedef jcparam::CStringTable<CCIDDef>	CCidMap;



class ISmiDevice : public virtual IJCInterface
{
public:
	enum CARD_INFO_MASK
	{
		CIM_F_BLOCK_NUM		= 0x00000001,
		CIM_F_PPB			= 0x00000002,	// f- page per block
		CIM_F_CKPP			= 0x00000004,	// f- chuck per page
		CIM_F_SPCK			= 0x00000008,	// sectors per f- chuck

		CIM_M_INTLV			= 0x00000010,	// interleave
		CIM_M_PLANE			= 0x00000020,	// plane
		CIM_M_CHANNEL		= 0x00000040,	// plane
	};

	enum READ_FLASH_OPTION
	{
		RFO_DISABLE_ECC		= 0x00000001,
		RFO_DISABLE_SCRAMB	= 0x00000002,
	};

	typedef std::vector<CBadBlockInfo>		BAD_BLOCK_LIST;

// Basic features (vender command wrapper)
	// General
	virtual bool GetCardInfo(CCardInfo &) = 0;
	virtual void SetCardInfo(const CCardInfo &, UINT mask) = 0;

	// Invoke a vendor command.
	//		buf_len in lba
	virtual bool VendorCommand(CSmiCommand &, READWRITE rd_wr, BYTE* data_buf, JCSIZE secs) = 0;

	// Read data from flash :
	//	secs [IN]: sectors (in 512 byte) to read.
	virtual void ReadFlashChunk(const CFlashAddress & add, CSpareData & spare, BYTE * buf, JCSIZE secs, UINT option = 0) = 0;
	virtual JCSIZE WriteFlash(const CFlashAddress & add, BYTE * buf, JCSIZE secs) = 0;
	virtual void EraseFlash(const CFlashAddress & add) = 0;

	// Read Flash ID
	virtual void ReadFlashID(BYTE * buf, JCSIZE secs) = 0;		// buffer size >= 1 sector

	// Read SFR
	virtual void ReadSFR(BYTE * buf, JCSIZE secs) = 0;

	// Read data from SRAM from ram_add with length (len) in byte
	virtual void ReadSRAM(WORD ram_add, JCSIZE len, BYTE * buf) = 0;

	virtual JCSIZE GetNewBadBlocks(BAD_BLOCK_LIST & bad_list) = 0;

// Expand features
	virtual bool GetProperty(LPCTSTR prop_name, UINT & val) = 0;
	virtual bool SetProperty(LPCTSTR prop_name, UINT val) = 0;

	virtual void ReadISP(IIspBuf * &) = 0;
	virtual void ReadCID(ICidTab * &) = 0;
	virtual const CCidMap * GetCidDefination(void) const = 0;

	virtual bool Initialize(void) = 0;

	// Get SMART data from device,
	//	[OUT] data: return 512 byte SMART data to caller. Caller need to insure data has more than 512 bytes.
	//	Remart: If storage device support SMART command use it. Else get smart data from WPRO
	virtual void GetSmartData(BYTE * data) = 0;
	virtual const CSmartAttrDefTab * GetSmartAttrDefTab(void) const = 0;
	virtual void ReadSmartFromWpro(BYTE * data) = 0;
};

typedef bool (* DEVICE_CREATOR)(IStorageDevice *, ISmiDevice *&);

typedef bool (* DEVICE_RECOGNIZER)(IStorageDevice *, BYTE *);

