#pragma once

#include "../include/ismi_device.h"
#include "../include/device_base.h"

class CCmdReadRam : public CSmiCommand
{
public:
	CCmdReadRam() : CSmiCommand() { id(0xF02A); };
	inline BYTE & add() { return m_cmd.b[7]; }
};

class CSmiDeviceComm 
	: virtual public ISmiDevice
	, public CSmiDeviceBase
	, public CJCInterfaceBase
{
protected:
	CSmiDeviceComm(IStorageDevice * dev);
	~CSmiDeviceComm(void);

public:
	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr);



// Implement of ISmiDevice
	virtual bool GetCardInfo(CCardInfo &);
	virtual void SetCardInfo(const CCardInfo &, UINT mask);

	virtual bool VendorCommand(CSmiCommand & cmd, READWRITE rd_wr, BYTE* data_buf, JCSIZE secs);
	// Read one flash sector from NAND flash
	virtual void ReadFlashChunk(const CFlashAddress & add, CSpareData & spare, BYTE * buf, JCSIZE secs, UINT option =0);
	virtual JCSIZE WriteFlash(const CFlashAddress & add, BYTE * buf, JCSIZE secs);
	virtual void EraseFlash(const CFlashAddress & add);


	// Read Flash ID
	virtual void ReadFlashID(BYTE * buf, JCSIZE secs);		// buffer size >= 1 sector
	// Read SFR
	virtual void ReadSFR(BYTE * buf, JCSIZE secs);
	// For ISmiDevice, read data in SRAM from ram_add with len (in byte)
	virtual void ReadSRAM(WORD ram_add, JCSIZE len, BYTE * buf);

// Others
	virtual void GetSmartData(BYTE * data);
	virtual const CSmartAttrDefTab * GetSmartAttrDefTab(void) const {return &m_smart_attr_tab;}

	// virtual functions 
protected:
	// 检查VenderCommand Read 0x55AA的结果是否正确，正确返回true，否则返回false中止Vender Command
	virtual bool CheckVenderCommand(BYTE * data) = 0;
	virtual void FlashAddToPhysicalAdd(const CFlashAddress & add, CSmiCommand & cmd, UINT option) = 0;
	virtual void GetSpare(CSpareData & spare, BYTE* spare_buf);
	virtual JCSIZE GetSystemBlockId(JCSIZE id);
	// Read SRAM in 512 bytes;
	virtual void ReadSRAM(WORD ram_add, BYTE * buf);

// CSmiDeviceComm interface
	virtual LPCTSTR Name(void) const = 0;

protected:
	IStorageDevice * m_dev;
	static const CSmartAttrDefTab	m_smart_attr_tab;

protected:
	//-- card info
	bool	m_isp_running;
	bool	m_info_block_valid;

	CCardInfo::NAND_TYPE	m_nand_type;	
	BYTE	m_channel_num;		//
	BYTE	m_ce_num;
	BYTE	m_interleave;		// 1 or 2
	BYTE	m_plane;			// 1 or 2

	// f parameter
	JCSIZE		m_mu;
	JCSIZE		m_f_block_num;			//
	JCSIZE		m_f_page_per_block;		// (*) fpages per fblock
	JCSIZE		m_f_chunk_per_page;		// (*) f-sectors per page
	JCSIZE		m_f_chunk_size;			// (*) in 512-byte

	// physical parameter
	JCSIZE		m_p_page_per_block;		// physical page per physical block
	JCSIZE		m_p_chunk_per_page;		// (*) physical chunk per page = m_f_chunk_per_page / m_plane

	WORD	m_info_index[2];
	WORD	m_isp_index[2];

};
