/*
 * This file was generated by orbit-idl-2 - DO NOT EDIT!
 */

#include <string.h>
#define ORBIT2_STUBS_API
#define ORBIT_IDL_C_COMMON
#define Ggv_COMMON
#include "Ggv.h"

static const CORBA_unsigned_long ORBit_zero_int = 0;

#if ( (TC_IMPL_TC_GNOME_GGV_Orientation_0 == 'G') \
&& (TC_IMPL_TC_GNOME_GGV_Orientation_1 == 'g') \
&& (TC_IMPL_TC_GNOME_GGV_Orientation_2 == 'v') \
) && !defined(TC_DEF_TC_GNOME_GGV_Orientation)
#define TC_DEF_TC_GNOME_GGV_Orientation 1
static const char *anon_subnames_array0[] =
   { "ORIENTATION_PORTRAIT", "ORIENTATION_LANDSCAPE",
"ORIENTATION_UPSIDEDOWN", "ORIENTATION_SEASCAPE" };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_GNOME_GGV_Orientation_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_enum,
   0,
   0,
   4,
   0,
   4,
   NULL,
   CORBA_OBJECT_NIL,
   "Orientation",
   "IDL:GNOME/GGV/Orientation:1.0",
   (char **) anon_subnames_array0,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_GNOME_GGV_Size_0 == 'G') \
