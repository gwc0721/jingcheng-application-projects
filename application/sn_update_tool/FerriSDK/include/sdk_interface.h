#pragma once

#include <stdext.h>

enum FERRI_ERROR_CODE
{
	FERR_OK = 0,
	FERR_NO_TESTER = 1,
	FERR_NO_CARD = 2,
	FERR_OPEN_DRIVE_FAIL = 3,
	FERR_CONTROLLER_NOT_MATCH = 4,
	FERR_OPEN_ISP_FAIL = 5,
	FERR_DEVICE_NOT_CONNECT = 6,
};

enum FERRI_CONFIGURATION
{
	FCONFIG_SATA_SPEED	=1,
	FCONFIG_TRIM		=2,
	FCONFIG_DEVSLP		=3,
	FCONFIG_TEMPERATURE	=4,
	FCONFIG_CYLINDERS	=5,
	FCONFIG_HEDERS		=6,
	FCONFIG_SECTORS		=7,
	FCONFIG_UDMA_LEVEL	=8,
};

class IIspBuffer : public IJCInterface
{
public:
	virtual void SetCapacity(UINT cap) = 0;
	virtual bool SetConfig(UINT prop_id, UINT val) = 0;
	virtual void SetSerianNumber(LPCTSTR sn, JCSIZE buf_len) = 0;
	virtual void SetVendorSpecific(LPVOID buf, JCSIZE buf_len) = 0;
	virtual void SetModelName(LPCTSTR model, JCSIZE buf_len) = 0;

	//virtual const LPVOID GetBuffer(void) const;
	virtual LPVOID GetBuffer(void) = 0;
	virtual void ReleaseBuffer(void) = 0;

	virtual bool LoadFromFile(LPCTSTR fn) = 0;
	virtual bool SaveToFile(LPCTSTR fn) = 0;
	virtual DWORD CheckSum(void) = 0;
	virtual bool GetIspVersion(LPVOID buf, JCSIZE buf_len) = 0;
	virtual bool Compare(IIspBuffer * isp) const = 0;
	virtual JCSIZE GetSize(void) const = 0;
};

class IFerriDevice : public IJCInterface
{
public:
	virtual bool CheckIsFerriChip(void) = 0;
	virtual bool GetIspVersion(UCHAR * isp_ver, JCSIZE buf_len) = 0;
	virtual bool ReadFlashId(LPVOID buf, JCSIZE buf_len) = 0;
	virtual bool DownloadIsp(IIspBuffer * isp) = 0;
	virtual bool ReadIsp(JCSIZE data_len, IIspBuffer * & isp1, IIspBuffer * & isp2) = 0;
	virtual bool CreateIspBuf(JCSIZE data_len, IIspBuffer * & isp) = 0;

	virtual UINT GetInitialiBadBlockCount(void) = 0;
	virtual bool ReadIdentify(LPVOID buf, JCSIZE buf_len) = 0;
	virtual bool ReadSmart(LPVOID buf, JCSIZE buf_len) = 0;
	virtual bool CheckFlashId(LPVOID fid, JCSIZE len) = 0;

	virtual bool SetMpisp(LPVOID mpisp, JCSIZE mpisp_len) = 0;
	// power_on == true: power on, power_on == false: power off
	virtual bool PowerOnOff(bool power_on) = 0;
	virtual void Disconnect(void) = 0;
	virtual bool Connect(void) = 0;

	//--
	virtual TCHAR GetDriveLetter(void) = 0;
	virtual void GetDriveName(CJCStringT & str) = 0;
	virtual UINT GetTesterPort(void) = 0;
};

class IFerriFactory : public IJCInterface
{
public:
	virtual bool CreateDevice(HANDLE dev, IFerriDevice * & ferri) = 0;
	virtual bool ScanDevice(IFerriDevice * & ferri) = 0;
	virtual bool CreateDummyDevice(IFerriDevice * & ferri) = 0;
};

extern "C"
{
	bool InitializeSdk(LPCTSTR ctrl_name, IFerriFactory * & factory);
}


#define MODEL_LENGTH_WORD	20
#define MODEL_LENGTH		(MODEL_LENGTH_WORD * 2)

#define SN_LENGTH_WORD		10
#define	SN_LENGTH			(SN_LENGTH_WORD * 2)

#define	VENDOR_LENGTH_WORD	31
#define VENDOR_LENGTH		(VENDOR_LENGTH_WORD * 2)

#define ISP_VER_LENGTH		10

#define IDTAB_CYLINDERS_1	1
#define IDTAB_HEADS_1		3
#define IDTAB_SECTORS_1		6
#define IDTAB_SN			10
#define IDTAB_MODEL_NAME	27
#define IDTAB_CYLINDERS_2	54
#define IDTAB_HEADS_2		55
#define IDTAB_SECTORS_2		56
#define IDTAB_UDMA			88
#define IDTAB_VENDOR_SPEC	129
