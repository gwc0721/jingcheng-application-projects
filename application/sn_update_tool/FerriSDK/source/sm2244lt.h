#pragma once

#include "ferri_comm.h"

#define CHS_START_ADDR_2244LT      0xA0E
#define MODEL_START_ADDR_2244LT    0xA36
#define SN_START_ADDR_2244LT       0xA14
#define	ISP_VER_OFFSET_2244LT		0x0D

//2244LT identify offset
#define VENDOR_START_ADDR_2244LT   0xB02

//2244LT CID offset
#define IF_ADDR_2244LT       0x99F
#define TRIM_ADDR_2244LT     0x9A5
#define DEVSLP_ADDR_2244LT   0x9A5


class CIspBufLT2244 : public CIspBufferComm
{
public:
	CIspBufLT2244(JCSIZE reserved) : CIspBufferComm(reserved) {}
	virtual ~CIspBufLT2244(void) {}

public:
	virtual void SetCapacity(UINT cap)
	{
		JCASSERT(m_buf);
		m_buf[CHS_START_ADDR_2244LT] = (UCHAR)((cap & 0xFF0000) >> 16);
		m_buf[CHS_START_ADDR_2244LT+1] = (UCHAR)((cap & 0xFF000000) >> 24);
		m_buf[CHS_START_ADDR_2244LT+2] = (UCHAR)(cap & 0xFF);
		m_buf[CHS_START_ADDR_2244LT+3] = (UCHAR)((cap & 0xFF00) >> 8);
	}

	virtual void SetSerianNumber(LPCTSTR sn, JCSIZE buf_len)
	{
		if (buf_len > MODEL_LENGTH) buf_len = MODEL_LENGTH;
		FillConvertString(sn, m_buf + SN_START_ADDR_2244LT, buf_len);
	}

	virtual void SetVendorSpecific(LPVOID vendor, JCSIZE buf_len)
	{
		BYTE * _vnd = (BYTE*)(vendor);
		if (buf_len > VENDOR_LENGTH) buf_len = VENDOR_LENGTH;
		for(JCSIZE ii = 0; ii < buf_len; ii += 2)
		{
			m_buf[VENDOR_START_ADDR_2244LT+ii]= _vnd[ii+1];
			m_buf[VENDOR_START_ADDR_2244LT+ii+1]= _vnd[ii];
		}	
	}

	virtual void SetModelName(LPCTSTR model, JCSIZE buf_len)
	{
		if (buf_len > MODEL_LENGTH) buf_len = MODEL_LENGTH;
		FillConvertString(model, m_buf + MODEL_START_ADDR_2244LT, buf_len);
	}

	virtual bool GetIspVersion(LPVOID buf, JCSIZE buf_len)
	{
		if (buf_len > ISP_VER_LENGTH) buf_len = ISP_VER_LENGTH;
		memcpy_s(buf, buf_len, m_buf + ISP_VER_OFFSET_2244LT, buf_len);
		return true;
	}
	virtual bool SetConfig(UINT prop_id, UINT val);
};

class CFerriLT2244 : public CFerriComm
{
public:
	CFerriLT2244(SMI_API_SET * apis, TCHAR drive_letter, LPCTSTR drive_name, SMI_TESTER_TYPE tester, UINT port, HANDLE dev);
	virtual ~CFerriLT2244(void) {};

public:
	virtual bool CreateIspBuf(JCSIZE data_len, IIspBuffer * & isp);
	virtual bool CheckFlashId(LPVOID fid, JCSIZE len);

};