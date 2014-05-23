#pragma once

#define FILE_MODEL_LIST		_T("model_list.txt")

#define MODLE_LIST_MODELS	_T("MODELS")
#define KEY_COUNT			_T("count")
#define KEY_MODEL_NAME		_T("name")
#define KEY_CONFIG_FILE		_T("config_file")
#define KEY_PREFIX			_T("PREFIX")
// run external process after succeeded update sn
#define SEC_EXTPROC			_T("EXTERNAL")
#define KEY_EXT_PATH		_T("PATH")
#define KEY_EXT_CMD			_T("COMMAND")
#define KEY_EXT_PARAM		_T("PARAMETER")
#define KEY_EXT_SHOW		_T("SHOW_WINDOW")
#define KEY_EXT_TIMEOUT		_T("TIME_OUT")

#define SEC_CONFIGURE		_T("CONFIGURE")
#define SEC_SOURCE			_T("SOURCE")
#define KEY_OEM_MODEL_NAME	_T("ModelName")
#define	KEY_VENDOR			_T("VENDOR")
#define KEY_CAPACITY		_T("Capacity(dec)")
#define KEY_IF_SETTING		_T("I/F Setting")
#define KEY_TRIM			_T("TRIM")
#define KEY_DEVSLP			_T("DEVSLP")
#define	KEY_MPISP			_T("MPISP")
#define	KEY_ISP				_T("ISP")
#define	KEY_FLASHID			_T("FLASHID")
#define KEY_DIS_ISP_CHECK	_T("DisableISPcheckversion")
// checksum for isp file
#define KEY_ISP_CHECKSUM	_T("ISPchecksum")
#define KEY_SN_PREFIX		_T("SNPREFIX")
// checksum for mpisp file
#define KEY_MPISP_CHECKSUM	_T("MPISP_CHECKSUM")
// verify firmware version in ID table
#define KEY_FW_VER			_T("FW_VER")
#define KEY_FLASHID_CHECKSUM	_T("FLASHID_CHECKSUM")

#define SEC_PARAMETER		_T("PARAMETER")

#define KEY_COUNT_CURRENT	_T("CurrentCount")
#define FILE_COUNT_CURRENT	_T("Setting\\CountCur.dat")

#define KEY_COUNT_LIMIT		_T("LimitCount")
#define FILE_COUNT_LIMIT	_T("Setting\\CountLim.dat")

#define KEY_DEVICE_NAME		_T("DeviceName")
#define FILE_DEVICE_NAME	_T("Setting\\DeviceName.dat")

#define	KEY_TEST_MACHINE	_T("TestMachineNo")
#define FILE_TEST_MACHINE_NO	_T("Setting\\TestMachineNo.dat")

#define KEY_PATA_SATA		_T("PATA_SATA")
#define KEY_DEVICE_TYPE		_T("DeviceType")
#define FILE_DEVICE_TYPE	_T("Setting\\DeviceType.dat")

#define FILE_LOG1_PATH		_T("Setting\\SaveLOG1Path.dat")
#define FILE_LOG2_PATH		_T("Setting\\SaveLOG2Path.dat")

#define ISP_BACKUP_PATH		_T("Firmware\\LogISP\\")
#define ISP_BACKUP_FILE_W	_T("ISPwrite%d.bin")
#define ISP_BACKUP_FILE_R	_T("ISPread%d.bin")

#define TIME_OUT			60		


#define LOG_TEST_PROCESS	"PKG_TEST"
#define LOG_TEST_MODE		"Test"


#define MPISP_LENGTH		(64 * 1024)
#define	ISP_LENGTH			(384 * 1024)
#define ISP_VER_LENGTH		10
#define	ISP_VER_OFFSET		0x0D

#define MAX_STRING_LEN		256

#define RETRY_TIMES			3

#ifndef __EXPORTTESTDEMOIGNORE__
#define LEXAR					 0
#define SNSCHECK				 0
#define NETWORKKEY				 0
#define ENGLISHVERSION			 1
#endif

//2244LT identify offset
#define CHS_START_ADDR_2244LT      0xA0E
#define SN_START_ADDR_2244LT       0xA14
#define MODEL_START_ADDR_2244LT    0xA36
#define VENDOR_START_ADDR_2244LT   0xB02

//2244LT CID offset
#define IF_ADDR_2244LT       0x99F
#define TRIM_ADDR_2244LT     0x9A5
#define DEVSLP_ADDR_2244LT   0x9A5

//2244LT Flash ID in flash id command
#define FLASH_ID_OFFSET     0x40
#define FLASH_ID_SIZE       0x80
#define SECTOR_SIZE			512


#define SN_LENGTH_WORD		10
#define	SN_LENGTH			(SN_LENGTH_WORD * 2)
#define	SN_LENGTH_OEM		16
#define SN_OFFSET_ID		0x14

#define MODEL_LENGTH_WORD	20
#define MODEL_LENGTH		(MODEL_LENGTH_WORD * 2)
//#define MODEL_LENGTH_OEM	15
#define MODEL_OFFSET_ID		0x36

#define	VENDOR_LENGTH_WORD	31
#define VENDOR_LENGTH		(VENDOR_LENGTH_WORD * 2)

#define FWVER_OFFSET		46
#define FWVER_LENGTH		8