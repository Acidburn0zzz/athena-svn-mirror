/*
 * This file was generated by orbit-idl - DO NOT EDIT!
 */

#include <string.h>
#include "Bonobo_Sample_Echo.h"

void
_ORBIT_skel_Bonobo_Sample_Echo_echo(POA_Bonobo_Sample_Echo * _ORBIT_servant,
				    GIOPRecvBuffer * _ORBIT_recv_buffer,
				    CORBA_Environment * ev,
				    void (*_impl_echo) (PortableServer_Servant
							_servant,
							const CORBA_char *
							message,
							CORBA_Environment *
							ev))
{
   CORBA_char *message;

   {				/* demarshalling */
      guchar *_ORBIT_curptr;
      register CORBA_unsigned_long _ORBIT_tmpvar_2;
      CORBA_unsigned_long _ORBIT_tmpvar_3;

      _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      if (giop_msg_conversion_needed(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer))) {
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 
	    (*((guint32 *) & (_ORBIT_tmpvar_3))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 message = (void *) _ORBIT_curptr;
	 _ORBIT_curptr += sizeof(message[_ORBIT_tmpvar_2]) * _ORBIT_tmpvar_3;
      } else {
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_3 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 message = (void *) _ORBIT_curptr;
	 _ORBIT_curptr += sizeof(message[_ORBIT_tmpvar_2]) * _ORBIT_tmpvar_3;
      }
   }
   _impl_echo(_ORBIT_servant, message, ev);
   {				/* marshalling */
      register GIOPSendBuffer *_ORBIT_send_buffer;

      _ORBIT_send_buffer =
	 giop_send_reply_buffer_use(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer)->
				    connection, NULL,
				    _ORBIT_recv_buffer->message.u.request.
				    request_id, ev->_major);
      if (_ORBIT_send_buffer) {
	 if (ev->_major == CORBA_NO_EXCEPTION) {
	 } else
	    ORBit_send_system_exception(_ORBIT_send_buffer, ev);
	 giop_send_buffer_write(_ORBIT_send_buffer);
	 giop_send_buffer_unuse(_ORBIT_send_buffer);
      }
   }
}
static ORBitSkeleton
get_skel_Bonobo_Sample_Echo(POA_Bonobo_Sample_Echo * servant,
			    GIOPRecvBuffer * _ORBIT_recv_buffer,
			    gpointer * impl)
{
   gchar *opname = _ORBIT_recv_buffer->message.u.request.operation;

   switch (opname[0]) {
     case 'e':
	if (strcmp((opname + 1), "cho"))
	   break;
	*impl = (gpointer) servant->vepv->Bonobo_Sample_Echo_epv->echo;
	return (ORBitSkeleton) _ORBIT_skel_Bonobo_Sample_Echo_echo;
	break;
     case 'q':
	if (strcmp((opname + 1), "ueryInterface"))
	   break;
	*impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->queryInterface;
	return (ORBitSkeleton) _ORBIT_skel_Bonobo_Unknown_queryInterface;
	break;
     case 'r':
	if (strcmp((opname + 1), "ef"))
	   break;
	*impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->ref;
	return (ORBitSkeleton) _ORBIT_skel_Bonobo_Unknown_ref;
	break;
     case 'u':
	if (strcmp((opname + 1), "nref"))
	   break;
	*impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->unref;
	return (ORBitSkeleton) _ORBIT_skel_Bonobo_Unknown_unref;
	break;
     default:
	break;
   }
   return NULL;
}

static void
init_local_objref_Bonobo_Sample_Echo(CORBA_Object obj,
				     POA_Bonobo_Sample_Echo * servant)
{
   obj->vepv[Bonobo_Unknown__classid] = servant->vepv->Bonobo_Unknown_epv;
   obj->vepv[Bonobo_Sample_Echo__classid] =
      servant->vepv->Bonobo_Sample_Echo_epv;
}

void
POA_Bonobo_Sample_Echo__init(PortableServer_Servant servant,
			     CORBA_Environment * env)
{
   static const PortableServer_ClassInfo class_info =
      { (ORBit_impl_finder) & get_skel_Bonobo_Sample_Echo,
	 "IDL:Bonobo/Sample/Echo:1.0",
	 (ORBit_local_objref_init) & init_local_objref_Bonobo_Sample_Echo };

   PortableServer_ServantBase__init(((PortableServer_ServantBase *) servant),
				    env);
   POA_Bonobo_Unknown__init(servant, env);
   ORBIT_OBJECT_KEY(((PortableServer_ServantBase *) servant)->_private)->
      class_info = (PortableServer_ClassInfo *) & class_info;
   if (!Bonobo_Sample_Echo__classid)
      Bonobo_Sample_Echo__classid = ORBit_register_class(&class_info);
}

void
POA_Bonobo_Sample_Echo__fini(PortableServer_Servant servant,
			     CORBA_Environment * env)
{
   POA_Bonobo_Unknown__fini(servant, env);
   PortableServer_ServantBase__fini(servant, env);
}
