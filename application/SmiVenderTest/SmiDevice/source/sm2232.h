#pragma once

#include "smi_device_comm.h"

class CSM2232 : public CSmiDeviceComm
{
protected:
	CSM2232(IStorageDevice * storage) : CSmiDeviceComm(storage) {};
	virtual ~CSM2232(void) {};

public:
	static bool CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device);
	static bool Recognize(IStorageDevice * storage, BYTE * inquery/*, ISmiDevice *& smi_device*/);

public:
	virtual void ReadISP(IIspBuf * &) {};
	virtual void ReadCID(ICidTab * &) {};
	virtual const CCidMap * GetCidDefination(void) const {return NULL;};
	virtual void ReadInfoBlock(CFlashAddress & add, CBinaryBuffer * &buf) {};
	virtual void ReadFlash(const CFlashAddress & add, CBinaryBuffer * & buf) {};
	virtual void ReadSRAM(WORD ram_add, WORD bank, JCSIZE len, CBinaryBuffer * &buf) {};
	virtual void ReadSmartFromWpro(BYTE * data) {};
	virtual const CSmartAttrDefTab * GetSmartAttrDefTab(LPCTSTR rev) const {return &m_smart_def_2232;}
	//virtual JCSIZE GetNewBadBlocks(BAD_BLOCK_LIST & bad_list);

protected:
	virtual void GetSpare(CSpareData & spare, BYTE * spare_buf);
	virtual bool CheckVenderCommand(BYTE * data);
	virtual bool Initialize(void);
	virtual void FlashAddToPhysicalAdd(const CFlashAddress & add, CSmiCommand & cmd, UINT option);

// Implement CSmiDeviceComm Interface
protected:
	virtual LPCTSTR Name(void) const {return _T("SM2232");}
	static const CSmartAttrDefTab	m_smart_def_2232;
};

