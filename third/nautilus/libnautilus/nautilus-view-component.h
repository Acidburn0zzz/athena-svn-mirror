/*
 * This file was generated by orbit-idl - DO NOT EDIT!
 */

#include <glib.h>
#define ORBIT_IDL_SERIAL 9
#include <orb/orbit.h>

#ifndef nautilus_view_component_H
#define nautilus_view_component_H 1
#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/** typedefs **/
#include <bonobo/Bonobo.h>
#if !defined(_Nautilus_URI_defined)
#define _Nautilus_URI_defined 1
   typedef CORBA_char *Nautilus_URI;
#if !defined(TC_IMPL_TC_Nautilus_URI_0)
#define TC_IMPL_TC_Nautilus_URI_0 'n'
#define TC_IMPL_TC_Nautilus_URI_1 'a'
#define TC_IMPL_TC_Nautilus_URI_2 'u'
#define TC_IMPL_TC_Nautilus_URI_3 't'
#define TC_IMPL_TC_Nautilus_URI_4 'i'
#define TC_IMPL_TC_Nautilus_URI_5 'l'
#define TC_IMPL_TC_Nautilus_URI_6 'u'
#define TC_IMPL_TC_Nautilus_URI_7 's'
#define TC_IMPL_TC_Nautilus_URI_8 '_'
#define TC_IMPL_TC_Nautilus_URI_9 'v'
#define TC_IMPL_TC_Nautilus_URI_10 'i'
#define TC_IMPL_TC_Nautilus_URI_11 'e'
#define TC_IMPL_TC_Nautilus_URI_12 'w'
#define TC_IMPL_TC_Nautilus_URI_13 '_'
#define TC_IMPL_TC_Nautilus_URI_14 'c'
#define TC_IMPL_TC_Nautilus_URI_15 'o'
#define TC_IMPL_TC_Nautilus_URI_16 'm'
#define TC_IMPL_TC_Nautilus_URI_17 'p'
#define TC_IMPL_TC_Nautilus_URI_18 'o'
#define TC_IMPL_TC_Nautilus_URI_19 'n'
#define TC_IMPL_TC_Nautilus_URI_20 'e'
#define TC_IMPL_TC_Nautilus_URI_21 'n'
#define TC_IMPL_TC_Nautilus_URI_22 't'
   extern const struct CORBA_TypeCode_struct TC_Nautilus_URI_struct;
#define TC_Nautilus_URI ((CORBA_TypeCode)&TC_Nautilus_URI_struct)
#endif
#define Nautilus_URI__free CORBA_string__free
#endif
#if !defined(ORBIT_DECL_CORBA_sequence_Nautilus_URI) && !defined(_CORBA_sequence_Nautilus_URI_defined)
#define ORBIT_DECL_CORBA_sequence_Nautilus_URI 1
#define _CORBA_sequence_Nautilus_URI_defined 1
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_0 'n'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_1 'a'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_2 'u'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_3 't'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_4 'i'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_5 'l'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_6 'u'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_7 's'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_8 '_'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_9 'v'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_10 'i'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_11 'e'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_12 'w'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_13 '_'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_14 'c'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_15 'o'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_16 'm'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_17 'p'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_18 'o'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_19 'n'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_20 'e'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_21 'n'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_URI_22 't'
   typedef struct
   {
      CORBA_unsigned_long _maximum,
       _length;
      Nautilus_URI *_buffer;
      CORBA_boolean _release;
   }
   CORBA_sequence_Nautilus_URI;
#if !defined(TC_IMPL_TC_CORBA_sequence_Nautilus_URI_0)
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_0 'n'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_1 'a'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_2 'u'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_3 't'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_4 'i'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_5 'l'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_6 'u'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_7 's'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_8 '_'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_9 'v'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_10 'i'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_11 'e'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_12 'w'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_13 '_'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_14 'c'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_15 'o'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_16 'm'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_17 'p'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_18 'o'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_19 'n'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_20 'e'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_21 'n'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_URI_22 't'
   extern const struct CORBA_TypeCode_struct
      TC_CORBA_sequence_Nautilus_URI_struct;
