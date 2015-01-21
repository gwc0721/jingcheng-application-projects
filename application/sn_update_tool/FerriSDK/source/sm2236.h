#pragma once
#include "ferri_comm.h"

//#define CHS_START_ADDR_SM2236      0xA0E
//#define MODEL_START_ADDR_SM2236    0xA36
//#define SN_START_ADDR_SM2236       0xA14
//#define VENDOR_START_ADDR_SM2236   0xB02

#define	ISP_VER_OFFSET_SM2236		0x0D
#define IDTAB_OFFSET_SM2236			0xA00
//2244LT CID offset
#define CID_CAP_SM2236				0x98C
#define CID_UDMA_SM2236				0x999
#define CID_CHS_HEADER_SM2236		0x996
#define CID_CHS_SECTOR_SM2236		0x995

class CIspBufSM2236 : public CIspBufferComm
{
public:
	CIspBufSM2236(JCSIZE reserved) : CIspBufferComm(reserved) {}
	virtual ~CIspBufSM2236(void) {}

public:
	virtual void SetCapacity(UINT cap)
	{
		JCASSERT(m_buf);
		m_buf[CID_CAP_SM2236] = HIBYTE(HIWORD(cap));
		m_buf[CID_CAP_SM2236+1] = LOBYTE(HIWORD(cap));
		m_buf[CID_CAP_SM2236+2] = HIBYTE(LOWORD(cap));
		m_buf[CID_CAP_SM2236+3] = LOBYTE(LOWORD(cap));
	}

	virtual void SetSerianNumber(LPCTSTR sn, JCSIZE buf_len)
	{
		if (buf_len > MODEL_LENGTH) buf_len = MODEL_LENGTH;
		FillConvertString(sn, m_buf + IDTAB_IN_ISP(IDTAB_OFFSET_SM2236, IDTAB_SN), buf_len);
	}

	virtual void SetVendorSpecific(LPVOID vendor, JCSIZE buf_len)
	{
		BYTE * _vnd = (BYTE*)(vendor);
		if (buf_len > VENDOR_LENGTH) buf_len = VENDOR_LENGTH;
		JCSIZE vendor_start = IDTAB_IN_ISP(IDTAB_OFFSET_SM2236, IDTAB_VENDOR_SPEC);
		for(JCSIZE ii = 0; ii < buf_len; ii += 2)
		{
			m_buf[vendor_start+ii]= _vnd[ii+1];
			m_buf[vendor_start+ii+1]= _vnd[ii];
		}	
	}

	virtual void SetModelName(LPCTSTR model, JCSIZE buf_len)
	{
		if (buf_len > MODEL_LENGTH) buf_len = MODEL_LENGTH;
		FillConvertString(model, m_buf + IDTAB_IN_ISP(IDTAB_OFFSET_SM2236, IDTAB_MODEL_NAME), buf_len);
	}

	virtual bool GetIspVersion(LPVOID buf, JCSIZE buf_len)
	{
		if (buf_len > ISP_VER_LENGTH) buf_len = ISP_VER_LENGTH;
		memcpy_s(buf, buf_len, m_buf + ISP_VER_OFFSET_SM2236, buf_len);
		return true;
	}
	virtual bool SetConfig(UINT prop_id, UINT val);
};

class CFerriSM2236 : public CFerriComm
{
public:
	CFerriSM2236(SMI_API_SET * apis, TCHAR drive_letter, LPCTSTR drive_name, SMI_TESTER_TYPE tester, UINT port, HANDLE dev);
	virtual ~CFerriSM2236(void) {};

public:
	virtual bool CreateIspBuf(JCSIZE data_len, IIspBuffer * & isp);
	virtual bool CheckFlashId(LPVOID fid_ref, JCSIZE len);
};