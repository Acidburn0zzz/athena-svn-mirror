/*
 * This file was generated by orbit-idl-2 - DO NOT EDIT!
 */

#ifndef plugin_H
#define plugin_H 1
#include <glib.h>
#define ORBIT_IDL_SERIAL 19
#include <orbit/orbit-types.h>

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/** typedefs **/
#if !defined(ORBIT_DECL_Plugin) && !defined(_Plugin_defined)
#define ORBIT_DECL_Plugin 1
#define _Plugin_defined 1
#define Plugin__freekids CORBA_Object__freekids
   typedef CORBA_Object Plugin;
   extern CORBA_unsigned_long Plugin__classid;
#if !defined(TC_IMPL_TC_Plugin_0)
#define TC_IMPL_TC_Plugin_0 'p'
#define TC_IMPL_TC_Plugin_1 'l'
#define TC_IMPL_TC_Plugin_2 'u'
#define TC_IMPL_TC_Plugin_3 'g'
#define TC_IMPL_TC_Plugin_4 'i'
#define TC_IMPL_TC_Plugin_5 'n'
#ifdef ORBIT_IDL_C_IMODULE
   static
#else
   extern
#endif
   const struct CORBA_TypeCode_struct TC_Plugin_struct;
#define TC_Plugin ((CORBA_TypeCode)&TC_Plugin_struct)
#endif
#endif

/** POA structures **/
#ifndef _defined_POA_Plugin
#define _defined_POA_Plugin 1
   typedef struct
   {
      void *_private;
      void (*doPluginTest) (PortableServer_Servant _servant,
			    CORBA_Environment * ev);
   }
   POA_Plugin__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Plugin__epv *Plugin_epv;
   }
   POA_Plugin__vepv;
   typedef struct
   {
      void *_private;
      POA_Plugin__vepv *vepv;
   }
   POA_Plugin;
   extern void POA_Plugin__init(PortableServer_Servant servant,
				CORBA_Environment * ev);
   extern void POA_Plugin__fini(PortableServer_Servant servant,
				CORBA_Environment * ev);
#endif				/* _defined_POA_Plugin */

/** skel prototypes **/
   void _ORBIT_skel_small_Plugin_doPluginTest(POA_Plugin * _ORBIT_servant,
					      gpointer _ORBIT_retval,
					      gpointer * _ORBIT_args,
					      CORBA_Context ctx,
					      CORBA_Environment * ev,
					      void (*_impl_doPluginTest)
					      (PortableServer_Servant
					       _servant,
					       CORBA_Environment * ev));

/** stub prototypes **/
   void Plugin_doPluginTest(Plugin _obj, CORBA_Environment * ev);

/** more internals **/
#include <orbit/orb-core/orbit-interface.h>

#ifdef ORBIT_IDL_C_IMODULE
   static
#else
   extern
#endif
   ORBit_IInterface Plugin__iinterface;
#define Plugin_IMETHODS_LEN 1
#ifdef ORBIT_IDL_C_IMODULE
   static
#else
   extern
#endif
   ORBit_IMethod Plugin__imethods[Plugin_IMETHODS_LEN];
#ifdef __cplusplus
}
#endif				/* __cplusplus */

#ifndef EXCLUDE_ORBIT_H
#include <orbit/orbit.h>

#endif				/* EXCLUDE_ORBIT_H */
#endif
#undef ORBIT_IDL_SERIAL
