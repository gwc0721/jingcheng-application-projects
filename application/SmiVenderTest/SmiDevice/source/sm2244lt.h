#pragma once

#include <jcparam.h>
#include "SM2242.h"

class CCmdBaseLT2244: public CSmiCommand
{
public:
	CCmdBaseLT2244(JCSIZE count) : CSmiCommand(count) {};
	inline void block(WORD b) 
	{
		m_cmd.b[2] = HIBYTE(b);
		m_cmd.b[3] = LOBYTE(b);
	}
	inline WORD block() { return MAKEWORD(m_cmd.b[3], m_cmd.b[2]); }
	inline void page(WORD p)
	{
		m_cmd.b[4] = HIBYTE(p);
		m_cmd.b[5] = LOBYTE(p);
	}
	inline WORD page() { return MAKEWORD(m_cmd.b[5], m_cmd.b[4]); }

	inline BYTE & chunk() { return m_cmd.b[6];}
	inline BYTE & mode()		{ return m_cmd.b[8]; }
};

class CLT2244 : public CSM2242
{
public:
	static bool CreateDevice(IStorageDevice * storage, ISmiDevice *& smi_device);
	static bool Recognize(IStorageDevice * storage, BYTE * inquery);
	static bool LocalRecognize(BYTE * data)
	{
		char * ic_ver = (char*)(data + 0x2E);	
		return (NULL != strstr(ic_ver, "SM2244LT") );
	}

public:
	virtual void ReadSmartFromWpro(BYTE * data);
	virtual JCSIZE GetSystemBlockId(JCSIZE id);

protected:
	CLT2244(IStorageDevice * dev);
	virtual ~CLT2244(void) {}

	virtual bool CheckVenderCommand(BYTE * data)	{return CLT2244::LocalRecognize(data); }
	virtual void FlashAddToPhysicalAdd(const CFlashAddress & add, CSmiCommand & cmd, UINT option);
	virtual const CSmartAttrDefTab * GetSmartAttrDefTab(void) const {return &m_smart_def_2244lt;}
	virtual void GetSpare(CSpareData & spare, BYTE* spare_buf);

	virtual bool GetProperty(LPCTSTR prop_name, UINT & val);

// Implement CSmiDeviceComm Interface
protected:
	virtual LPCTSTR Name(void) const {return _T("LT2244");}
	virtual bool Initialize(void);

protected:
	static const CSmartAttrDefTab	m_smart_def_2244lt;
};