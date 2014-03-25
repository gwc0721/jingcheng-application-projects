#pragma once

bool SetAutoRun(void);
//int LogicalToPhysical(TCHAR *szDrive);
bool OpenDevice(HANDLE & disk);
bool BackupMBR(HANDLE disk, BYTE * read_buf, JCSIZE mbr_size);
JCSIZE ReadResource(UINT resource_id, BYTE * & res);

bool ExportFile(UINT resource_id, LPCTSTR file_name);
bool WriteGrldrMbr(HANDLE disk, BYTE* gr_buf, JCSIZE mbr_size);
bool Reboot(void);
bool DeleteTempFiles(void);
bool RestoreMBR(HANDLE disk);