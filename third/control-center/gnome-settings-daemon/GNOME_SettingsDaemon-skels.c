/*
 * This file was generated by orbit-idl-2 - DO NOT EDIT!
 */

#include <string.h>
#define ORBIT2_STUBS_API
#include "GNOME_SettingsDaemon.h"

void
_ORBIT_skel_small_GNOME_SettingsDaemon_awake(POA_GNOME_SettingsDaemon *
					     _o_servant, gpointer _o_retval,
					     gpointer * _o_args,
					     CORBA_Context _o_ctx,
					     CORBA_Environment * _o_ev,
					     CORBA_boolean(*_impl_awake)
					     (PortableServer_Servant _servant,
					      const CORBA_char * service,
					      CORBA_Environment * ev))
{
   *(CORBA_boolean *) _o_retval =
      _impl_awake(_o_servant, *(const CORBA_char * *) _o_args[0], _o_ev);
}
static ORBitSmallSkeleton
get_skel_small_GNOME_SettingsDaemon(POA_GNOME_SettingsDaemon * servant,
				    const char *opname, gpointer * m_data,
				    gpointer * impl)
{
   switch (opname[0]) {
     case 'a':
	if (strcmp((opname + 1), "wake"))
	   break;
	*impl = (gpointer) servant->vepv->GNOME_SettingsDaemon_epv->awake;
	*m_data =
	   (gpointer) & GNOME_SettingsDaemon__iinterface.methods._buffer[0];
	return (ORBitSmallSkeleton)
	   _ORBIT_skel_small_GNOME_SettingsDaemon_awake;
	break;
     case 'q':
	if (strcmp((opname + 1), "ueryInterface"))
	   break;
	*impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->queryInterface;
	*m_data = (gpointer) & Bonobo_Unknown__iinterface.methods._buffer[2];
	return (ORBitSmallSkeleton)
	   _ORBIT_skel_small_Bonobo_Unknown_queryInterface;
	break;
     case 'r':
	if (strcmp((opname + 1), "ef"))
	   break;
	*impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->ref;
	*m_data = (gpointer) & Bonobo_Unknown__iinterface.methods._buffer[0];
	return (ORBitSmallSkeleton) _ORBIT_skel_small_Bonobo_Unknown_ref;
	break;
     case 'u':
	if (strcmp((opname + 1), "nref"))
	   break;
	*impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->unref;
	*m_data = (gpointer) & Bonobo_Unknown__iinterface.methods._buffer[1];
	return (ORBitSmallSkeleton) _ORBIT_skel_small_Bonobo_Unknown_unref;
	break;
     default:
	break;
   }
   return NULL;
}

void
POA_GNOME_SettingsDaemon__init(PortableServer_Servant servant,
			       CORBA_Environment * env)
{
   static PortableServer_ClassInfo class_info =
      { NULL, (ORBit_small_impl_finder) & get_skel_small_GNOME_SettingsDaemon,
"IDL:GNOME/SettingsDaemon:1.0", &GNOME_SettingsDaemon__classid, NULL, &GNOME_SettingsDaemon__iinterface };
   POA_GNOME_SettingsDaemon__vepv *fakevepv = NULL;

   if (((PortableServer_ServantBase *) servant)->vepv[0]->finalize == 0) {
      ((PortableServer_ServantBase *) servant)->vepv[0]->finalize =
	 POA_GNOME_SettingsDaemon__fini;
   }
   PortableServer_ServantBase__init(((PortableServer_ServantBase *) servant),
				    env);
   POA_Bonobo_Unknown__init(servant, env);
   ORBit_classinfo_register(&class_info);
   ORBIT_SERVANT_SET_CLASSINFO(servant, &class_info);

   if (!class_info.vepvmap) {
      class_info.vepvmap =
	 g_new0(ORBit_VepvIdx, GNOME_SettingsDaemon__classid + 1);
      class_info.vepvmap[Bonobo_Unknown__classid] =
	 (((char *) &(fakevepv->Bonobo_Unknown_epv)) -
	  ((char *) (fakevepv))) / sizeof(GFunc);
      class_info.vepvmap[GNOME_SettingsDaemon__classid] =
	 (((char *) &(fakevepv->GNOME_SettingsDaemon_epv)) -
	  ((char *) (fakevepv))) / sizeof(GFunc);
   }
}

void
POA_GNOME_SettingsDaemon__fini(PortableServer_Servant servant,
			       CORBA_Environment * env)
{
   POA_Bonobo_Unknown__fini(servant, env);
   PortableServer_ServantBase__fini(servant, env);
}
