#pragma once

bool SetAutoRun(void);
bool RemoveAutoRun(void);

//int LogicalToPhysical(TCHAR *szDrive);
bool OpenDevice(HANDLE & disk, LPCTSTR drive);
bool BackupMBR(HANDLE disk, BYTE * read_buf, JCSIZE mbr_size, LPCTSTR file_name);
bool RestoreMBR(HANDLE disk, LPCTSTR file_name);

bool ExportFile(UINT resource_id, LPCTSTR file_name);
bool WriteGrldrMbr(HANDLE disk, BYTE* gr_buf, JCSIZE mbr_size);
bool Reboot(void);

JCSIZE ReadResource(UINT resource_id, BYTE * & res);

