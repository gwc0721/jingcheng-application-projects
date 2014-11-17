#pragma once 

#include "sm2244lt.h"

class CSM2246 : public CSmiDeviceComm
{
public:
	static bool CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device);
	static bool Recognize(IStorageDevice * storage, BYTE * inquery);
	static bool LocalRecognize(BYTE * data);

public:
	virtual void ReadSmartFromWpro(BYTE * data) {};
	virtual JCSIZE GetSystemBlockId(JCSIZE id) {return 0;};

protected:
	CSM2246(IStorageDevice * dev);
	virtual ~CSM2246(void) {}

	virtual bool CheckVenderCommand(BYTE * data)	{return CSM2246::LocalRecognize(data); }
	virtual void FlashAddToPhysicalAdd(const CFlashAddress & add, CSmiCommand & cmd, UINT option) {}
	virtual const CSmartAttrDefTab * GetSmartAttrDefTab(LPCTSTR rev) const {return NULL;}
	virtual void GetSpare(CSpareData & spare, BYTE* spare_buf) {}

	virtual bool GetProperty(LPCTSTR prop_name, UINT & val) {return false;}

// Implement CSmiDeviceComm Interface
protected:
	virtual LPCTSTR Name(void) const {return _T("SM2246");}
	virtual bool Initialize(void) {return true;};
};