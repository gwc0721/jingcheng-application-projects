#include "stdafx.h"
#include "sm2236.h"

LOCAL_LOGGER_ENABLE(_T("ferri_2244"), LOGGER_LEVEL_NOTICE);
	
bool CIspBufSM2236::SetConfig(UINT config_id, UINT val)
{
	switch (config_id)
	{
	case FCONFIG_SATA_SPEED:	return false;

	case FCONFIG_TRIM:
		//m_buf[TRIM_ADDR_SM2236] &= 0xF7;	// not support
		//if( val )		m_buf[TRIM_ADDR_SM2236] |= 0x08;
		break;

	case FCONFIG_DEVSLP:
		//m_buf[DEVSLP_ADDR_SM2236] &= 0xEF;
		//if ( val )	m_buf[DEVSLP_ADDR_SM2236] |= 0x10;	
		break;

	case FCONFIG_TEMPERATURE:
		//m_buf[0x9B0] = 0x0;
		break;

	case FCONFIG_CYLINDERS:	break;

	case FCONFIG_HEDERS:
		m_buf[CID_CHS_HEADER_SM2236] = val;
		break;

	case FCONFIG_SECTORS:
		m_buf[CID_CHS_SECTOR_SM2236] = val;
		break;

	case FCONFIG_UDMA_LEVEL:
		m_buf[CID_UDMA_SM2236] &= 0xF8;
		m_buf[CID_UDMA_SM2236] |= (val & 0x7);
		break;

	default:
		return false;	// not support
	}
	return true;
}

CFerriSM2236::CFerriSM2236(SMI_API_SET * apis, TCHAR drive_letter, LPCTSTR drive_name, SMI_TESTER_TYPE tester, UINT port, HANDLE dev)
: CFerriComm(apis, drive_letter, drive_name, tester, port, dev)
{
}

bool CFerriSM2236::CreateIspBuf(JCSIZE data_len, IIspBuffer * & isp)
{
	JCASSERT(NULL == isp);
	CIspBufSM2236 * _isp = new CIspBufSM2236(MAX_ISP_LENGTH);
	_isp->SetDataLen(data_len);
	isp = static_cast<IIspBuffer*>(_isp);
	JCASSERT(isp);
	return true;
}

bool CFerriSM2236::CheckFlashId(LPVOID _fid_ref, JCSIZE len)
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
