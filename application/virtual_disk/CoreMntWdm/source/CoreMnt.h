#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <ntddk.h>
#include <wdm.h>
#ifdef __cplusplus
}
#endif 

#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")

#define SECTOR_SIZE		512
#define SECTOR_TO_BYTE(sec)		( (sec) << 9)

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))