#define TC_CORBA_sequence_Nautilus_URI ((CORBA_TypeCode)&TC_CORBA_sequence_Nautilus_URI_struct)
#endif
   extern CORBA_sequence_Nautilus_URI
      *CORBA_sequence_Nautilus_URI__alloc(void);
   extern gpointer CORBA_sequence_Nautilus_URI__free(gpointer mem,
						     gpointer dat,
						     CORBA_boolean free_strings);	/* ORBit internal use */
   Nautilus_URI *CORBA_sequence_Nautilus_URI_allocbuf(CORBA_unsigned_long
						      len);
#endif
#if !defined(_Nautilus_URIList_defined)
#define _Nautilus_URIList_defined 1
   typedef CORBA_sequence_Nautilus_URI Nautilus_URIList;
#if !defined(TC_IMPL_TC_Nautilus_URIList_0)
#define TC_IMPL_TC_Nautilus_URIList_0 'n'
#define TC_IMPL_TC_Nautilus_URIList_1 'a'
#define TC_IMPL_TC_Nautilus_URIList_2 'u'
#define TC_IMPL_TC_Nautilus_URIList_3 't'
#define TC_IMPL_TC_Nautilus_URIList_4 'i'
#define TC_IMPL_TC_Nautilus_URIList_5 'l'
#define TC_IMPL_TC_Nautilus_URIList_6 'u'
#define TC_IMPL_TC_Nautilus_URIList_7 's'
#define TC_IMPL_TC_Nautilus_URIList_8 '_'
#define TC_IMPL_TC_Nautilus_URIList_9 'v'
#define TC_IMPL_TC_Nautilus_URIList_10 'i'
#define TC_IMPL_TC_Nautilus_URIList_11 'e'
#define TC_IMPL_TC_Nautilus_URIList_12 'w'
#define TC_IMPL_TC_Nautilus_URIList_13 '_'
#define TC_IMPL_TC_Nautilus_URIList_14 'c'
#define TC_IMPL_TC_Nautilus_URIList_15 'o'
#define TC_IMPL_TC_Nautilus_URIList_16 'm'
#define TC_IMPL_TC_Nautilus_URIList_17 'p'
#define TC_IMPL_TC_Nautilus_URIList_18 'o'
#define TC_IMPL_TC_Nautilus_URIList_19 'n'
#define TC_IMPL_TC_Nautilus_URIList_20 'e'
#define TC_IMPL_TC_Nautilus_URIList_21 'n'
#define TC_IMPL_TC_Nautilus_URIList_22 't'
   extern const struct CORBA_TypeCode_struct TC_Nautilus_URIList_struct;
