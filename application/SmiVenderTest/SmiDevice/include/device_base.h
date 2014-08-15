#pragma once

#include "istorage_device.h"
#include "ismi_device.h"

class CStorageDeviceBase 
	: virtual public IStorageDevice
	, public CJCInterfaceBase
{
public:
	virtual bool Inquiry(BYTE * buf, JCSIZE buf_len)	{return error(); }
	virtual bool SectorRead(BYTE * buf, FILESIZE lba, JCSIZE sectors)	{return error(); }
	virtual bool SectorWrite(const BYTE * buf, FILESIZE lba, JCSIZE sectors) 	{return error(); }

	// 用于性能测试，单位us。
	virtual UINT GetLastInvokeTime(void) {return 0;}

	virtual bool ScsiRead(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout) {return error();};
	virtual bool ScsiWrite(BYTE * buf, FILESIZE lba, JCSIZE secs, UINT timeout) {return error();};
	virtual FILESIZE GetCapacity(void)	{ return 0; }
	virtual bool DeviceLock(void)	{return error(); }
	virtual bool DeviceUnlock(void) 	{return false; }
	virtual bool Dismount(void)	{return error(); }

	virtual void SetDeviceName(LPCTSTR name) 	{error(); }
	virtual void UnmountAllLogical(void) {};
	virtual bool ScsiCommand(READWRITE rd_wr, BYTE *buf, JCSIZE length, BYTE *cb, JCSIZE cb_length, UINT timeout) 
	{return error();};
	virtual bool Recognize() {return true;}
	virtual void Detach(HANDLE & dev) {}

	//virtual bool ReadSmartData(BYTE * buf, JCSIZE len) { NOT_SUPPORT(bool); };
	//virtual bool IdentifyDevice(BYTE * buf, JCSIZE len) { NOT_SUPPORT(bool); };

#ifdef _DEBUG
	virtual HANDLE GetHandle() {return NULL;};
	virtual bool SectorVerify(void) {return true;}; 
#endif

protected:
	bool error(void) const
	{
		THROW_ERROR(ERR_APP, _T("No storage device is connected."));
		return false;
	}
};

class CSmiDeviceBase
	: virtual public ISmiDevice
{
public:
	virtual bool GetCardInfo(CCardInfo & card_info) 
	{
		card_info.m_channel_num = 0;
		card_info.m_p_ppb = 0;
		card_info.m_p_spp = 0;
		return true;
	}
	virtual void SetCardInfo(const CCardInfo &, UINT mask) {error();};

	// Invoke a vendor command.
	//		buf_len in lba
	virtual bool VendorCommand(CSmiCommand &, READWRITE rd_wr, BYTE* data_buf, JCSIZE secs) {return error();}

	virtual void ReadFlashChunk(const CFlashAddress & add, CSpareData & spare, BYTE * buf, JCSIZE secs, UINT option = 0) {error();}
	virtual JCSIZE WriteFlash(const CFlashAddress & add, BYTE * buf, JCSIZE secs) {error(); return 0;}
	virtual void EraseFlash(const CFlashAddress & add) {error();}


	virtual void ReadFlashID(BYTE * buf, JCSIZE secs) {error();}		// buffer size >= 1 sector

	// Read SFR
	virtual void ReadSFR(BYTE * buf, JCSIZE secs) {error();};
	
	// Read data from SRAM. Buffer size must large than 512 byte
	virtual void ReadSRAM(WORD ram_add, JCSIZE len, BYTE * buf)  {error();}
	virtual JCSIZE GetNewBadBlocks(BAD_BLOCK_LIST & bad_list) {return error();}

	virtual bool GetProperty(LPCTSTR prop_name, UINT & val) {return false;}
	virtual bool SetProperty(LPCTSTR prop_name, UINT val)	 {return false;}

	virtual void ReadISP(IIspBuf * &) {error();}
	virtual void ReadCID(ICidTab * &) {error();}
	virtual const CCidMap * GetCidDefination(void) const {error(); return NULL;}


	// Get SMART data from device,
	//	[OUT] data: return 512 byte SMART data to caller. Caller need to insure data has more than 512 bytes.
	//	Remart: If storage device support SMART command use it. Else get smart data from WPRO
	virtual bool VendorReadSmart(BYTE * data)  { NOT_SUPPORT(bool); }
	virtual const CSmartAttrDefTab * GetSmartAttrDefTab(LPCTSTR rev) const  {error(); return NULL;}
	virtual void ReadSmartFromWpro(BYTE * data)  {error();}

	virtual bool VendorIdentifyDevice(BYTE * data) { NOT_SUPPORT(bool); }

	virtual bool Initialize(void) {return true;}

public:
	static LPCTSTR PROP_WPRO;			// UINT
	static LPCTSTR PROP_CACHE_NUM;		// UINT
	static LPCTSTR PROP_INFO_BLOCK;		// UINT, hi word: info 1, lo word: info 2
	static LPCTSTR PROP_ISP_BLOCK;		// UINT, hi word: isp 1, lo word: isp 2
	static LPCTSTR PROP_INFO_PAGE;		// UINT
	static LPCTSTR PROP_ORG_BAD_INFO;		// UINT
	static LPCTSTR PROP_ISP_MODE;		// UINT

	//// UINT, hi word: Combined Orphan Page, lo word: Block Index Page
	//static LPCTSTR PROP_INFO_DIFFADD_PAGE;

	//static LPCTSTR FBLOCK_NUM;			// UINT

protected:
	bool error(void) const
	{
		THROW_ERROR(ERR_APP, _T("No SMI device is connected."));
		return false;
	}


};

