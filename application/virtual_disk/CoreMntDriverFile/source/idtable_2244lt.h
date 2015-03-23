#pragma once

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