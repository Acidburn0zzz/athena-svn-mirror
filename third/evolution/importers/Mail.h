/*
 * This file was generated by orbit-idl - DO NOT EDIT!
 */

#include <glib.h>
#define ORBIT_IDL_SERIAL 9
#include <orb/orbit.h>

#ifndef Mail_H
#define Mail_H 1
#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/** typedefs **/
#include <bonobo/Bonobo.h>
#if !defined(ORBIT_DECL_GNOME_Evolution_MessageList) && !defined(_GNOME_Evolution_MessageList_defined)
#define ORBIT_DECL_GNOME_Evolution_MessageList 1
#define _GNOME_Evolution_MessageList_defined 1
#define GNOME_Evolution_MessageList__free CORBA_Object__free
   typedef CORBA_Object GNOME_Evolution_MessageList;
   extern CORBA_unsigned_long GNOME_Evolution_MessageList__classid;
#if !defined(TC_IMPL_TC_GNOME_Evolution_MessageList_0)
#define TC_IMPL_TC_GNOME_Evolution_MessageList_0 'M'
#define TC_IMPL_TC_GNOME_Evolution_MessageList_1 'a'
#define TC_IMPL_TC_GNOME_Evolution_MessageList_2 'i'
#define TC_IMPL_TC_GNOME_Evolution_MessageList_3 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Evolution_MessageList_struct;
#define TC_GNOME_Evolution_MessageList ((CORBA_TypeCode)&TC_GNOME_Evolution_MessageList_struct)
#endif
#endif
#if !defined(ORBIT_DECL_GNOME_Evolution_FolderBrowser) && !defined(_GNOME_Evolution_FolderBrowser_defined)
#define ORBIT_DECL_GNOME_Evolution_FolderBrowser 1
#define _GNOME_Evolution_FolderBrowser_defined 1
#define GNOME_Evolution_FolderBrowser__free CORBA_Object__free
   typedef CORBA_Object GNOME_Evolution_FolderBrowser;
   extern CORBA_unsigned_long GNOME_Evolution_FolderBrowser__classid;
#if !defined(TC_IMPL_TC_GNOME_Evolution_FolderBrowser_0)
#define TC_IMPL_TC_GNOME_Evolution_FolderBrowser_0 'M'
#define TC_IMPL_TC_GNOME_Evolution_FolderBrowser_1 'a'
#define TC_IMPL_TC_GNOME_Evolution_FolderBrowser_2 'i'
#define TC_IMPL_TC_GNOME_Evolution_FolderBrowser_3 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Evolution_FolderBrowser_struct;
#define TC_GNOME_Evolution_FolderBrowser ((CORBA_TypeCode)&TC_GNOME_Evolution_FolderBrowser_struct)
#endif
#endif
#if !defined(ORBIT_DECL_GNOME_Evolution_FolderInfo) && !defined(_GNOME_Evolution_FolderInfo_defined)
#define ORBIT_DECL_GNOME_Evolution_FolderInfo 1
#define _GNOME_Evolution_FolderInfo_defined 1
#define GNOME_Evolution_FolderInfo__free CORBA_Object__free
   typedef CORBA_Object GNOME_Evolution_FolderInfo;
   extern CORBA_unsigned_long GNOME_Evolution_FolderInfo__classid;
#if !defined(TC_IMPL_TC_GNOME_Evolution_FolderInfo_0)
#define TC_IMPL_TC_GNOME_Evolution_FolderInfo_0 'M'
#define TC_IMPL_TC_GNOME_Evolution_FolderInfo_1 'a'
#define TC_IMPL_TC_GNOME_Evolution_FolderInfo_2 'i'
#define TC_IMPL_TC_GNOME_Evolution_FolderInfo_3 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Evolution_FolderInfo_struct;
#define TC_GNOME_Evolution_FolderInfo ((CORBA_TypeCode)&TC_GNOME_Evolution_FolderInfo_struct)
#endif
#endif
#if !defined(_GNOME_Evolution_FolderInfo_MessageCount_defined)
#define _GNOME_Evolution_FolderInfo_MessageCount_defined 1
   typedef struct
   {
      CORBA_char *path;
      CORBA_long count;
      CORBA_long unread;
   }
   GNOME_Evolution_FolderInfo_MessageCount;

