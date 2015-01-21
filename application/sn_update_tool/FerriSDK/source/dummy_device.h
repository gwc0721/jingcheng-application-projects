#pragma once

#include "ferri_comm.h"

class CDummyIspBuffer : public CIspBufferComm
{
public:
	CDummyIspBuffer(void) : CIspBufferComm(MAX_ISP_LENGHT) {};
	virtual ~CDummyIspBuffer(void) {};
public:
	virtual void SetCapacity(UINT cap) {};
	virtual bool SetConfig(UINT prop_id, UINT val) {return true;};
	virtual void SetSerianNumber(LPCTSTR sn, JCSIZE buf_len) {};
	virtual void SetVendorSpecific(LPVOID buf, JCSIZE buf_len) {};
	virtual void SetModelName(LPCTSTR model, JCSIZE buf_len) {};

	virtual bool GetIspVersion(LPVOID buf, JCSIZE buf_len)
	{
		if (buf_len > ISP_VER_LENGTH) buf_len = ISP_VER_LENGTH;
		memcpy_s(buf, buf_len, m_buf + 0x0D, buf_len);
		return true;
	}
};

class CDummyDevice : public IFerriDevice
{
public:
	CDummyDevice(TCHAR letter, LPCTSTR name, UINT port);
	virtual ~CDummyDevice(void);
	IMPLEMENT_INTERFACE;

public:
	virtual bool CheckIsFerriChip(void) {return true;};
	virtual bool GetIspVersion(UCHAR * isp_ver, JCSIZE buf_len) 
	{	
		_stprintf_s(isp_ver, buf_len, _T("DUMMY"));
		return true;
	}
	virtual bool ReadFlashId(LPVOID buf, JCSIZE buf_len)	{return true;}
	virtual bool DownloadIsp(IIspBuffer * isp) {return true;}
	virtual bool ReadIsp(JCSIZE data_len, IIspBuffer * & isp1, IIspBuffer * & isp2)
	{
	}
	virtual bool CreateIspBuf(JCSIZE data_len, IIspBuffer * & isp)
	{
		isp = static_cast<IIspBuffer*>(new CDummyIspBuffer);
		return true;
	}

	virtual UINT GetInitialiBadBlockCount(void)	{return 0;}
	virtual bool ReadIdentify(LPVOID buf, JCSIZE buf_len) {return true;}
	virtual bool ReadSmart(LPVOID buf, JCSIZE buf_len) {return true;}
	virtual bool CheckFlashId(LPVOID fid, JCSIZE len) {return true;}

	virtual bool SetMpisp(LPVOID mpisp, JCSIZE mpisp_len) {return true;}
	virtual bool ResetTester(void) {return true;}

	//--
	virtual TCHAR GetDriveLetter(void) {return _T('X');}
	virtual void GetDriveName(CJCStringT & str) { str = _T("\\\\.\\X:");}
	virtual UINT GetTesterPort(void) {return 1;}
};
