#pragma once

#include <jcparam.h>
#include "SM2244LT.h"

class CSM2236 : public CLT2244
{
public:
	static bool CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device);
	static bool Recognize(IStorageDevice * storage, BYTE * inquery);
	static bool LocalRecognize(BYTE * data)
	{
		char * ic_ver = (char*)(data + 0x2E);	
		return (NULL != strstr(ic_ver, "SM2236-") );
	}

public:
	virtual void ReadSmartFromWpro(BYTE * data);
	virtual JCSIZE GetSystemBlockId(JCSIZE id);

protected:
	CSM2236(IStorageDevice * dev);
	virtual ~CSM2236(void) {}

	virtual bool CheckVenderCommand(BYTE * data)	{return CSM2236::LocalRecognize(data); }
	virtual void FlashAddToPhysicalAdd(const CFlashAddress & add, CSmiCommand & cmd, UINT option);
	virtual const CSmartAttrDefTab * GetSmartAttrDefTab(LPCTSTR rev) const 
	{
		if ( rev && _tcscmp(rev, _T("NECI"))==0 )	return &m_smart_neci;
		else return &m_smart_def_2236;
	}
	virtual void GetSpare(CSpareData & spare, BYTE* spare_buf);

	virtual bool GetProperty(LPCTSTR prop_name, UINT & val);

	// Read SRAM in 512 bytes;
	virtual void ReadSRAM(WORD ram_add, WORD bank, BYTE * buf);

// Implement CSmiDeviceComm Interface
protected:
	virtual LPCTSTR Name(void) const {return _T("SM2236");}
	virtual bool Initialize(void);

protected:
	static const CSmartAttrDefTab	m_smart_def_2236;
	//static const CSmartAttrDefTab	m_smart_neci;
	BYTE m_info_page, m_bitmap_page, m_orphan_page, m_blockindex_page;
	WORD m_org_bad_info;
};