#if !defined(TC_IMPL_TC_GNOME_Evolution_FolderInfo_MessageCount_0)
#define TC_IMPL_TC_GNOME_Evolution_FolderInfo_MessageCount_0 'M'
#define TC_IMPL_TC_GNOME_Evolution_FolderInfo_MessageCount_1 'a'
#define TC_IMPL_TC_GNOME_Evolution_FolderInfo_MessageCount_2 'i'
#define TC_IMPL_TC_GNOME_Evolution_FolderInfo_MessageCount_3 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Evolution_FolderInfo_MessageCount_struct;
#define TC_GNOME_Evolution_FolderInfo_MessageCount ((CORBA_TypeCode)&TC_GNOME_Evolution_FolderInfo_MessageCount_struct)
#endif
   extern GNOME_Evolution_FolderInfo_MessageCount
      *GNOME_Evolution_FolderInfo_MessageCount__alloc(void);
   extern gpointer GNOME_Evolution_FolderInfo_MessageCount__free(gpointer mem,
								 gpointer dat,
								 CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(ORBIT_DECL_GNOME_Evolution_MailConfig) && !defined(_GNOME_Evolution_MailConfig_defined)
#define ORBIT_DECL_GNOME_Evolution_MailConfig 1
#define _GNOME_Evolution_MailConfig_defined 1
#define GNOME_Evolution_MailConfig__free CORBA_Object__free
   typedef CORBA_Object GNOME_Evolution_MailConfig;
   extern CORBA_unsigned_long GNOME_Evolution_MailConfig__classid;
#if !defined(TC_IMPL_TC_GNOME_Evolution_MailConfig_0)
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_0 'M'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_1 'a'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_2 'i'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_3 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Evolution_MailConfig_struct;
#define TC_GNOME_Evolution_MailConfig ((CORBA_TypeCode)&TC_GNOME_Evolution_MailConfig_struct)
#endif
#endif
#if !defined(_GNOME_Evolution_MailConfig_Identity_defined)
#define _GNOME_Evolution_MailConfig_Identity_defined 1
   typedef struct
   {
      CORBA_char *name;
      CORBA_char *address;
      CORBA_char *organization;
      CORBA_char *signature;
      CORBA_char *html_signature;
      CORBA_boolean has_html_signature;
   }
   GNOME_Evolution_MailConfig_Identity;

#if !defined(TC_IMPL_TC_GNOME_Evolution_MailConfig_Identity_0)
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Identity_0 'M'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Identity_1 'a'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Identity_2 'i'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Identity_3 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Evolution_MailConfig_Identity_struct;
#define TC_GNOME_Evolution_MailConfig_Identity ((CORBA_TypeCode)&TC_GNOME_Evolution_MailConfig_Identity_struct)
#endif
   extern GNOME_Evolution_MailConfig_Identity
      *GNOME_Evolution_MailConfig_Identity__alloc(void);
   extern gpointer GNOME_Evolution_MailConfig_Identity__free(gpointer mem,
							     gpointer dat,
							     CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(_GNOME_Evolution_MailConfig_Service_defined)
#define _GNOME_Evolution_MailConfig_Service_defined 1
   typedef struct
   {
      CORBA_char *url;
      CORBA_boolean keep_on_server;
      CORBA_boolean auto_check;
      CORBA_long auto_check_time;
      CORBA_boolean save_passwd;
      CORBA_boolean enabled;
   }
   GNOME_Evolution_MailConfig_Service;

#if !defined(TC_IMPL_TC_GNOME_Evolution_MailConfig_Service_0)
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Service_0 'M'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Service_1 'a'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Service_2 'i'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Service_3 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Evolution_MailConfig_Service_struct;
#define TC_GNOME_Evolution_MailConfig_Service ((CORBA_TypeCode)&TC_GNOME_Evolution_MailConfig_Service_struct)
#endif
   extern GNOME_Evolution_MailConfig_Service
      *GNOME_Evolution_MailConfig_Service__alloc(void);
   extern gpointer GNOME_Evolution_MailConfig_Service__free(gpointer mem,
							    gpointer dat,
							    CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(_GNOME_Evolution_MailConfig_Account_defined)
#define _GNOME_Evolution_MailConfig_Account_defined 1
   typedef struct
   {
      CORBA_char *name;
      GNOME_Evolution_MailConfig_Identity id;
      GNOME_Evolution_MailConfig_Service source;
      GNOME_Evolution_MailConfig_Service transport;
      CORBA_char *drafts_folder_name;
      CORBA_char *drafts_folder_uri;
      CORBA_char *sent_folder_name;
      CORBA_char *sent_folder_uri;
   }
   GNOME_Evolution_MailConfig_Account;

#if !defined(TC_IMPL_TC_GNOME_Evolution_MailConfig_Account_0)
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Account_0 'M'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Account_1 'a'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Account_2 'i'
#define TC_IMPL_TC_GNOME_Evolution_MailConfig_Account_3 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Evolution_MailConfig_Account_struct;
#define TC_GNOME_Evolution_MailConfig_Account ((CORBA_TypeCode)&TC_GNOME_Evolution_MailConfig_Account_struct)
#endif
   extern GNOME_Evolution_MailConfig_Account
      *GNOME_Evolution_MailConfig_Account__alloc(void);
   extern gpointer GNOME_Evolution_MailConfig_Account__free(gpointer mem,
							    gpointer dat,
							    CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(ORBIT_DECL_GNOME_Evolution_MailFilter) && !defined(_GNOME_Evolution_MailFilter_defined)
#define ORBIT_DECL_GNOME_Evolution_MailFilter 1
#define _GNOME_Evolution_MailFilter_defined 1
#define GNOME_Evolution_MailFilter__free CORBA_Object__free
   typedef CORBA_Object GNOME_Evolution_MailFilter;
   extern CORBA_unsigned_long GNOME_Evolution_MailFilter__classid;
#if !defined(TC_IMPL_TC_GNOME_Evolution_MailFilter_0)
#define TC_IMPL_TC_GNOME_Evolution_MailFilter_0 'M'
#define TC_IMPL_TC_GNOME_Evolution_MailFilter_1 'a'
#define TC_IMPL_TC_GNOME_Evolution_MailFilter_2 'i'
#define TC_IMPL_TC_GNOME_Evolution_MailFilter_3 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Evolution_MailFilter_struct;
#define TC_GNOME_Evolution_MailFilter ((CORBA_TypeCode)&TC_GNOME_Evolution_MailFilter_struct)
#endif
#endif

/** POA structures **/
   typedef struct
   {
      void *_private;
      void (*selectMessage) (PortableServer_Servant _servant,
			     const CORBA_long message_number,
			     CORBA_Environment * ev);
      void (*openMessage) (PortableServer_Servant _servant,
			   const CORBA_long message_number,
			   CORBA_Environment * ev);
   }
   POA_GNOME_Evolution_MessageList__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_GNOME_Evolution_MessageList__epv *GNOME_Evolution_MessageList_epv;
   }
   POA_GNOME_Evolution_MessageList__vepv;
   typedef struct
   {
      void *_private;
      POA_GNOME_Evolution_MessageList__vepv *vepv;
   }
   POA_GNOME_Evolution_MessageList;
   extern void POA_GNOME_Evolution_MessageList__init(PortableServer_Servant
						     servant,
						     CORBA_Environment * ev);
   extern void POA_GNOME_Evolution_MessageList__fini(PortableServer_Servant
						     servant,
						     CORBA_Environment * ev);
   typedef struct
   {
      void *_private;
      
	 GNOME_Evolution_MessageList(*getMessageList) (PortableServer_Servant
						       _servant,
						       CORBA_Environment *
						       ev);
   }
   POA_GNOME_Evolution_FolderBrowser__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_GNOME_Evolution_FolderBrowser__epv
	 *GNOME_Evolution_FolderBrowser_epv;
   }
   POA_GNOME_Evolution_FolderBrowser__vepv;
   typedef struct
   {
      void *_private;
      POA_GNOME_Evolution_FolderBrowser__vepv *vepv;
   }
   POA_GNOME_Evolution_FolderBrowser;
   extern void POA_GNOME_Evolution_FolderBrowser__init(PortableServer_Servant
						       servant,
						       CORBA_Environment *
						       ev);
   extern void POA_GNOME_Evolution_FolderBrowser__fini(PortableServer_Servant
						       servant,
						       CORBA_Environment *
						       ev);
   typedef struct
   {
      void *_private;
      void (*getInfo) (PortableServer_Servant _servant,
		       const CORBA_char * foldername,
		       const Bonobo_Listener listener,
		       CORBA_Environment * ev);
   }
   POA_GNOME_Evolution_FolderInfo__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_GNOME_Evolution_FolderInfo__epv *GNOME_Evolution_FolderInfo_epv;
   }
   POA_GNOME_Evolution_FolderInfo__vepv;
   typedef struct
   {
      void *_private;
      POA_GNOME_Evolution_FolderInfo__vepv *vepv;
   }
   POA_GNOME_Evolution_FolderInfo;
   extern void POA_GNOME_Evolution_FolderInfo__init(PortableServer_Servant
						    servant,
						    CORBA_Environment * ev);
   extern void POA_GNOME_Evolution_FolderInfo__fini(PortableServer_Servant
						    servant,
						    CORBA_Environment * ev);
   typedef struct
   {
      void *_private;
      void (*addAccount) (PortableServer_Servant _servant,
			  const GNOME_Evolution_MailConfig_Account * acc,
			  CORBA_Environment * ev);
   }
   POA_GNOME_Evolution_MailConfig__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_GNOME_Evolution_MailConfig__epv *GNOME_Evolution_MailConfig_epv;
   }
   POA_GNOME_Evolution_MailConfig__vepv;
   typedef struct
   {
      void *_private;
      POA_GNOME_Evolution_MailConfig__vepv *vepv;
   }
   POA_GNOME_Evolution_MailConfig;
   extern void POA_GNOME_Evolution_MailConfig__init(PortableServer_Servant
						    servant,
						    CORBA_Environment * ev);
   extern void POA_GNOME_Evolution_MailConfig__fini(PortableServer_Servant
						    servant,
						    CORBA_Environment * ev);
   typedef struct
   {
      void *_private;
      void (*addFilter) (PortableServer_Servant _servant,
			 const CORBA_char * rule, CORBA_Environment * ev);
      void (*removeFilter) (PortableServer_Servant _servant,
			    const CORBA_char * rule, CORBA_Environment * ev);
   }
   POA_GNOME_Evolution_MailFilter__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_GNOME_Evolution_MailFilter__epv *GNOME_Evolution_MailFilter_epv;
   }
   POA_GNOME_Evolution_MailFilter__vepv;
   typedef struct
   {
      void *_private;
      POA_GNOME_Evolution_MailFilter__vepv *vepv;
   }
   POA_GNOME_Evolution_MailFilter;
   extern void POA_GNOME_Evolution_MailFilter__init(PortableServer_Servant
						    servant,
						    CORBA_Environment * ev);
   extern void POA_GNOME_Evolution_MailFilter__fini(PortableServer_Servant
						    servant,
						    CORBA_Environment * ev);

