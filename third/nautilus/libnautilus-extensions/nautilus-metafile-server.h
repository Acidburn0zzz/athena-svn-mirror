/*
 * This file was generated by orbit-idl - DO NOT EDIT!
 */

#include <glib.h>
#define ORBIT_IDL_SERIAL 9
#include <orb/orbit.h>

#ifndef nautilus_metafile_server_H
#define nautilus_metafile_server_H 1
#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/** typedefs **/
#include <bonobo/Bonobo.h>
#include <libnautilus/nautilus-view-component.h>
#if !defined(ORBIT_DECL_CORBA_sequence_CORBA_string) && !defined(_CORBA_sequence_CORBA_string_defined)
#define ORBIT_DECL_CORBA_sequence_CORBA_string 1
#define _CORBA_sequence_CORBA_string_defined 1
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_0 'n'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_1 'a'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_2 'u'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_3 't'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_4 'i'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_5 'l'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_6 'u'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_7 's'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_8 '_'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_9 'm'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_10 'e'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_11 't'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_12 'a'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_13 'f'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_14 'i'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_15 'l'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_16 'e'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_17 '_'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_18 's'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_19 'e'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_20 'r'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_21 'v'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_22 'e'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_23 'r'
   typedef struct
   {
      CORBA_unsigned_long _maximum,
       _length;
      CORBA_char **_buffer;
      CORBA_boolean _release;
   }
   CORBA_sequence_CORBA_string;
#if !defined(TC_IMPL_TC_CORBA_sequence_CORBA_string_0)
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_0 'n'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_1 'a'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_2 'u'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_3 't'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_4 'i'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_5 'l'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_6 'u'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_7 's'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_8 '_'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_9 'm'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_10 'e'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_11 't'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_12 'a'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_13 'f'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_14 'i'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_15 'l'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_16 'e'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_17 '_'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_18 's'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_19 'e'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_20 'r'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_21 'v'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_22 'e'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_23 'r'
   extern const struct CORBA_TypeCode_struct
      TC_CORBA_sequence_CORBA_string_struct;
#define TC_CORBA_sequence_CORBA_string ((CORBA_TypeCode)&TC_CORBA_sequence_CORBA_string_struct)
#endif
   extern CORBA_sequence_CORBA_string
      *CORBA_sequence_CORBA_string__alloc(void);
   extern gpointer CORBA_sequence_CORBA_string__free(gpointer mem,
						     gpointer dat,
						     CORBA_boolean free_strings);	/* ORBit internal use */
   CORBA_char **CORBA_sequence_CORBA_string_allocbuf(CORBA_unsigned_long len);
#endif
#if !defined(_Nautilus_FileNameList_defined)
#define _Nautilus_FileNameList_defined 1
   typedef CORBA_sequence_CORBA_string Nautilus_FileNameList;
#if !defined(TC_IMPL_TC_Nautilus_FileNameList_0)
#define TC_IMPL_TC_Nautilus_FileNameList_0 'n'
#define TC_IMPL_TC_Nautilus_FileNameList_1 'a'
#define TC_IMPL_TC_Nautilus_FileNameList_2 'u'
#define TC_IMPL_TC_Nautilus_FileNameList_3 't'
#define TC_IMPL_TC_Nautilus_FileNameList_4 'i'
#define TC_IMPL_TC_Nautilus_FileNameList_5 'l'
#define TC_IMPL_TC_Nautilus_FileNameList_6 'u'
#define TC_IMPL_TC_Nautilus_FileNameList_7 's'
#define TC_IMPL_TC_Nautilus_FileNameList_8 '_'
#define TC_IMPL_TC_Nautilus_FileNameList_9 'm'
#define TC_IMPL_TC_Nautilus_FileNameList_10 'e'
#define TC_IMPL_TC_Nautilus_FileNameList_11 't'
#define TC_IMPL_TC_Nautilus_FileNameList_12 'a'
#define TC_IMPL_TC_Nautilus_FileNameList_13 'f'
#define TC_IMPL_TC_Nautilus_FileNameList_14 'i'
#define TC_IMPL_TC_Nautilus_FileNameList_15 'l'
#define TC_IMPL_TC_Nautilus_FileNameList_16 'e'
#define TC_IMPL_TC_Nautilus_FileNameList_17 '_'
#define TC_IMPL_TC_Nautilus_FileNameList_18 's'
#define TC_IMPL_TC_Nautilus_FileNameList_19 'e'
#define TC_IMPL_TC_Nautilus_FileNameList_20 'r'
#define TC_IMPL_TC_Nautilus_FileNameList_21 'v'
#define TC_IMPL_TC_Nautilus_FileNameList_22 'e'
#define TC_IMPL_TC_Nautilus_FileNameList_23 'r'
   extern const struct CORBA_TypeCode_struct TC_Nautilus_FileNameList_struct;
#define TC_Nautilus_FileNameList ((CORBA_TypeCode)&TC_Nautilus_FileNameList_struct)
#endif
   extern Nautilus_FileNameList *Nautilus_FileNameList__alloc(void);
   extern gpointer Nautilus_FileNameList__free(gpointer mem, gpointer dat,
					       CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(ORBIT_DECL_Nautilus_MetafileMonitor) && !defined(_Nautilus_MetafileMonitor_defined)
#define ORBIT_DECL_Nautilus_MetafileMonitor 1
#define _Nautilus_MetafileMonitor_defined 1
#define Nautilus_MetafileMonitor__free CORBA_Object__free
   typedef CORBA_Object Nautilus_MetafileMonitor;
   extern CORBA_unsigned_long Nautilus_MetafileMonitor__classid;
#if !defined(TC_IMPL_TC_Nautilus_MetafileMonitor_0)
#define TC_IMPL_TC_Nautilus_MetafileMonitor_0 'n'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_1 'a'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_2 'u'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_3 't'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_4 'i'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_5 'l'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_6 'u'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_7 's'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_8 '_'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_9 'm'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_10 'e'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_11 't'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_12 'a'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_13 'f'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_14 'i'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_15 'l'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_16 'e'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_17 '_'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_18 's'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_19 'e'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_20 'r'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_21 'v'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_22 'e'
#define TC_IMPL_TC_Nautilus_MetafileMonitor_23 'r'
   extern const struct CORBA_TypeCode_struct
      TC_Nautilus_MetafileMonitor_struct;
#define TC_Nautilus_MetafileMonitor ((CORBA_TypeCode)&TC_Nautilus_MetafileMonitor_struct)
#endif
#endif
#if !defined(ORBIT_DECL_CORBA_sequence_CORBA_string) && !defined(_CORBA_sequence_CORBA_string_defined)
#define ORBIT_DECL_CORBA_sequence_CORBA_string 1
#define _CORBA_sequence_CORBA_string_defined 1
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_0 'n'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_1 'a'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_2 'u'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_3 't'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_4 'i'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_5 'l'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_6 'u'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_7 's'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_8 '_'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_9 'm'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_10 'e'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_11 't'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_12 'a'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_13 'f'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_14 'i'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_15 'l'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_16 'e'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_17 '_'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_18 's'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_19 'e'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_20 'r'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_21 'v'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_22 'e'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_23 'r'
   typedef struct
   {
      CORBA_unsigned_long _maximum,
       _length;
      CORBA_char **_buffer;
      CORBA_boolean _release;
   }
   CORBA_sequence_CORBA_string;
#if !defined(TC_IMPL_TC_CORBA_sequence_CORBA_string_0)
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_0 'n'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_1 'a'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_2 'u'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_3 't'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_4 'i'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_5 'l'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_6 'u'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_7 's'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_8 '_'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_9 'm'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_10 'e'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_11 't'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_12 'a'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_13 'f'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_14 'i'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_15 'l'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_16 'e'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_17 '_'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_18 's'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_19 'e'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_20 'r'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_21 'v'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_22 'e'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_23 'r'
   extern const struct CORBA_TypeCode_struct
      TC_CORBA_sequence_CORBA_string_struct;
#define TC_CORBA_sequence_CORBA_string ((CORBA_TypeCode)&TC_CORBA_sequence_CORBA_string_struct)
#endif
   extern CORBA_sequence_CORBA_string
      *CORBA_sequence_CORBA_string__alloc(void);
   extern gpointer CORBA_sequence_CORBA_string__free(gpointer mem,
						     gpointer dat,
						     CORBA_boolean free_strings);	/* ORBit internal use */
   CORBA_char **CORBA_sequence_CORBA_string_allocbuf(CORBA_unsigned_long len);
#endif
#if !defined(_Nautilus_MetadataList_defined)
#define _Nautilus_MetadataList_defined 1
   typedef CORBA_sequence_CORBA_string Nautilus_MetadataList;
#if !defined(TC_IMPL_TC_Nautilus_MetadataList_0)
#define TC_IMPL_TC_Nautilus_MetadataList_0 'n'
#define TC_IMPL_TC_Nautilus_MetadataList_1 'a'
#define TC_IMPL_TC_Nautilus_MetadataList_2 'u'
#define TC_IMPL_TC_Nautilus_MetadataList_3 't'
#define TC_IMPL_TC_Nautilus_MetadataList_4 'i'
#define TC_IMPL_TC_Nautilus_MetadataList_5 'l'
#define TC_IMPL_TC_Nautilus_MetadataList_6 'u'
#define TC_IMPL_TC_Nautilus_MetadataList_7 's'
#define TC_IMPL_TC_Nautilus_MetadataList_8 '_'
#define TC_IMPL_TC_Nautilus_MetadataList_9 'm'
#define TC_IMPL_TC_Nautilus_MetadataList_10 'e'
#define TC_IMPL_TC_Nautilus_MetadataList_11 't'
#define TC_IMPL_TC_Nautilus_MetadataList_12 'a'
#define TC_IMPL_TC_Nautilus_MetadataList_13 'f'
#define TC_IMPL_TC_Nautilus_MetadataList_14 'i'
#define TC_IMPL_TC_Nautilus_MetadataList_15 'l'
#define TC_IMPL_TC_Nautilus_MetadataList_16 'e'
#define TC_IMPL_TC_Nautilus_MetadataList_17 '_'
#define TC_IMPL_TC_Nautilus_MetadataList_18 's'
#define TC_IMPL_TC_Nautilus_MetadataList_19 'e'
#define TC_IMPL_TC_Nautilus_MetadataList_20 'r'
#define TC_IMPL_TC_Nautilus_MetadataList_21 'v'
#define TC_IMPL_TC_Nautilus_MetadataList_22 'e'
#define TC_IMPL_TC_Nautilus_MetadataList_23 'r'
   extern const struct CORBA_TypeCode_struct TC_Nautilus_MetadataList_struct;
#define TC_Nautilus_MetadataList ((CORBA_TypeCode)&TC_Nautilus_MetadataList_struct)
#endif
   extern Nautilus_MetadataList *Nautilus_MetadataList__alloc(void);
   extern gpointer Nautilus_MetadataList__free(gpointer mem, gpointer dat,
					       CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(ORBIT_DECL_Nautilus_Metafile) && !defined(_Nautilus_Metafile_defined)
#define ORBIT_DECL_Nautilus_Metafile 1
#define _Nautilus_Metafile_defined 1
#define Nautilus_Metafile__free CORBA_Object__free
   typedef CORBA_Object Nautilus_Metafile;
   extern CORBA_unsigned_long Nautilus_Metafile__classid;
#if !defined(TC_IMPL_TC_Nautilus_Metafile_0)
#define TC_IMPL_TC_Nautilus_Metafile_0 'n'
#define TC_IMPL_TC_Nautilus_Metafile_1 'a'
#define TC_IMPL_TC_Nautilus_Metafile_2 'u'
#define TC_IMPL_TC_Nautilus_Metafile_3 't'
#define TC_IMPL_TC_Nautilus_Metafile_4 'i'
#define TC_IMPL_TC_Nautilus_Metafile_5 'l'
#define TC_IMPL_TC_Nautilus_Metafile_6 'u'
#define TC_IMPL_TC_Nautilus_Metafile_7 's'
#define TC_IMPL_TC_Nautilus_Metafile_8 '_'
#define TC_IMPL_TC_Nautilus_Metafile_9 'm'
#define TC_IMPL_TC_Nautilus_Metafile_10 'e'
#define TC_IMPL_TC_Nautilus_Metafile_11 't'
#define TC_IMPL_TC_Nautilus_Metafile_12 'a'
#define TC_IMPL_TC_Nautilus_Metafile_13 'f'
#define TC_IMPL_TC_Nautilus_Metafile_14 'i'
#define TC_IMPL_TC_Nautilus_Metafile_15 'l'
#define TC_IMPL_TC_Nautilus_Metafile_16 'e'
#define TC_IMPL_TC_Nautilus_Metafile_17 '_'
#define TC_IMPL_TC_Nautilus_Metafile_18 's'
#define TC_IMPL_TC_Nautilus_Metafile_19 'e'
#define TC_IMPL_TC_Nautilus_Metafile_20 'r'
#define TC_IMPL_TC_Nautilus_Metafile_21 'v'
#define TC_IMPL_TC_Nautilus_Metafile_22 'e'
#define TC_IMPL_TC_Nautilus_Metafile_23 'r'
   extern const struct CORBA_TypeCode_struct TC_Nautilus_Metafile_struct;
#define TC_Nautilus_Metafile ((CORBA_TypeCode)&TC_Nautilus_Metafile_struct)
#endif
#endif
#if !defined(ORBIT_DECL_Nautilus_MetafileFactory) && !defined(_Nautilus_MetafileFactory_defined)
#define ORBIT_DECL_Nautilus_MetafileFactory 1
#define _Nautilus_MetafileFactory_defined 1
#define Nautilus_MetafileFactory__free CORBA_Object__free
   typedef CORBA_Object Nautilus_MetafileFactory;
   extern CORBA_unsigned_long Nautilus_MetafileFactory__classid;
#if !defined(TC_IMPL_TC_Nautilus_MetafileFactory_0)
#define TC_IMPL_TC_Nautilus_MetafileFactory_0 'n'
#define TC_IMPL_TC_Nautilus_MetafileFactory_1 'a'
#define TC_IMPL_TC_Nautilus_MetafileFactory_2 'u'
#define TC_IMPL_TC_Nautilus_MetafileFactory_3 't'
#define TC_IMPL_TC_Nautilus_MetafileFactory_4 'i'
#define TC_IMPL_TC_Nautilus_MetafileFactory_5 'l'
#define TC_IMPL_TC_Nautilus_MetafileFactory_6 'u'
#define TC_IMPL_TC_Nautilus_MetafileFactory_7 's'
#define TC_IMPL_TC_Nautilus_MetafileFactory_8 '_'
#define TC_IMPL_TC_Nautilus_MetafileFactory_9 'm'
#define TC_IMPL_TC_Nautilus_MetafileFactory_10 'e'
#define TC_IMPL_TC_Nautilus_MetafileFactory_11 't'
#define TC_IMPL_TC_Nautilus_MetafileFactory_12 'a'
#define TC_IMPL_TC_Nautilus_MetafileFactory_13 'f'
#define TC_IMPL_TC_Nautilus_MetafileFactory_14 'i'
#define TC_IMPL_TC_Nautilus_MetafileFactory_15 'l'
#define TC_IMPL_TC_Nautilus_MetafileFactory_16 'e'
#define TC_IMPL_TC_Nautilus_MetafileFactory_17 '_'
#define TC_IMPL_TC_Nautilus_MetafileFactory_18 's'
#define TC_IMPL_TC_Nautilus_MetafileFactory_19 'e'
#define TC_IMPL_TC_Nautilus_MetafileFactory_20 'r'
#define TC_IMPL_TC_Nautilus_MetafileFactory_21 'v'
#define TC_IMPL_TC_Nautilus_MetafileFactory_22 'e'
#define TC_IMPL_TC_Nautilus_MetafileFactory_23 'r'
   extern const struct CORBA_TypeCode_struct
      TC_Nautilus_MetafileFactory_struct;
#define TC_Nautilus_MetafileFactory ((CORBA_TypeCode)&TC_Nautilus_MetafileFactory_struct)
#endif
#endif

/** POA structures **/
   typedef struct
   {
      void *_private;
      void (*metafile_changed) (PortableServer_Servant _servant,
				const Nautilus_FileNameList * file_names,
				CORBA_Environment * ev);
      void (*metafile_ready) (PortableServer_Servant _servant,
			      CORBA_Environment * ev);
   }
   POA_Nautilus_MetafileMonitor__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_Nautilus_MetafileMonitor__epv *Nautilus_MetafileMonitor_epv;
   }
   POA_Nautilus_MetafileMonitor__vepv;
   typedef struct
   {
      void *_private;
      POA_Nautilus_MetafileMonitor__vepv *vepv;
   }
   POA_Nautilus_MetafileMonitor;
   extern void POA_Nautilus_MetafileMonitor__init(PortableServer_Servant
						  servant,
						  CORBA_Environment * ev);
   extern void POA_Nautilus_MetafileMonitor__fini(PortableServer_Servant
						  servant,
						  CORBA_Environment * ev);
   typedef struct
   {
      void *_private;
      
	 CORBA_boolean(*is_read) (PortableServer_Servant _servant,
				  CORBA_Environment * ev);
      CORBA_char *(*get) (PortableServer_Servant _servant,
			  const CORBA_char * file_name,
			  const CORBA_char * key,
			  const CORBA_char * default_value,
			  CORBA_Environment * ev);
      Nautilus_MetadataList *(*get_list) (PortableServer_Servant _servant,
					  const CORBA_char * file_name,
					  const CORBA_char * list_key,
					  const CORBA_char * list_subkey,
					  CORBA_Environment * ev);
      void (*set) (PortableServer_Servant _servant,
		   const CORBA_char * file_name, const CORBA_char * key,
		   const CORBA_char * default_value,
		   const CORBA_char * metadata, CORBA_Environment * ev);
      void (*set_list) (PortableServer_Servant _servant,
			const CORBA_char * file_name,
			const CORBA_char * list_key,
			const CORBA_char * list_subkey,
			const Nautilus_MetadataList * list,
			CORBA_Environment * ev);
      void (*copy) (PortableServer_Servant _servant,
		    const CORBA_char * source_file_name,
		    const Nautilus_URI destination_directory_uri,
		    const CORBA_char * destination_file_name,
		    CORBA_Environment * ev);
      void (*remove) (PortableServer_Servant _servant,
		      const CORBA_char * file_name, CORBA_Environment * ev);
      void (*rename) (PortableServer_Servant _servant,
		      const CORBA_char * old_file_name,
		      const CORBA_char * new_file_name,
		      CORBA_Environment * ev);
      void (*register_monitor) (PortableServer_Servant _servant,
				const Nautilus_MetafileMonitor monitor,
				CORBA_Environment * ev);
      void (*unregister_monitor) (PortableServer_Servant _servant,
				  const Nautilus_MetafileMonitor monitor,
				  CORBA_Environment * ev);
   }
   POA_Nautilus_Metafile__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_Nautilus_Metafile__epv *Nautilus_Metafile_epv;
   }
   POA_Nautilus_Metafile__vepv;
   typedef struct
   {
      void *_private;
      POA_Nautilus_Metafile__vepv *vepv;
   }
   POA_Nautilus_Metafile;
   extern void POA_Nautilus_Metafile__init(PortableServer_Servant servant,
					   CORBA_Environment * ev);
   extern void POA_Nautilus_Metafile__fini(PortableServer_Servant servant,
					   CORBA_Environment * ev);
   typedef struct
   {
      void *_private;
      
	 Nautilus_Metafile(*open) (PortableServer_Servant _servant,
				   const Nautilus_URI directory,
				   CORBA_Environment * ev);
   }
   POA_Nautilus_MetafileFactory__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_Nautilus_MetafileFactory__epv *Nautilus_MetafileFactory_epv;
   }
   POA_Nautilus_MetafileFactory__vepv;
   typedef struct
   {
      void *_private;
      POA_Nautilus_MetafileFactory__vepv *vepv;
   }
   POA_Nautilus_MetafileFactory;
   extern void POA_Nautilus_MetafileFactory__init(PortableServer_Servant
						  servant,
						  CORBA_Environment * ev);
   extern void POA_Nautilus_MetafileFactory__fini(PortableServer_Servant
						  servant,
						  CORBA_Environment * ev);