&& (TC_IMPL_TC_GNOME_GGV_Size_1 == 'g') \
&& (TC_IMPL_TC_GNOME_GGV_Size_2 == 'v') \
) && !defined(TC_DEF_TC_GNOME_GGV_Size)
#define TC_DEF_TC_GNOME_GGV_Size 1
static const CORBA_TypeCode anon_subtypes_array8[] =
   { (CORBA_TypeCode) & TC_CORBA_string_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_GNOME_GGV_Size_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_alias,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array8,
   CORBA_OBJECT_NIL,
   "Size",
   "IDL:GNOME/GGV/Size:1.0",
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_GNOME_GGV_PageName_0 == 'G') \
&& (TC_IMPL_TC_GNOME_GGV_PageName_1 == 'g') \
&& (TC_IMPL_TC_GNOME_GGV_PageName_2 == 'v') \
) && !defined(TC_DEF_TC_GNOME_GGV_PageName)
#define TC_DEF_TC_GNOME_GGV_PageName 1
static const CORBA_TypeCode anon_subtypes_array15[] =
   { (CORBA_TypeCode) & TC_CORBA_string_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_GNOME_GGV_PageName_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_alias,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array15,
   CORBA_OBJECT_NIL,
   "PageName",
   "IDL:GNOME/GGV/PageName:1.0",
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_CORBA_sequence_CORBA_string_0 == 'G') \
&& (TC_IMPL_TC_CORBA_sequence_CORBA_string_1 == 'g') \
&& (TC_IMPL_TC_CORBA_sequence_CORBA_string_2 == 'v') \
) && !defined(TC_DEF_TC_CORBA_sequence_CORBA_string)
#define TC_DEF_TC_CORBA_sequence_CORBA_string 1
static const CORBA_TypeCode anon_subtypes_array18[] =
   { (CORBA_TypeCode) & TC_CORBA_string_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_CORBA_sequence_CORBA_string_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_sequence,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array18,
   CORBA_OBJECT_NIL,
   NULL,
   NULL,
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_PageName_0 == 'G') \
&& (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_PageName_1 == 'g') \
&& (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_PageName_2 == 'v') \
) && !defined(TC_DEF_TC_CORBA_sequence_GNOME_GGV_PageName)
#define TC_DEF_TC_CORBA_sequence_GNOME_GGV_PageName 1
static const CORBA_TypeCode anon_subtypes_array21[] =
   { (CORBA_TypeCode) & TC_GNOME_GGV_PageName_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_CORBA_sequence_GNOME_GGV_PageName_struct
   = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_sequence,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array21,
   CORBA_OBJECT_NIL,
   NULL,
   NULL,
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_PageName_0 == 'G') \
&& (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_PageName_1 == 'g') \
&& (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_PageName_2 == 'v') \
) && !defined(TC_DEF_TC_CORBA_sequence_GNOME_GGV_PageName)
#define TC_DEF_TC_CORBA_sequence_GNOME_GGV_PageName 1
static const CORBA_TypeCode anon_subtypes_array28[] =
   { (CORBA_TypeCode) & TC_GNOME_GGV_PageName_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_CORBA_sequence_GNOME_GGV_PageName_struct
   = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_sequence,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array28,
   CORBA_OBJECT_NIL,
   NULL,
   NULL,
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_GNOME_GGV_PageNameList_0 == 'G') \
&& (TC_IMPL_TC_GNOME_GGV_PageNameList_1 == 'g') \
&& (TC_IMPL_TC_GNOME_GGV_PageNameList_2 == 'v') \
) && !defined(TC_DEF_TC_GNOME_GGV_PageNameList)
#define TC_DEF_TC_GNOME_GGV_PageNameList 1
static const CORBA_TypeCode anon_subtypes_array31[] =
   { (CORBA_TypeCode) & TC_CORBA_sequence_GNOME_GGV_PageName_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_GNOME_GGV_PageNameList_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_alias,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array31,
   CORBA_OBJECT_NIL,
   "PageNameList",
   "IDL:GNOME/GGV/PageNameList:1.0",
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_GNOME_GGV_Page_0 == 'G') \
&& (TC_IMPL_TC_GNOME_GGV_Page_1 == 'g') \
&& (TC_IMPL_TC_GNOME_GGV_Page_2 == 'v') \
) && !defined(TC_DEF_TC_GNOME_GGV_Page)
#define TC_DEF_TC_GNOME_GGV_Page 1
static const CORBA_TypeCode anon_subtypes_array38[] =
   { (CORBA_TypeCode) & TC_CORBA_long_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_GNOME_GGV_Page_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_alias,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array38,
   CORBA_OBJECT_NIL,
   "Page",
   "IDL:GNOME/GGV/Page:1.0",
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_CORBA_sequence_CORBA_long_0 == 'G') \
&& (TC_IMPL_TC_CORBA_sequence_CORBA_long_1 == 'g') \
&& (TC_IMPL_TC_CORBA_sequence_CORBA_long_2 == 'v') \
) && !defined(TC_DEF_TC_CORBA_sequence_CORBA_long)
#define TC_DEF_TC_CORBA_sequence_CORBA_long 1
static const CORBA_TypeCode anon_subtypes_array41[] =
   { (CORBA_TypeCode) & TC_CORBA_long_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_CORBA_sequence_CORBA_long_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_sequence,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array41,
   CORBA_OBJECT_NIL,
   NULL,
   NULL,
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_Page_0 == 'G') \
&& (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_Page_1 == 'g') \
&& (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_Page_2 == 'v') \
) && !defined(TC_DEF_TC_CORBA_sequence_GNOME_GGV_Page)
#define TC_DEF_TC_CORBA_sequence_GNOME_GGV_Page 1
static const CORBA_TypeCode anon_subtypes_array44[] =
   { (CORBA_TypeCode) & TC_GNOME_GGV_Page_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_CORBA_sequence_GNOME_GGV_Page_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_sequence,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array44,
   CORBA_OBJECT_NIL,
   NULL,
   NULL,
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_Page_0 == 'G') \
&& (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_Page_1 == 'g') \
&& (TC_IMPL_TC_CORBA_sequence_GNOME_GGV_Page_2 == 'v') \
) && !defined(TC_DEF_TC_CORBA_sequence_GNOME_GGV_Page)
#define TC_DEF_TC_CORBA_sequence_GNOME_GGV_Page 1
static const CORBA_TypeCode anon_subtypes_array51[] =
   { (CORBA_TypeCode) & TC_GNOME_GGV_Page_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_CORBA_sequence_GNOME_GGV_Page_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_sequence,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array51,
   CORBA_OBJECT_NIL,
   NULL,
   NULL,
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_GNOME_GGV_PageList_0 == 'G') \
&& (TC_IMPL_TC_GNOME_GGV_PageList_1 == 'g') \
&& (TC_IMPL_TC_GNOME_GGV_PageList_2 == 'v') \
) && !defined(TC_DEF_TC_GNOME_GGV_PageList)
#define TC_DEF_TC_GNOME_GGV_PageList 1
static const CORBA_TypeCode anon_subtypes_array54[] =
   { (CORBA_TypeCode) & TC_CORBA_sequence_GNOME_GGV_Page_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_GNOME_GGV_PageList_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_alias,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array54,
   CORBA_OBJECT_NIL,
   "PageList",
   "IDL:GNOME/GGV/PageList:1.0",
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_GNOME_GGV_DocumentError_0 == 'G') \
&& (TC_IMPL_TC_GNOME_GGV_DocumentError_1 == 'g') \
&& (TC_IMPL_TC_GNOME_GGV_DocumentError_2 == 'v') \
) && !defined(TC_DEF_TC_GNOME_GGV_DocumentError)
#define TC_DEF_TC_GNOME_GGV_DocumentError 1
static const char *anon_subnames_array56[] = { "moreInfo" };
static const CORBA_TypeCode anon_subtypes_array57[] =
   { (CORBA_TypeCode) & TC_CORBA_string_struct };
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_GNOME_GGV_DocumentError_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_except,
   0,
   0,
   4,
   0,
   1,
   (CORBA_TypeCode *) anon_subtypes_array57,
   CORBA_OBJECT_NIL,
   "DocumentError",
   "IDL:GNOME/GGV/DocumentError:1.0",
   (char **) anon_subnames_array56,
   NULL,
   -1,
   0,
   0, 0
};
#endif
#if ( (TC_IMPL_TC_GNOME_GGV_PostScriptView_0 == 'G') \
&& (TC_IMPL_TC_GNOME_GGV_PostScriptView_1 == 'g') \
&& (TC_IMPL_TC_GNOME_GGV_PostScriptView_2 == 'v') \
) && !defined(TC_DEF_TC_GNOME_GGV_PostScriptView)
#define TC_DEF_TC_GNOME_GGV_PostScriptView 1
#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
const struct CORBA_TypeCode_struct TC_GNOME_GGV_PostScriptView_struct = {
   {&ORBit_TypeCode_epv, ORBIT_REFCOUNT_STATIC},
   CORBA_tk_objref,
   0,
   0,
   4,
   0,
   0,
   NULL,
   CORBA_OBJECT_NIL,
   "PostScriptView",
   "IDL:GNOME/GGV/PostScriptView:1.0",
   NULL,
   NULL,
   -1,
   0,
   0, 0
};
#endif