/** prototypes **/
#define GNOME_Evolution_MessageList_ref Bonobo_Unknown_ref
#define GNOME_Evolution_MessageList_unref Bonobo_Unknown_unref
#define GNOME_Evolution_MessageList_queryInterface Bonobo_Unknown_queryInterface
   void GNOME_Evolution_MessageList_selectMessage(GNOME_Evolution_MessageList
						  _obj,
						  const CORBA_long
						  message_number,
						  CORBA_Environment * ev);
   void GNOME_Evolution_MessageList_openMessage(GNOME_Evolution_MessageList
						_obj,
						const CORBA_long
						message_number,
						CORBA_Environment * ev);
#define GNOME_Evolution_FolderBrowser_ref Bonobo_Unknown_ref
#define GNOME_Evolution_FolderBrowser_unref Bonobo_Unknown_unref
#define GNOME_Evolution_FolderBrowser_queryInterface Bonobo_Unknown_queryInterface
   GNOME_Evolution_MessageList
      GNOME_Evolution_FolderBrowser_getMessageList
      (GNOME_Evolution_FolderBrowser _obj, CORBA_Environment * ev);
#define GNOME_Evolution_FolderInfo_ref Bonobo_Unknown_ref
#define GNOME_Evolution_FolderInfo_unref Bonobo_Unknown_unref
#define GNOME_Evolution_FolderInfo_queryInterface Bonobo_Unknown_queryInterface
   void GNOME_Evolution_FolderInfo_getInfo(GNOME_Evolution_FolderInfo _obj,
					   const CORBA_char * foldername,
					   const Bonobo_Listener listener,
					   CORBA_Environment * ev);