/** prototypes **/
#define Nautilus_MetafileMonitor_ref Bonobo_Unknown_ref
#define Nautilus_MetafileMonitor_unref Bonobo_Unknown_unref
#define Nautilus_MetafileMonitor_queryInterface Bonobo_Unknown_queryInterface
   void Nautilus_MetafileMonitor_metafile_changed(Nautilus_MetafileMonitor
						  _obj,
						  const Nautilus_FileNameList
						  * file_names,
						  CORBA_Environment * ev);
   void Nautilus_MetafileMonitor_metafile_ready(Nautilus_MetafileMonitor _obj,
						CORBA_Environment * ev);
#define Nautilus_Metafile_ref Bonobo_Unknown_ref
#define Nautilus_Metafile_unref Bonobo_Unknown_unref
#define Nautilus_Metafile_queryInterface Bonobo_Unknown_queryInterface
   CORBA_boolean Nautilus_Metafile_is_read(Nautilus_Metafile _obj,
					   CORBA_Environment * ev);
   CORBA_char *Nautilus_Metafile_get(Nautilus_Metafile _obj,
				     const CORBA_char * file_name,
				     const CORBA_char * key,
				     const CORBA_char * default_value,
				     CORBA_Environment * ev);
   Nautilus_MetadataList *Nautilus_Metafile_get_list(Nautilus_Metafile _obj,
						     const CORBA_char *
						     file_name,
						     const CORBA_char *
						     list_key,
						     const CORBA_char *
						     list_subkey,
						     CORBA_Environment * ev);
   void Nautilus_Metafile_set(Nautilus_Metafile _obj,
			      const CORBA_char * file_name,
			      const CORBA_char * key,
			      const CORBA_char * default_value,
			      const CORBA_char * metadata,
			      CORBA_Environment * ev);
   void Nautilus_Metafile_set_list(Nautilus_Metafile _obj,
				   const CORBA_char * file_name,
				   const CORBA_char * list_key,
				   const CORBA_char * list_subkey,
				   const Nautilus_MetadataList * list,
				   CORBA_Environment * ev);
   void Nautilus_Metafile_copy(Nautilus_Metafile _obj,
			       const CORBA_char * source_file_name,
			       const Nautilus_URI destination_directory_uri,
			       const CORBA_char * destination_file_name,
			       CORBA_Environment * ev);
   void Nautilus_Metafile_remove(Nautilus_Metafile _obj,
				 const CORBA_char * file_name,
				 CORBA_Environment * ev);
   void Nautilus_Metafile_rename(Nautilus_Metafile _obj,
				 const CORBA_char * old_file_name,
				 const CORBA_char * new_file_name,
				 CORBA_Environment * ev);
   void Nautilus_Metafile_register_monitor(Nautilus_Metafile _obj,
					   const Nautilus_MetafileMonitor
					   monitor, CORBA_Environment * ev);
   void Nautilus_Metafile_unregister_monitor(Nautilus_Metafile _obj,
					     const Nautilus_MetafileMonitor
					     monitor, CORBA_Environment * ev);
