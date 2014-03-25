#pragma once

#define BUF_SIZE		256
#define RETRY_TIMES		3
#define MBR_SIZE		(9 * 1024)
#define MAX_MBR_SIZE	(100 * 1024)
#define READ_CMD        0x28
#define WRITE_CMD       0x2A
#define MBR_BACKUP		_T("C:\\BackUp.bin")
#define FN_DOS_IMAGE	_T("C:\\Dos.iso")
#define FN_GRLDR		_T("C:\\grldr")
#define FN_MENU_LST		_T("C:\\menu.lst")
#define FN_GRLDR_MBR	_T("C:\\grldr.mbr")
#define KEY_AUTORUN		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define VAL_NAME_AUTORUN	_T("UpdateFW")
#define	PARTITION_TAB	0x1BE
#define PARTITION_TAB_LEN	0x40