#define GNOME_Evolution_MailConfig_ref Bonobo_Unknown_ref
#define GNOME_Evolution_MailConfig_unref Bonobo_Unknown_unref
#define GNOME_Evolution_MailConfig_queryInterface Bonobo_Unknown_queryInterface
   void GNOME_Evolution_MailConfig_addAccount(GNOME_Evolution_MailConfig _obj,
					      const
					      GNOME_Evolution_MailConfig_Account
					      * acc, CORBA_Environment * ev);
#define GNOME_Evolution_MailFilter_ref Bonobo_Unknown_ref
#define GNOME_Evolution_MailFilter_unref Bonobo_Unknown_unref
#define GNOME_Evolution_MailFilter_queryInterface Bonobo_Unknown_queryInterface
   void GNOME_Evolution_MailFilter_addFilter(GNOME_Evolution_MailFilter _obj,
					     const CORBA_char * rule,
					     CORBA_Environment * ev);
   void GNOME_Evolution_MailFilter_removeFilter(GNOME_Evolution_MailFilter
						_obj, const CORBA_char * rule,
						CORBA_Environment * ev);

   void
      _ORBIT_skel_GNOME_Evolution_MessageList_selectMessage
      (POA_GNOME_Evolution_MessageList * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_selectMessage) (PortableServer_Servant _servant,
				    const CORBA_long message_number,
				    CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Evolution_MessageList_openMessage
      (POA_GNOME_Evolution_MessageList * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_openMessage) (PortableServer_Servant _servant,
				  const CORBA_long message_number,
				  CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Evolution_FolderBrowser_getMessageList
      (POA_GNOME_Evolution_FolderBrowser * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       GNOME_Evolution_MessageList(*_impl_getMessageList)
       (PortableServer_Servant _servant, CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Evolution_FolderInfo_getInfo
      (POA_GNOME_Evolution_FolderInfo * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_getInfo) (PortableServer_Servant _servant,
			      const CORBA_char * foldername,
			      const Bonobo_Listener listener,
			      CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Evolution_MailConfig_addAccount
      (POA_GNOME_Evolution_MailConfig * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_addAccount) (PortableServer_Servant _servant,
				 const GNOME_Evolution_MailConfig_Account *
				 acc, CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Evolution_MailFilter_addFilter
      (POA_GNOME_Evolution_MailFilter * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_addFilter) (PortableServer_Servant _servant,
				const CORBA_char * rule,
				CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Evolution_MailFilter_removeFilter
      (POA_GNOME_Evolution_MailFilter * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_removeFilter) (PortableServer_Servant _servant,
				   const CORBA_char * rule,
				   CORBA_Environment * ev));
#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif
#undef ORBIT_IDL_SERIAL
