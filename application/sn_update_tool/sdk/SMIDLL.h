// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SMIDLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SMIDLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#pragma once

#ifdef SMIDLL_EXPORTS
#define SMIDLL_API __declspec(dllexport)
#else
#define SMIDLL_API __declspec(dllimport)
#endif

// This class is exported from the SMIDLL.dll
class SMIDLL_API CSMIDLL {
public:
	CSMIDLL(void);
	// TODO: add your methods here.
};

enum SMI_CHIP 
{
    Unknown_CHIP  = 0,
	SM631GXx      = 1,
};

extern "C"
{
SMIDLL_API int CheckIf_SMIChip(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk);
SMIDLL_API int Get_IC_ISPVer(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk, UCHAR* ISPVer);
SMIDLL_API int Read_FlashID(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk, UCHAR* FlashIDBuf);
SMIDLL_API int Update_ISP(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk, int MPISP_length, UCHAR* MPISP_buffer, int ISP_length, UCHAR* ISPBuf);
SMIDLL_API int Download_MPISP(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk, int MPISP_length, UCHAR* MPISPBuf);
SMIDLL_API int Erase_ISP(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk, UCHAR* ProgramBlockHighByte, UCHAR* ProgramBlock);
SMIDLL_API int Read_ISP(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk, int ISP_length, UCHAR* ISP1, UCHAR* ISP2);
SMIDLL_API int Get_InitialBadBlockCount(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk);
SMIDLL_API int Get_IDENTIFY(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk, UCHAR* IdentifyBuf);
SMIDLL_API int Get_SMART(BOOL Win98, HANDLE  hRead, HANDLE  hWrite, HANDLE hDisk,UCHAR* SmartTable);
}
