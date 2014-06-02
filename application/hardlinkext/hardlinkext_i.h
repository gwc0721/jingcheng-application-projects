

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Fri May 30 19:19:11 2014
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

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __hardlinkext_i_h__
#define __hardlinkext_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IHardLinkOverlyaIcon_FWD_DEFINED__
#define __IHardLinkOverlyaIcon_FWD_DEFINED__
typedef interface IHardLinkOverlyaIcon IHardLinkOverlyaIcon;
#endif 	/* __IHardLinkOverlyaIcon_FWD_DEFINED__ */


#ifndef __HardLinkOverlyaIcon_FWD_DEFINED__
#define __HardLinkOverlyaIcon_FWD_DEFINED__

#ifdef __cplusplus
typedef class HardLinkOverlyaIcon HardLinkOverlyaIcon;
#else
typedef struct HardLinkOverlyaIcon HardLinkOverlyaIcon;
#endif /* __cplusplus */

#endif 	/* __HardLinkOverlyaIcon_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IHardLinkOverlyaIcon_INTERFACE_DEFINED__
#define __IHardLinkOverlyaIcon_INTERFACE_DEFINED__

/* interface IHardLinkOverlyaIcon */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IHardLinkOverlyaIcon;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CFC6FF0C-76EA-4384-9465-84F695D6DA23")
    IHardLinkOverlyaIcon : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IHardLinkOverlyaIconVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHardLinkOverlyaIcon * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHardLinkOverlyaIcon * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHardLinkOverlyaIcon * This);
        
        END_INTERFACE
    } IHardLinkOverlyaIconVtbl;

    interface IHardLinkOverlyaIcon
    {
        CONST_VTBL struct IHardLinkOverlyaIconVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHardLinkOverlyaIcon_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHardLinkOverlyaIcon_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHardLinkOverlyaIcon_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHardLinkOverlyaIcon_INTERFACE_DEFINED__ */



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
#endif /* __hardlinkextLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif

