/*
 * This file was generated by orbit-idl - DO NOT EDIT!
 */

#include <glib.h>
#define ORBIT_IDL_SERIAL 9
#include <orb/orbit.h>

#ifndef Spell_H
#define Spell_H 1
#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/** typedefs **/
#include <bonobo/Bonobo.h>
#if !defined(ORBIT_DECL_CORBA_sequence_CORBA_string) && !defined(_CORBA_sequence_CORBA_string_defined)
#define ORBIT_DECL_CORBA_sequence_CORBA_string 1
#define _CORBA_sequence_CORBA_string_defined 1
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_0 'S'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_1 'p'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_2 'e'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_3 'l'
#define ORBIT_IMPL_CORBA_sequence_CORBA_string_4 'l'
   typedef struct
   {
      CORBA_unsigned_long _maximum,
       _length;
      CORBA_char **_buffer;
      CORBA_boolean _release;
   }
   CORBA_sequence_CORBA_string;
   CORBA_char **CORBA_sequence_CORBA_string_allocbuf(CORBA_unsigned_long len);
#endif
#if !defined(TC_IMPL_TC_CORBA_sequence_CORBA_string_0)
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_0 'S'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_1 'p'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_2 'e'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_3 'l'
#define TC_IMPL_TC_CORBA_sequence_CORBA_string_4 'l'
   extern const struct CORBA_TypeCode_struct
      TC_CORBA_sequence_CORBA_string_struct;
#define TC_CORBA_sequence_CORBA_string ((CORBA_TypeCode)&TC_CORBA_sequence_CORBA_string_struct)
#endif
   extern CORBA_sequence_CORBA_string
      *CORBA_sequence_CORBA_string__alloc(void);
   extern gpointer CORBA_sequence_CORBA_string__free(gpointer mem, gpointer dat, CORBA_boolean free_strings);	/* ORBit internal use */
#if !defined(_GNOME_Spell_StringSeq_defined)
#define _GNOME_Spell_StringSeq_defined 1
   typedef CORBA_sequence_CORBA_string GNOME_Spell_StringSeq;
#if !defined(TC_IMPL_TC_GNOME_Spell_StringSeq_0)
#define TC_IMPL_TC_GNOME_Spell_StringSeq_0 'S'
#define TC_IMPL_TC_GNOME_Spell_StringSeq_1 'p'
#define TC_IMPL_TC_GNOME_Spell_StringSeq_2 'e'
#define TC_IMPL_TC_GNOME_Spell_StringSeq_3 'l'
#define TC_IMPL_TC_GNOME_Spell_StringSeq_4 'l'
   extern const struct CORBA_TypeCode_struct TC_GNOME_Spell_StringSeq_struct;
#define TC_GNOME_Spell_StringSeq ((CORBA_TypeCode)&TC_GNOME_Spell_StringSeq_struct)
#endif
   extern GNOME_Spell_StringSeq *GNOME_Spell_StringSeq__alloc(void);
   extern gpointer GNOME_Spell_StringSeq__free(gpointer mem, gpointer dat, CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(_GNOME_Spell_Language_defined)
#define _GNOME_Spell_Language_defined 1
   typedef struct
   {
      CORBA_char *name;
      CORBA_char *abrev;
   }
   GNOME_Spell_Language;

#if !defined(TC_IMPL_TC_GNOME_Spell_Language_0)
#define TC_IMPL_TC_GNOME_Spell_Language_0 'S'
#define TC_IMPL_TC_GNOME_Spell_Language_1 'p'
#define TC_IMPL_TC_GNOME_Spell_Language_2 'e'
#define TC_IMPL_TC_GNOME_Spell_Language_3 'l'
#define TC_IMPL_TC_GNOME_Spell_Language_4 'l'
   extern const struct CORBA_TypeCode_struct TC_GNOME_Spell_Language_struct;
#define TC_GNOME_Spell_Language ((CORBA_TypeCode)&TC_GNOME_Spell_Language_struct)
#endif
   extern GNOME_Spell_Language *GNOME_Spell_Language__alloc(void);
   extern gpointer GNOME_Spell_Language__free(gpointer mem, gpointer dat, CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(ORBIT_DECL_CORBA_sequence_GNOME_Spell_Language) && !defined(_CORBA_sequence_GNOME_Spell_Language_defined)
#define ORBIT_DECL_CORBA_sequence_GNOME_Spell_Language 1
#define _CORBA_sequence_GNOME_Spell_Language_defined 1
#define ORBIT_IMPL_CORBA_sequence_GNOME_Spell_Language_0 'S'
#define ORBIT_IMPL_CORBA_sequence_GNOME_Spell_Language_1 'p'
#define ORBIT_IMPL_CORBA_sequence_GNOME_Spell_Language_2 'e'
#define ORBIT_IMPL_CORBA_sequence_GNOME_Spell_Language_3 'l'
#define ORBIT_IMPL_CORBA_sequence_GNOME_Spell_Language_4 'l'
   typedef struct
   {
      CORBA_unsigned_long _maximum,
       _length;
      GNOME_Spell_Language *_buffer;
      CORBA_boolean _release;
   }
   CORBA_sequence_GNOME_Spell_Language;
   GNOME_Spell_Language
      *CORBA_sequence_GNOME_Spell_Language_allocbuf(CORBA_unsigned_long len);
#endif
#if !defined(TC_IMPL_TC_CORBA_sequence_GNOME_Spell_Language_0)
#define TC_IMPL_TC_CORBA_sequence_GNOME_Spell_Language_0 'S'
#define TC_IMPL_TC_CORBA_sequence_GNOME_Spell_Language_1 'p'
#define TC_IMPL_TC_CORBA_sequence_GNOME_Spell_Language_2 'e'
#define TC_IMPL_TC_CORBA_sequence_GNOME_Spell_Language_3 'l'
#define TC_IMPL_TC_CORBA_sequence_GNOME_Spell_Language_4 'l'
   extern const struct CORBA_TypeCode_struct
      TC_CORBA_sequence_GNOME_Spell_Language_struct;
#define TC_CORBA_sequence_GNOME_Spell_Language ((CORBA_TypeCode)&TC_CORBA_sequence_GNOME_Spell_Language_struct)
#endif
   extern CORBA_sequence_GNOME_Spell_Language
      *CORBA_sequence_GNOME_Spell_Language__alloc(void);
   extern gpointer CORBA_sequence_GNOME_Spell_Language__free(gpointer mem, gpointer dat, CORBA_boolean free_strings);	/* ORBit internal use */
#if !defined(_GNOME_Spell_LanguageSeq_defined)
#define _GNOME_Spell_LanguageSeq_defined 1
   typedef CORBA_sequence_GNOME_Spell_Language GNOME_Spell_LanguageSeq;
#if !defined(TC_IMPL_TC_GNOME_Spell_LanguageSeq_0)
#define TC_IMPL_TC_GNOME_Spell_LanguageSeq_0 'S'
#define TC_IMPL_TC_GNOME_Spell_LanguageSeq_1 'p'
#define TC_IMPL_TC_GNOME_Spell_LanguageSeq_2 'e'
#define TC_IMPL_TC_GNOME_Spell_LanguageSeq_3 'l'
#define TC_IMPL_TC_GNOME_Spell_LanguageSeq_4 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Spell_LanguageSeq_struct;
#define TC_GNOME_Spell_LanguageSeq ((CORBA_TypeCode)&TC_GNOME_Spell_LanguageSeq_struct)
#endif
   extern GNOME_Spell_LanguageSeq *GNOME_Spell_LanguageSeq__alloc(void);
   extern gpointer GNOME_Spell_LanguageSeq__free(gpointer mem, gpointer dat, CORBA_boolean free_strings);	/* ORBit internal use */
#endif
#if !defined(ORBIT_DECL_GNOME_Spell_Dictionary) && !defined(_GNOME_Spell_Dictionary_defined)
#define ORBIT_DECL_GNOME_Spell_Dictionary 1
#define _GNOME_Spell_Dictionary_defined 1
#define GNOME_Spell_Dictionary__free CORBA_Object__free
   typedef CORBA_Object GNOME_Spell_Dictionary;
   extern CORBA_unsigned_long GNOME_Spell_Dictionary__classid;
#if !defined(TC_IMPL_TC_GNOME_Spell_Dictionary_0)
#define TC_IMPL_TC_GNOME_Spell_Dictionary_0 'S'
#define TC_IMPL_TC_GNOME_Spell_Dictionary_1 'p'
#define TC_IMPL_TC_GNOME_Spell_Dictionary_2 'e'
#define TC_IMPL_TC_GNOME_Spell_Dictionary_3 'l'
#define TC_IMPL_TC_GNOME_Spell_Dictionary_4 'l'
   extern const struct CORBA_TypeCode_struct TC_GNOME_Spell_Dictionary_struct;
#define TC_GNOME_Spell_Dictionary ((CORBA_TypeCode)&TC_GNOME_Spell_Dictionary_struct)
#endif
#endif
#define ex_GNOME_Spell_Dictionary_Error "IDL:GNOME/Spell/Dictionary/Error:1.0"
   void _ORBIT_GNOME_Spell_Dictionary_Error_demarshal(GIOPRecvBuffer *
						      _ORBIT_recv_buffer,
						      CORBA_Environment * ev);
   void _ORBIT_GNOME_Spell_Dictionary_Error_marshal(GIOPSendBuffer *
						    _ORBIT_send_buffer,
						    CORBA_Environment * ev);
#if !defined(_GNOME_Spell_Dictionary_Error_defined)
#define _GNOME_Spell_Dictionary_Error_defined 1
   typedef struct
   {
      CORBA_char *error;
   }
   GNOME_Spell_Dictionary_Error;

#if !defined(TC_IMPL_TC_GNOME_Spell_Dictionary_Error_0)
#define TC_IMPL_TC_GNOME_Spell_Dictionary_Error_0 'S'
#define TC_IMPL_TC_GNOME_Spell_Dictionary_Error_1 'p'
#define TC_IMPL_TC_GNOME_Spell_Dictionary_Error_2 'e'
#define TC_IMPL_TC_GNOME_Spell_Dictionary_Error_3 'l'
#define TC_IMPL_TC_GNOME_Spell_Dictionary_Error_4 'l'
   extern const struct CORBA_TypeCode_struct
      TC_GNOME_Spell_Dictionary_Error_struct;
#define TC_GNOME_Spell_Dictionary_Error ((CORBA_TypeCode)&TC_GNOME_Spell_Dictionary_Error_struct)
#endif
   extern GNOME_Spell_Dictionary_Error
      *GNOME_Spell_Dictionary_Error__alloc(void);
   extern gpointer GNOME_Spell_Dictionary_Error__free(gpointer mem, gpointer dat, CORBA_boolean free_strings);	/* ORBit internal use */
#endif

/** POA structures **/
   typedef struct
   {
      void *_private;
      GNOME_Spell_LanguageSeq *(*getLanguages) (PortableServer_Servant
						_servant,
						CORBA_Environment * ev);
      void (*setLanguage) (PortableServer_Servant _servant,
			   const CORBA_char * language,
			   CORBA_Environment * ev);
       CORBA_boolean(*checkWord) (PortableServer_Servant _servant,
				  const CORBA_char * word,
				  CORBA_Environment * ev);
      GNOME_Spell_StringSeq *(*getSuggestions) (PortableServer_Servant
						_servant,
						const CORBA_char * word,
						CORBA_Environment * ev);
      void (*addWordToSession) (PortableServer_Servant _servant,
				const CORBA_char * word,
				CORBA_Environment * ev);
      void (*addWordToPersonal) (PortableServer_Servant _servant,
				 const CORBA_char * word,
				 CORBA_Environment * ev);
      void (*setCorrection) (PortableServer_Servant _servant,
			     const CORBA_char * word,
			     const CORBA_char * replacement,
			     CORBA_Environment * ev);
   }
   POA_GNOME_Spell_Dictionary__epv;
   typedef struct
   {
      PortableServer_ServantBase__epv *_base_epv;
      POA_Bonobo_Unknown__epv *Bonobo_Unknown_epv;
      POA_GNOME_Spell_Dictionary__epv *GNOME_Spell_Dictionary_epv;
   }
   POA_GNOME_Spell_Dictionary__vepv;
   typedef struct
   {
      void *_private;
      POA_GNOME_Spell_Dictionary__vepv *vepv;
   }
   POA_GNOME_Spell_Dictionary;
   extern void POA_GNOME_Spell_Dictionary__init(PortableServer_Servant
						servant,
						CORBA_Environment * ev);
   extern void POA_GNOME_Spell_Dictionary__fini(PortableServer_Servant
						servant,
						CORBA_Environment * ev);

/** prototypes **/
#define GNOME_Spell_Dictionary_ref Bonobo_Unknown_ref
#define GNOME_Spell_Dictionary_unref Bonobo_Unknown_unref
#define GNOME_Spell_Dictionary_queryInterface Bonobo_Unknown_queryInterface
   GNOME_Spell_LanguageSeq
      *GNOME_Spell_Dictionary_getLanguages(GNOME_Spell_Dictionary _obj,
					   CORBA_Environment * ev);
   void GNOME_Spell_Dictionary_setLanguage(GNOME_Spell_Dictionary _obj,
					   const CORBA_char * language,
					   CORBA_Environment * ev);
   CORBA_boolean GNOME_Spell_Dictionary_checkWord(GNOME_Spell_Dictionary _obj,
						  const CORBA_char * word,
						  CORBA_Environment * ev);
   GNOME_Spell_StringSeq
      *GNOME_Spell_Dictionary_getSuggestions(GNOME_Spell_Dictionary _obj,
					     const CORBA_char * word,
					     CORBA_Environment * ev);
   void GNOME_Spell_Dictionary_addWordToSession(GNOME_Spell_Dictionary _obj,
						const CORBA_char * word,
						CORBA_Environment * ev);
   void GNOME_Spell_Dictionary_addWordToPersonal(GNOME_Spell_Dictionary _obj,
						 const CORBA_char * word,
						 CORBA_Environment * ev);
   void GNOME_Spell_Dictionary_setCorrection(GNOME_Spell_Dictionary _obj,
					     const CORBA_char * word,
					     const CORBA_char * replacement,
					     CORBA_Environment * ev);

   void
      _ORBIT_skel_GNOME_Spell_Dictionary_getLanguages
      (POA_GNOME_Spell_Dictionary * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       GNOME_Spell_LanguageSeq *
       (*_impl_getLanguages) (PortableServer_Servant _servant,
			      CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Spell_Dictionary_setLanguage
      (POA_GNOME_Spell_Dictionary * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_setLanguage) (PortableServer_Servant _servant,
				  const CORBA_char * language,
				  CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Spell_Dictionary_checkWord(POA_GNOME_Spell_Dictionary
						   * _ORBIT_servant,
						   GIOPRecvBuffer *
						   _ORBIT_recv_buffer,
						   CORBA_Environment * ev,
						   CORBA_boolean
						   (*_impl_checkWord)
						   (PortableServer_Servant
						    _servant,
						    const CORBA_char * word,
						    CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Spell_Dictionary_getSuggestions
      (POA_GNOME_Spell_Dictionary * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       GNOME_Spell_StringSeq *
       (*_impl_getSuggestions) (PortableServer_Servant _servant,
				const CORBA_char * word,
				CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Spell_Dictionary_addWordToSession
      (POA_GNOME_Spell_Dictionary * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_addWordToSession) (PortableServer_Servant _servant,
				       const CORBA_char * word,
				       CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Spell_Dictionary_addWordToPersonal
      (POA_GNOME_Spell_Dictionary * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_addWordToPersonal) (PortableServer_Servant _servant,
					const CORBA_char * word,
					CORBA_Environment * ev));
   void
      _ORBIT_skel_GNOME_Spell_Dictionary_setCorrection
      (POA_GNOME_Spell_Dictionary * _ORBIT_servant,
       GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
       void (*_impl_setCorrection) (PortableServer_Servant _servant,
				    const CORBA_char * word,
				    const CORBA_char * replacement,
				    CORBA_Environment * ev));
#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif
#undef ORBIT_IDL_SERIAL
