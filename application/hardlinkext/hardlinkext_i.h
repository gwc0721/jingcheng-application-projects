

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Wed Jun 04 17:00:26 2014
 */
/* Compiler settings for .\hardlinkext.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __hardlinkext_i_h__
#define __hardlinkext_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __HardLinkOverlyaIcon_FWD_DEFINED__
#define __HardLinkOverlyaIcon_FWD_DEFINED__

#ifdef __cplusplus
typedef class HardLinkOverlyaIcon HardLinkOverlyaIcon;
#else
typedef struct HardLinkOverlyaIcon HardLinkOverlyaIcon;
#endif /* __cplusplus */

#endif 	/* __HardLinkOverlyaIcon_FWD_DEFINED__ */


#ifndef __HardLinkList_FWD_DEFINED__
#define __HardLinkList_FWD_DEFINED__

#ifdef __cplusplus
typedef class HardLinkList HardLinkList;
#else
typedef struct HardLinkList HardLinkList;
#endif /* __cplusplus */

#endif 	/* __HardLinkList_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "Shobjidl.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __hardlinkextLib_LIBRARY_DEFINED__
#define __hardlinkextLib_LIBRARY_DEFINED__

/* library hardlinkextLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_hardlinkextLib;

EXTERN_C const CLSID CLSID_HardLinkOverlyaIcon;

#ifdef __cplusplus

class DECLSPEC_UUID("95C1E3BB-99D3-4A21-BFD3-1034F0F50C49")
HardLinkOverlyaIcon;
#endif

EXTERN_C const CLSID CLSID_HardLinkList;

#ifdef __cplusplus

class DECLSPEC_UUID("A5332B1C-AB0B-43E0-83B7-9201D0EFFFB8")
HardLinkList;
#endif
#endif /* __hardlinkextLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