#ifndef ORBIT_IDL_C_IMODULE_Ggv
CORBA_unsigned_long GNOME_GGV_PostScriptView__classid = 0;
#endif

/* Interface type data */

/* Exceptions */
static CORBA_TypeCode GNOME_GGV_PostScriptView_getDocument__exceptinfo[] = {
   TC_GNOME_GGV_DocumentError,
   NULL
};
static ORBit_IArg GNOME_GGV_PostScriptView_getPages__arginfo[] = {
   {TC_GNOME_GGV_PageList, ORBit_I_ARG_IN, "pages"}
};

/* Exceptions */
static CORBA_TypeCode GNOME_GGV_PostScriptView_getPages__exceptinfo[] = {
   TC_GNOME_GGV_DocumentError,
   NULL
};

#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
ORBit_IMethod GNOME_GGV_PostScriptView__imethods[] = {
   {
    {0, 0, NULL, FALSE},
    {0, 0, NULL, FALSE},
    {1, 1, GNOME_GGV_PostScriptView_getDocument__exceptinfo, FALSE},
    TC_CORBA_string, "getDocument", 11,
    0}
   , {
      {1, 1, GNOME_GGV_PostScriptView_getPages__arginfo, FALSE},
      {0, 0, NULL, FALSE},
      {1, 1, GNOME_GGV_PostScriptView_getPages__exceptinfo, FALSE},
      TC_CORBA_string, "getPages", 8,
      0}
   , {
      {0, 0, NULL, FALSE},
      {0, 0, NULL, FALSE},
      {0, 0, NULL, FALSE},
      TC_void, "reload", 6,
      0}
   , {
      {0, 0, NULL, FALSE},
      {0, 0, NULL, FALSE},
      {0, 0, NULL, FALSE},
      TC_void, "close", 5,
      0}
};
static CORBA_string GNOME_GGV_PostScriptView__base_itypes[] = {
   "IDL:Bonobo/Unknown:1.0",
   "IDL:omg.org/CORBA/Object:1.0"
};

#ifdef ORBIT_IDL_C_IMODULE_Ggv
static
#endif
ORBit_IInterface GNOME_GGV_PostScriptView__iinterface = {
   TC_GNOME_GGV_PostScriptView, {4, 4, GNOME_GGV_PostScriptView__imethods,
				 FALSE},
   {2, 2, GNOME_GGV_PostScriptView__base_itypes, FALSE}
};
