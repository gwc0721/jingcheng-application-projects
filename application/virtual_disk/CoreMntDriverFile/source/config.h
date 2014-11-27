#pragma once

// vendor command entry read 0x55AAŽù—v•Ô‰ñ“I“à—e
#define FN_VENDOR_BIN	_T("vendor_signe.bin")
#define FN_FID_BIN		_T("fid.bin")
#define FN_SFR_BIN		_T("sfr.bin")
#define FN_PAR_BIN		_T("par.bin")
#define FN_IDTAB_BIN	_T("idtable.bin")
#define FN_DEVINFO_BIN	_T("device_info.bin")
#define FN_ISP_BIN		_T("isp.bin")
#define FN_INFO_BIN		_T("info.bin")
#define FN_BOOTISP_BIN	_T("bootisp.bin")
#define FN_ORGBAD_BIN	_T("org_bad.bin")

#define FN_MPISP_LOG	_T("log\\mpisp.bin")
#define FN_PRETEST_LOG	_T("log\\pretest.bin")
#define FN_BOOTISP_LOG	_T("log\\bootisp.bin")

#define MAX_ISP_SEC		1024
#define BOOTISP_SIZE	2

#define MAX_STRING_LEN	256

#define SEG_SYSTEM_INFO		_T("system_info")
#define FIELD_DATA_FOLDER	_T("data_folder")
#define FIELD_MAX_ISP_LEN	_T("max_isp_len")	

#define SEG_STORAGE			_T("storage")
#define FIELD_STORAGE_FILE	_T("storage_file")
#define FIELD_TOTAL_SEC		_T("total_sectors")
