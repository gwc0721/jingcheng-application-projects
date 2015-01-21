#include "stdafx.h"

#include "sm2244lt.h"

LOCAL_LOGGER_ENABLE(_T("ferri_2244"), LOGGER_LEVEL_NOTICE);
	
bool CIspBufLT2244::SetConfig(UINT config_id, UINT val)
{
	switch (config_id)
	{
	case FCONFIG_SATA_SPEED:
		m_buf[IF_ADDR_2244LT] &= 0xEF;
		if(val == 1)		m_buf[IF_ADDR_2244LT] |= 0x10;
		break;

	case FCONFIG_TRIM:
		m_buf[TRIM_ADDR_2244LT] &= 0xF7;	// not support
		if( val )		m_buf[TRIM_ADDR_2244LT] |= 0x08;
		break;

	case FCONFIG_DEVSLP:
		m_buf[DEVSLP_ADDR_2244LT] &= 0xEF;
		if ( val )	m_buf[DEVSLP_ADDR_2244LT] |= 0x10;	
		break;

	case FCONFIG_TEMPERATURE:
		m_buf[0x9B0] = 0x0;
		break;
	default:
		return false;	// not support
	}
	return true;
}

CFerriLT2244::CFerriLT2244(SMI_API_SET * apis, TCHAR drive_letter, LPCTSTR drive_name, SMI_TESTER_TYPE tester, UINT port, HANDLE dev)
: CFerriComm(apis, drive_letter, drive_name, tester, port, dev)
{
}

bool CFerriLT2244::CreateIspBuf(JCSIZE data_len, IIspBuffer * & isp)
{
	JCASSERT(NULL == isp);
	CIspBufLT2244 * _isp = new CIspBufLT2244(MAX_ISP_LENGTH);
	_isp->SetDataLen(data_len);
	isp = static_cast<IIspBuffer*>(_isp);
	JCASSERT(isp);
	return true;
}

bool CFerriLT2244::CheckFlashId(LPVOID _fid_ref, JCSIZE len)
{
	LOG_STACK_TRACE();
	BYTE * fid_ref = (BYTE*)(_fid_ref);

	bool success = true;
	stdext::auto_array<BYTE> flash_id_buf(SECTOR_SIZE);
	memset(flash_id_buf, 0, SECTOR_SIZE);

	success = ReadFlashId(flash_id_buf, SECTOR_SIZE);
	if (!success) return false;
	if (0 != memcmp(flash_id_buf+FLASH_ID_OFFSET, fid_ref + FLASH_ID_OFFSET, FLASH_ID_SIZE) )
	{
		LOG_ERROR(_T("flash id do not match"));
		return false;
	}
	return true;
}