#define TC_Nautilus_URIList ((CORBA_TypeCode)&TC_Nautilus_URIList_struct)
#endif
   extern Nautilus_URIList *Nautilus_URIList__alloc(void);
   extern gpointer Nautilus_URIList__free(gpointer mem, gpointer dat,
					  CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(_Nautilus_HistoryItem_defined)
#define _Nautilus_HistoryItem_defined 1
   typedef struct
   {
      CORBA_char *title;
      Nautilus_URI location;
      CORBA_char *icon;
   }
   Nautilus_HistoryItem;

#if !defined(TC_IMPL_TC_Nautilus_HistoryItem_0)
#define TC_IMPL_TC_Nautilus_HistoryItem_0 'n'
#define TC_IMPL_TC_Nautilus_HistoryItem_1 'a'
#define TC_IMPL_TC_Nautilus_HistoryItem_2 'u'
#define TC_IMPL_TC_Nautilus_HistoryItem_3 't'
#define TC_IMPL_TC_Nautilus_HistoryItem_4 'i'
#define TC_IMPL_TC_Nautilus_HistoryItem_5 'l'
#define TC_IMPL_TC_Nautilus_HistoryItem_6 'u'
#define TC_IMPL_TC_Nautilus_HistoryItem_7 's'
#define TC_IMPL_TC_Nautilus_HistoryItem_8 '_'
#define TC_IMPL_TC_Nautilus_HistoryItem_9 'v'
#define TC_IMPL_TC_Nautilus_HistoryItem_10 'i'
#define TC_IMPL_TC_Nautilus_HistoryItem_11 'e'
#define TC_IMPL_TC_Nautilus_HistoryItem_12 'w'
#define TC_IMPL_TC_Nautilus_HistoryItem_13 '_'
#define TC_IMPL_TC_Nautilus_HistoryItem_14 'c'
#define TC_IMPL_TC_Nautilus_HistoryItem_15 'o'
#define TC_IMPL_TC_Nautilus_HistoryItem_16 'm'
#define TC_IMPL_TC_Nautilus_HistoryItem_17 'p'
#define TC_IMPL_TC_Nautilus_HistoryItem_18 'o'
#define TC_IMPL_TC_Nautilus_HistoryItem_19 'n'
#define TC_IMPL_TC_Nautilus_HistoryItem_20 'e'
#define TC_IMPL_TC_Nautilus_HistoryItem_21 'n'
#define TC_IMPL_TC_Nautilus_HistoryItem_22 't'
   extern const struct CORBA_TypeCode_struct TC_Nautilus_HistoryItem_struct;
#define TC_Nautilus_HistoryItem ((CORBA_TypeCode)&TC_Nautilus_HistoryItem_struct)
#endif
   extern Nautilus_HistoryItem *Nautilus_HistoryItem__alloc(void);
   extern gpointer Nautilus_HistoryItem__free(gpointer mem, gpointer dat,
					      CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(ORBIT_DECL_CORBA_sequence_Nautilus_HistoryItem) && !defined(_CORBA_sequence_Nautilus_HistoryItem_defined)
#define ORBIT_DECL_CORBA_sequence_Nautilus_HistoryItem 1
#define _CORBA_sequence_Nautilus_HistoryItem_defined 1
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_0 'n'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_1 'a'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_2 'u'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_3 't'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_4 'i'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_5 'l'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_6 'u'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_7 's'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_8 '_'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_9 'v'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_10 'i'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_11 'e'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_12 'w'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_13 '_'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_14 'c'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_15 'o'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_16 'm'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_17 'p'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_18 'o'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_19 'n'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_20 'e'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_21 'n'
#define ORBIT_IMPL_CORBA_sequence_Nautilus_HistoryItem_22 't'
   typedef struct
   {
      CORBA_unsigned_long _maximum,
       _length;
      Nautilus_HistoryItem *_buffer;
      CORBA_boolean _release;
   }
   CORBA_sequence_Nautilus_HistoryItem;
#if !defined(TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_0)
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_0 'n'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_1 'a'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_2 'u'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_3 't'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_4 'i'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_5 'l'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_6 'u'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_7 's'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_8 '_'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_9 'v'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_10 'i'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_11 'e'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_12 'w'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_13 '_'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_14 'c'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_15 'o'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_16 'm'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_17 'p'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_18 'o'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_19 'n'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_20 'e'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_21 'n'
#define TC_IMPL_TC_CORBA_sequence_Nautilus_HistoryItem_22 't'
   extern const struct CORBA_TypeCode_struct
      TC_CORBA_sequence_Nautilus_HistoryItem_struct;
#define TC_CORBA_sequence_Nautilus_HistoryItem ((CORBA_TypeCode)&TC_CORBA_sequence_Nautilus_HistoryItem_struct)
#endif
   extern CORBA_sequence_Nautilus_HistoryItem
      *CORBA_sequence_Nautilus_HistoryItem__alloc(void);
   extern gpointer CORBA_sequence_Nautilus_HistoryItem__free(gpointer mem,
							     gpointer dat,
							     CORBA_boolean free_strings);	/* ORBit internal use */
   Nautilus_HistoryItem
      *CORBA_sequence_Nautilus_HistoryItem_allocbuf(CORBA_unsigned_long len);
#endif
#if !defined(_Nautilus_History_defined)
#define _Nautilus_History_defined 1
   typedef CORBA_sequence_Nautilus_HistoryItem Nautilus_History;
#if !defined(TC_IMPL_TC_Nautilus_History_0)
#define TC_IMPL_TC_Nautilus_History_0 'n'
#define TC_IMPL_TC_Nautilus_History_1 'a'
#define TC_IMPL_TC_Nautilus_History_2 'u'
#define TC_IMPL_TC_Nautilus_History_3 't'
#define TC_IMPL_TC_Nautilus_History_4 'i'
#define TC_IMPL_TC_Nautilus_History_5 'l'
#define TC_IMPL_TC_Nautilus_History_6 'u'
#define TC_IMPL_TC_Nautilus_History_7 's'
#define TC_IMPL_TC_Nautilus_History_8 '_'
#define TC_IMPL_TC_Nautilus_History_9 'v'
#define TC_IMPL_TC_Nautilus_History_10 'i'
#define TC_IMPL_TC_Nautilus_History_11 'e'
#define TC_IMPL_TC_Nautilus_History_12 'w'
#define TC_IMPL_TC_Nautilus_History_13 '_'
#define TC_IMPL_TC_Nautilus_History_14 'c'
#define TC_IMPL_TC_Nautilus_History_15 'o'
#define TC_IMPL_TC_Nautilus_History_16 'm'
#define TC_IMPL_TC_Nautilus_History_17 'p'
#define TC_IMPL_TC_Nautilus_History_18 'o'
#define TC_IMPL_TC_Nautilus_History_19 'n'
#define TC_IMPL_TC_Nautilus_History_20 'e'
#define TC_IMPL_TC_Nautilus_History_21 'n'
#define TC_IMPL_TC_Nautilus_History_22 't'
   extern const struct CORBA_TypeCode_struct TC_Nautilus_History_struct;
#define TC_Nautilus_History ((CORBA_TypeCode)&TC_Nautilus_History_struct)
#endif
   extern Nautilus_History *Nautilus_History__alloc(void);
   extern gpointer Nautilus_History__free(gpointer mem, gpointer dat,
					  CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(ORBIT_DECL_Nautilus_View) && !defined(_Nautilus_View_defined)
#define ORBIT_DECL_Nautilus_View 1
#define _Nautilus_View_defined 1
#define Nautilus_View__free CORBA_Object__free
   typedef CORBA_Object Nautilus_View;
   extern CORBA_unsigned_long Nautilus_View__classid;
#if !defined(TC_IMPL_TC_Nautilus_View_0)
#define TC_IMPL_TC_Nautilus_View_0 'n'
#define TC_IMPL_TC_Nautilus_View_1 'a'
#define TC_IMPL_TC_Nautilus_View_2 'u'
#define TC_IMPL_TC_Nautilus_View_3 't'
#define TC_IMPL_TC_Nautilus_View_4 'i'
#define TC_IMPL_TC_Nautilus_View_5 'l'
#define TC_IMPL_TC_Nautilus_View_6 'u'
#define TC_IMPL_TC_Nautilus_View_7 's'
#define TC_IMPL_TC_Nautilus_View_8 '_'
#define TC_IMPL_TC_Nautilus_View_9 'v'
#define TC_IMPL_TC_Nautilus_View_10 'i'
#define TC_IMPL_TC_Nautilus_View_11 'e'
#define TC_IMPL_TC_Nautilus_View_12 'w'
#define TC_IMPL_TC_Nautilus_View_13 '_'
#define TC_IMPL_TC_Nautilus_View_14 'c'
#define TC_IMPL_TC_Nautilus_View_15 'o'
#define TC_IMPL_TC_Nautilus_View_16 'm'
#define TC_IMPL_TC_Nautilus_View_17 'p'
#define TC_IMPL_TC_Nautilus_View_18 'o'
#define TC_IMPL_TC_Nautilus_View_19 'n'
#define TC_IMPL_TC_Nautilus_View_20 'e'
#define TC_IMPL_TC_Nautilus_View_21 'n'
#define TC_IMPL_TC_Nautilus_View_22 't'
   extern const struct CORBA_TypeCode_struct TC_Nautilus_View_struct;
#define TC_Nautilus_View ((CORBA_TypeCode)&TC_Nautilus_View_struct)
#endif
#endif
#if !defined(ORBIT_DECL_Nautilus_ViewFrame) && !defined(_Nautilus_ViewFrame_defined)
#define ORBIT_DECL_Nautilus_ViewFrame 1
#define _Nautilus_ViewFrame_defined 1
#define Nautilus_ViewFrame__free CORBA_Object__free
   typedef CORBA_Object Nautilus_ViewFrame;
   extern CORBA_unsigned_long Nautilus_ViewFrame__classid;
#if !defined(TC_IMPL_TC_Nautilus_ViewFrame_0)
#define TC_IMPL_TC_Nautilus_ViewFrame_0 'n'
#define TC_IMPL_TC_Nautilus_ViewFrame_1 'a'
#define TC_IMPL_TC_Nautilus_ViewFrame_2 'u'
#define TC_IMPL_TC_Nautilus_ViewFrame_3 't'
#define TC_IMPL_TC_Nautilus_ViewFrame_4 'i'
#define TC_IMPL_TC_Nautilus_ViewFrame_5 'l'
#define TC_IMPL_TC_Nautilus_ViewFrame_6 'u'
#define TC_IMPL_TC_Nautilus_ViewFrame_7 's'
#define TC_IMPL_TC_Nautilus_ViewFrame_8 '_'
#define TC_IMPL_TC_Nautilus_ViewFrame_9 'v'
#define TC_IMPL_TC_Nautilus_ViewFrame_10 'i'
#define TC_IMPL_TC_Nautilus_ViewFrame_11 'e'
#define TC_IMPL_TC_Nautilus_ViewFrame_12 'w'
#define TC_IMPL_TC_Nautilus_ViewFrame_13 '_'
#define TC_IMPL_TC_Nautilus_ViewFrame_14 'c'
#define TC_IMPL_TC_Nautilus_ViewFrame_15 'o'
#define TC_IMPL_TC_Nautilus_ViewFrame_16 'm'
#define TC_IMPL_TC_Nautilus_ViewFrame_17 'p'
#define TC_IMPL_TC_Nautilus_ViewFrame_18 'o'
#define TC_IMPL_TC_Nautilus_ViewFrame_19 'n'
#define TC_IMPL_TC_Nautilus_ViewFrame_20 'e'
#define TC_IMPL_TC_Nautilus_ViewFrame_21 'n'
#define TC_IMPL_TC_Nautilus_ViewFrame_22 't'
   extern const struct CORBA_TypeCode_struct TC_Nautilus_ViewFrame_struct;
#define TC_Nautilus_ViewFrame ((CORBA_TypeCode)&TC_Nautilus_ViewFrame_struct)
#endif
#endif

/** POA structures **/
   typedef struct
   {
      void *_private;
      void (*load_location) (PortableServer_Servant _servant,
			     const Nautilus_URI location,
			     CORBA_Environment * ev);
      void (*stop_loading) (PortableServer_Servant _servant,
			    CORBA_Environment * ev);
      void (*selection_changed) (PortableServer_Servant _servant,
				 const Nautilus_URIList * selection,
				 CORBA_Environment * ev);
      void (*title_changed) (PortableServer_Servant _servant,
			     const CORBA_char * title,
			     CORBA_Environment * ev);
      void (*history_changed) (PortableServer_Servant _servant,
			       const Nautilus_History * history,
			       CORBA_Environment * ev);
   }
   POA_Nautilus_View__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_Nautilus_View__epv *Nautilus_View_epv;
   }
   POA_Nautilus_View__vepv;
   typedef struct
   {
      void *_private;
      POA_Nautilus_View__vepv *vepv;
   }
   POA_Nautilus_View;
   extern void POA_Nautilus_View__init(PortableServer_Servant servant,
				       CORBA_Environment * ev);
   extern void POA_Nautilus_View__fini(PortableServer_Servant servant,
				       CORBA_Environment * ev);
   typedef struct
   {
      void *_private;
      void (*open_location_in_this_window) (PortableServer_Servant _servant,
					    const Nautilus_URI location,
					    CORBA_Environment * ev);
      void (*open_location_prefer_existing_window) (PortableServer_Servant
						    _servant,
						    const Nautilus_URI
						    location,
						    CORBA_Environment * ev);
      void (*open_location_force_new_window) (PortableServer_Servant _servant,
					      const Nautilus_URI location,
					      const Nautilus_URIList *
					      selection,
					      CORBA_Environment * ev);
      void (*report_location_change) (PortableServer_Servant _servant,
				      const Nautilus_URI location,
				      const Nautilus_URIList * selection,
				      const CORBA_char * title,
				      CORBA_Environment * ev);
      void (*report_redirect) (PortableServer_Servant _servant,
			       const Nautilus_URI from_location,
			       const Nautilus_URI to_location,
			       const Nautilus_URIList * selection,
			       const CORBA_char * title,
			       CORBA_Environment * ev);
      void (*report_selection_change) (PortableServer_Servant _servant,
				       const Nautilus_URIList * selection,
				       CORBA_Environment * ev);
      void (*report_status) (PortableServer_Servant _servant,
			     const CORBA_char * status,
			     CORBA_Environment * ev);
      void (*report_load_underway) (PortableServer_Servant _servant,
				    CORBA_Environment * ev);
      void (*report_load_progress) (PortableServer_Servant _servant,
				    const CORBA_float fraction_done,
				    CORBA_Environment * ev);
      void (*report_load_complete) (PortableServer_Servant _servant,
				    CORBA_Environment * ev);
      void (*report_load_failed) (PortableServer_Servant _servant,
				  CORBA_Environment * ev);
      void (*set_title) (PortableServer_Servant _servant,
			 const CORBA_char * new_title,
			 CORBA_Environment * ev);
      void (*go_back) (PortableServer_Servant _servant,
		       CORBA_Environment * ev);
   }
   POA_Nautilus_ViewFrame__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_Nautilus_ViewFrame__epv *Nautilus_ViewFrame_epv;
   }
   POA_Nautilus_ViewFrame__vepv;
   typedef struct
   {
      void *_private;
      POA_Nautilus_ViewFrame__vepv *vepv;
   }
   POA_Nautilus_ViewFrame;
   extern void POA_Nautilus_ViewFrame__init(PortableServer_Servant servant,
					    CORBA_Environment * ev);
   extern void POA_Nautilus_ViewFrame__fini(PortableServer_Servant servant,
					    CORBA_Environment * ev);

/** prototypes **/
#define Nautilus_View_ref Bonobo_Unknown_ref
#define Nautilus_View_unref Bonobo_Unknown_unref
#define Nautilus_View_queryInterface Bonobo_Unknown_queryInterface
   void Nautilus_View_load_location(Nautilus_View _obj,
				    const Nautilus_URI location,
				    CORBA_Environment * ev);
   void Nautilus_View_stop_loading(Nautilus_View _obj,
				   CORBA_Environment * ev);
   void Nautilus_View_selection_changed(Nautilus_View _obj,
					const Nautilus_URIList * selection,
					CORBA_Environment * ev);
   void Nautilus_View_title_changed(Nautilus_View _obj,
				    const CORBA_char * title,
				    CORBA_Environment * ev);
   void Nautilus_View_history_changed(Nautilus_View _obj,
				      const Nautilus_History * history,
				      CORBA_Environment * ev);
#define Nautilus_ViewFrame_ref Bonobo_Unknown_ref
#define Nautilus_ViewFrame_unref Bonobo_Unknown_unref
#define Nautilus_ViewFrame_queryInterface Bonobo_Unknown_queryInterface
   void Nautilus_ViewFrame_open_location_in_this_window(Nautilus_ViewFrame
							_obj,
							const Nautilus_URI
							location,
							CORBA_Environment *
							ev);
   void
      Nautilus_ViewFrame_open_location_prefer_existing_window
      (Nautilus_ViewFrame _obj, const Nautilus_URI location,
       CORBA_Environment * ev);
   void Nautilus_ViewFrame_open_location_force_new_window(Nautilus_ViewFrame
							  _obj,
							  const Nautilus_URI
							  location,
							  const
							  Nautilus_URIList *
							  selection,
							  CORBA_Environment *
							  ev);
   void Nautilus_ViewFrame_report_location_change(Nautilus_ViewFrame _obj,
						  const Nautilus_URI location,
						  const Nautilus_URIList *
						  selection,
						  const CORBA_char * title,
						  CORBA_Environment * ev);
   void Nautilus_ViewFrame_report_redirect(Nautilus_ViewFrame _obj,
					   const Nautilus_URI from_location,
					   const Nautilus_URI to_location,
					   const Nautilus_URIList * selection,
					   const CORBA_char * title,
					   CORBA_Environment * ev);
   void Nautilus_ViewFrame_report_selection_change(Nautilus_ViewFrame _obj,
						   const Nautilus_URIList *
						   selection,
						   CORBA_Environment * ev);
   void Nautilus_ViewFrame_report_status(Nautilus_ViewFrame _obj,
					 const CORBA_char * status,
					 CORBA_Environment * ev);
   void Nautilus_ViewFrame_report_load_underway(Nautilus_ViewFrame _obj,
						CORBA_Environment * ev);
   void Nautilus_ViewFrame_report_load_progress(Nautilus_ViewFrame _obj,
						const CORBA_float
						fraction_done,
						CORBA_Environment * ev);
   void Nautilus_ViewFrame_report_load_complete(Nautilus_ViewFrame _obj,
						CORBA_Environment * ev);
   void Nautilus_ViewFrame_report_load_failed(Nautilus_ViewFrame _obj,
					      CORBA_Environment * ev);
   void Nautilus_ViewFrame_set_title(Nautilus_ViewFrame _obj,
				     const CORBA_char * new_title,
				     CORBA_Environment * ev);
   void Nautilus_ViewFrame_go_back(Nautilus_ViewFrame _obj,
				   CORBA_Environment * ev);

   void _ORBIT_skel_Nautilus_View_load_location(POA_Nautilus_View *
						_ORBIT_servant,
						GIOPRecvBuffer *
						_ORBIT_recv_buffer,
						CORBA_Environment * ev,
						void (*_impl_load_location)
						(PortableServer_Servant
						 _servant,
						 const Nautilus_URI location,
						 CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_View_stop_loading(POA_Nautilus_View *
					       _ORBIT_servant,
					       GIOPRecvBuffer *
					       _ORBIT_recv_buffer,
					       CORBA_Environment * ev,
					       void (*_impl_stop_loading)
					       (PortableServer_Servant
						_servant,
						CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_View_selection_changed(POA_Nautilus_View *
						    _ORBIT_servant,
						    GIOPRecvBuffer *
						    _ORBIT_recv_buffer,
						    CORBA_Environment * ev,
						    void
						    (*_impl_selection_changed)
						    (PortableServer_Servant
						     _servant,
						     const Nautilus_URIList *
						     selection,
						     CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_View_title_changed(POA_Nautilus_View *
						_ORBIT_servant,
						GIOPRecvBuffer *
						_ORBIT_recv_buffer,
						CORBA_Environment * ev,
						void (*_impl_title_changed)
						(PortableServer_Servant
						 _servant,
						 const CORBA_char * title,
						 CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_View_history_changed(POA_Nautilus_View *
						  _ORBIT_servant,
						  GIOPRecvBuffer *
						  _ORBIT_recv_buffer,
						  CORBA_Environment * ev,
						  void
						  (*_impl_history_changed)
						  (PortableServer_Servant
						   _servant,
						   const Nautilus_History *
						   history,
						   CORBA_Environment * ev));
   void
      _ORBIT_skel_Nautilus_ViewFrame_open_location_in_this_window
      (POA_Nautilus_ViewFrame * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_open_location_in_this_window) (PortableServer_Servant
						   _servant,
						   const Nautilus_URI
						   location,
						   CORBA_Environment * ev));
   void
      _ORBIT_skel_Nautilus_ViewFrame_open_location_prefer_existing_window
      (POA_Nautilus_ViewFrame * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_open_location_prefer_existing_window)
       (PortableServer_Servant _servant, const Nautilus_URI location,
	CORBA_Environment * ev));
   void
      _ORBIT_skel_Nautilus_ViewFrame_open_location_force_new_window
      (POA_Nautilus_ViewFrame * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_open_location_force_new_window) (PortableServer_Servant
						     _servant,
						     const Nautilus_URI
						     location,
						     const Nautilus_URIList *
						     selection,
						     CORBA_Environment * ev));
   void
      _ORBIT_skel_Nautilus_ViewFrame_report_location_change
      (POA_Nautilus_ViewFrame * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_report_location_change) (PortableServer_Servant _servant,
					     const Nautilus_URI location,
					     const Nautilus_URIList *
					     selection,
					     const CORBA_char * title,
					     CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_ViewFrame_report_redirect(POA_Nautilus_ViewFrame
						       * _ORBIT_servant,
						       GIOPRecvBuffer *
						       _ORBIT_recv_buffer,
						       CORBA_Environment * ev,
						       void
						       (*_impl_report_redirect)
						       (PortableServer_Servant
							_servant,
							const Nautilus_URI
							from_location,
							const Nautilus_URI
							to_location,
							const Nautilus_URIList
							* selection,
							const CORBA_char *
							title,
							CORBA_Environment *
							ev));
   void
      _ORBIT_skel_Nautilus_ViewFrame_report_selection_change
      (POA_Nautilus_ViewFrame * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_report_selection_change) (PortableServer_Servant _servant,
					      const Nautilus_URIList *
					      selection,
					      CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_ViewFrame_report_status(POA_Nautilus_ViewFrame *
						     _ORBIT_servant,
						     GIOPRecvBuffer *
						     _ORBIT_recv_buffer,
						     CORBA_Environment * ev,
						     void
						     (*_impl_report_status)
						     (PortableServer_Servant
						      _servant,
						      const CORBA_char *
						      status,
						      CORBA_Environment *
						      ev));
   void
      _ORBIT_skel_Nautilus_ViewFrame_report_load_underway
      (POA_Nautilus_ViewFrame * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_report_load_underway) (PortableServer_Servant _servant,
					   CORBA_Environment * ev));
   void
      _ORBIT_skel_Nautilus_ViewFrame_report_load_progress
      (POA_Nautilus_ViewFrame * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_report_load_progress) (PortableServer_Servant _servant,
					   const CORBA_float fraction_done,
					   CORBA_Environment * ev));
   void
      _ORBIT_skel_Nautilus_ViewFrame_report_load_complete
      (POA_Nautilus_ViewFrame * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_report_load_complete) (PortableServer_Servant _servant,
					   CORBA_Environment * ev));
   void
      _ORBIT_skel_Nautilus_ViewFrame_report_load_failed(POA_Nautilus_ViewFrame
							* _ORBIT_servant,
							GIOPRecvBuffer *
							_ORBIT_recv_buffer,
							CORBA_Environment *
							ev,
							void
							(*_impl_report_load_failed)
							(PortableServer_Servant
							 _servant,
							 CORBA_Environment *
							 ev));
   void _ORBIT_skel_Nautilus_ViewFrame_set_title(POA_Nautilus_ViewFrame *
						 _ORBIT_servant,
						 GIOPRecvBuffer *
						 _ORBIT_recv_buffer,
						 CORBA_Environment * ev,
						 void (*_impl_set_title)
						 (PortableServer_Servant
						  _servant,
						  const CORBA_char *
						  new_title,
						  CORBA_Environment * ev));
   void _ORBIT_skel_Nautilus_ViewFrame_go_back(POA_Nautilus_ViewFrame *
					       _ORBIT_servant,
					       GIOPRecvBuffer *
					       _ORBIT_recv_buffer,
					       CORBA_Environment * ev,
					       void (*_impl_go_back)
					       (PortableServer_Servant
						_servant,
						CORBA_Environment * ev));
#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif
#undef ORBIT_IDL_SERIAL