#define Nautilus_MetafileFactory_ref Bonobo_Unknown_ref
#define Nautilus_MetafileFactory_unref Bonobo_Unknown_unref
#define Nautilus_MetafileFactory_queryInterface Bonobo_Unknown_queryInterface
   Nautilus_Metafile Nautilus_MetafileFactory_open(Nautilus_MetafileFactory
						   _obj,
						   const Nautilus_URI
						   directory,
						   CORBA_Environment * ev);

   void
      _ORBIT_skel_Nautilus_MetafileMonitor_metafile_changed
      (POA_Nautilus_MetafileMonitor * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_metafile_changed) (PortableServer_Servant _servant,
				       const Nautilus_FileNameList *
				       file_names, CORBA_Environment * ev));
   void
      _ORBIT_skel_Nautilus_MetafileMonitor_metafile_ready
      (POA_Nautilus_MetafileMonitor * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_metafile_ready) (PortableServer_Servant _servant,
				     CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_Metafile_is_read(POA_Nautilus_Metafile *
					      _ORBIT_servant,
					      GIOPRecvBuffer *
					      _ORBIT_recv_buffer,
					      CORBA_Environment * ev,
					      CORBA_boolean(*_impl_is_read)
					      (PortableServer_Servant
					       _servant,
					       CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_Metafile_get(POA_Nautilus_Metafile *
					  _ORBIT_servant,
					  GIOPRecvBuffer * _ORBIT_recv_buffer,
					  CORBA_Environment * ev,
					  CORBA_char *
					  (*_impl_get) (PortableServer_Servant
							_servant,
							const CORBA_char *
							file_name,
							const CORBA_char *
							key,
							const CORBA_char *
							default_value,
							CORBA_Environment *
							ev));
   void _ORBIT_skel_Nautilus_Metafile_get_list(POA_Nautilus_Metafile *
					       _ORBIT_servant,
					       GIOPRecvBuffer *
					       _ORBIT_recv_buffer,
					       CORBA_Environment * ev,
					       Nautilus_MetadataList *
					       (*_impl_get_list)
					       (PortableServer_Servant
						_servant,
						const CORBA_char * file_name,
						const CORBA_char * list_key,
						const CORBA_char *
						list_subkey,
						CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_Metafile_set(POA_Nautilus_Metafile *
					  _ORBIT_servant,
					  GIOPRecvBuffer * _ORBIT_recv_buffer,
					  CORBA_Environment * ev,
					  void (*_impl_set)
					  (PortableServer_Servant _servant,
					   const CORBA_char * file_name,
					   const CORBA_char * key,
					   const CORBA_char * default_value,
					   const CORBA_char * metadata,
					   CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_Metafile_set_list(POA_Nautilus_Metafile *
					       _ORBIT_servant,
					       GIOPRecvBuffer *
					       _ORBIT_recv_buffer,
					       CORBA_Environment * ev,
					       void (*_impl_set_list)
					       (PortableServer_Servant
						_servant,
						const CORBA_char * file_name,
						const CORBA_char * list_key,
						const CORBA_char *
						list_subkey,
						const Nautilus_MetadataList *
						list,
						CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_Metafile_copy(POA_Nautilus_Metafile *
					   _ORBIT_servant,
					   GIOPRecvBuffer *
					   _ORBIT_recv_buffer,
					   CORBA_Environment * ev,
					   void (*_impl_copy)
					   (PortableServer_Servant _servant,
					    const CORBA_char *
					    source_file_name,
					    const Nautilus_URI
					    destination_directory_uri,
					    const CORBA_char *
					    destination_file_name,
					    CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_Metafile_remove(POA_Nautilus_Metafile *
					     _ORBIT_servant,
					     GIOPRecvBuffer *
					     _ORBIT_recv_buffer,
					     CORBA_Environment * ev,
					     void (*_impl_remove)
					     (PortableServer_Servant _servant,
					      const CORBA_char * file_name,
					      CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_Metafile_rename(POA_Nautilus_Metafile *
					     _ORBIT_servant,
					     GIOPRecvBuffer *
					     _ORBIT_recv_buffer,
					     CORBA_Environment * ev,
					     void (*_impl_rename)
					     (PortableServer_Servant _servant,
					      const CORBA_char *
					      old_file_name,
					      const CORBA_char *
					      new_file_name,
					      CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_Metafile_register_monitor(POA_Nautilus_Metafile *
						       _ORBIT_servant,
						       GIOPRecvBuffer *
						       _ORBIT_recv_buffer,
						       CORBA_Environment * ev,
						       void
						       (*_impl_register_monitor)
						       (PortableServer_Servant
							_servant,
							const
							Nautilus_MetafileMonitor
							monitor,
							CORBA_Environment *
							ev));
   void _ORBIT_skel_Nautilus_Metafile_unregister_monitor(POA_Nautilus_Metafile
							 * _ORBIT_servant,
							 GIOPRecvBuffer *
							 _ORBIT_recv_buffer,
							 CORBA_Environment *
							 ev,
							 void
							 (*_impl_unregister_monitor)
							 (PortableServer_Servant
							  _servant,
							  const
							  Nautilus_MetafileMonitor
							  monitor,
							  CORBA_Environment *
							  ev));
   void _ORBIT_skel_Nautilus_MetafileFactory_open(POA_Nautilus_MetafileFactory
						  * _ORBIT_servant,
						  GIOPRecvBuffer *
						  _ORBIT_recv_buffer,
						  CORBA_Environment * ev,
						  Nautilus_Metafile
						  (*_impl_open)
						  (PortableServer_Servant
						   _servant,
						   const Nautilus_URI
						   directory,
						   CORBA_Environment * ev));
#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif
#undef ORBIT_IDL_SERIAL
