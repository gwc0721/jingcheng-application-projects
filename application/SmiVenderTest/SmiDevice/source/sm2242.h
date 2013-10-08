#pragma once

#include <jcparam.h>
#include "smi_device_comm.h"

class CSM2242;
class C2242IspBuf;
class C2242CidBuf;

class CSM2242 : public CSmiDeviceComm
{
	friend class C2242CidTab;
	friend class C2242IspBuf;
public:
	enum CID_ID
	{
		CID_WL_QUEUE,
		CID_WL_DELTA,
		CID_DATA_REFRESH,
		CID_EARLY_RETIRE,
		CID_LBA48,
	};

public:
	static bool CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device);
	static bool Recognize(IStorageDevice * storage, BYTE * inquery);
	static bool LocalRecognize(BYTE * data)
	{
		char * ic_ver = (char*)(data + 0x2E);	
		return (NULL != strstr(ic_ver, "SM2242") );
	}

protected:
	CSM2242(IStorageDevice * dev, SMI_DEVICE_TYPE type = DEV_SM2242);
	virtual ~CSM2242(void);

public:
	// buf_len in sectors
	virtual void ReadISP(IIspBuf * & isp) {}
	virtual void ReadCID(ICidTab * & cid);
	virtual const CCidMap * GetCidDefination(void) const {return &m_cid_map;}
	//virtual JCSIZE GetProperty(const CJCStringT & prop_name);
	virtual bool GetProperty(LPCTSTR prop_name, UINT & val);

	virtual JCSIZE GetNewBadBlocks(BAD_BLOCK_LIST & bad_list);


protected:
	virtual bool CheckVenderCommand(BYTE * data)	{return LocalRecognize(data); }

	virtual bool Initialize(void);
	virtual void FlashAddToPhysicalAdd(const CFlashAddress & add, CSmiCommand & cmd, UINT option);
	virtual void GetSpare(CSpareData & spare, BYTE* spare_buf);

public:
	//virtual void GetSpareData(const CFlashAddress & add, CSpareData & spare);
	virtual void ReadSmartFromWpro(BYTE * data);

	virtual JCSIZE GetSystemBlockId(JCSIZE id);

	virtual const CSmartAttrDefTab * GetSmartAttrDefTab(void) const {return &m_smart_def_2242;}

// Implement CSmiDeviceComm Interface
protected:
	virtual LPCTSTR Name(void) const {return _T("SM2242");}

protected:
	// Read one sector in SRAM.
	//   ram_add must align 512B
	//   return size always = 512B
	//JCSIZE ReadSRAM(WORD ram_add, BYTE * buf, JCSIZE buf_len);
	//JCSIZE LoadISP(BYTE * buf);
	void DownloadISP(const C2242IspBuf *);
	void GetIspVersion(void);

protected:
	SMI_DEVICE_TYPE m_device_type;

	JCSIZE m_isp_length;	// length of isp, get from db

	char m_isp_ver[16];

	static const JCSIZE CID_LENGTH = 0x40;
	static const JCSIZE CID_OFFSET;
	static const JCSIZE ISP_MAX_LEN;
	static const CCidMap	m_cid_map;

protected:
	static const CSmartAttrDefTab	m_smart_def_2242;
};

class C2242CidTab : virtual public ICidTab, public jcparam::CParamSet
{
	friend class C2242IspBuf;
	friend class CSM2242;

protected:
	C2242CidTab(BYTE * buf, JCSIZE length, C2242IspBuf * isp);
	~C2242CidTab(void);

public:
	virtual DWORD GetValue(const CJCStringT & name);
	virtual void SetValue(const CJCStringT & name, DWORD val);
	virtual void Save(void);
	virtual void Format(FILE * file, LPCTSTR format);
	virtual void WriteHeader(FILE * file) {}
	virtual bool QueryInterface(const char * if_name, IJCInterface * &if_ptr);

protected:
	CBinaryBuffer	* m_buf;
	C2242IspBuf		* m_isp;
};

class C2242IspBuf : virtual public IIspBuf, public CBinaryBuffer
{
	friend class CSM2242;
	friend class C2242CidTab;

protected:
	C2242IspBuf(JCSIZE buf_size, CSM2242 * sm2242);
	~C2242IspBuf(void);
	void GetCidTab(C2242CidTab * &);
	void SetCidTab(BYTE * buf, JCSIZE len);

public:
	virtual void Save(void);

protected:
	CSM2242 * m_sm2242;
};

