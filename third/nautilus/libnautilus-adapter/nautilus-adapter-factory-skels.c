/*
 * This file was generated by orbit-idl - DO NOT EDIT!
 */

#include <string.h>
#include "nautilus-adapter-factory.h"

void
_ORBIT_skel_Nautilus_ComponentAdapterFactory_create_adapter
   (POA_Nautilus_ComponentAdapterFactory * _ORBIT_servant,
    GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
    Nautilus_View(*_impl_create_adapter) (PortableServer_Servant _servant,
					  const Bonobo_Unknown component,
					  CORBA_Environment * ev))
{
   Nautilus_View _ORBIT_retval;
   Bonobo_Unknown component;

   {				/* demarshalling */
      guchar *_ORBIT_curptr;

      _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      if (giop_msg_conversion_needed(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer))) {
	 GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur = _ORBIT_curptr;
	 component =
	    ORBit_demarshal_object(_ORBIT_recv_buffer,
				   (((ORBit_ObjectKey
				      *) _ORBIT_servant->_private)->object->
				    orb));
	 _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      } else {
	 GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur = _ORBIT_curptr;
	 component =
	    ORBit_demarshal_object(_ORBIT_recv_buffer,
				   (((ORBit_ObjectKey
				      *) _ORBIT_servant->_private)->object->
				    orb));
	 _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      }
   }
   _ORBIT_retval = _impl_create_adapter(_ORBIT_servant, component, ev);
   {				/* marshalling */
      register GIOPSendBuffer *_ORBIT_send_buffer;

      _ORBIT_send_buffer =
	 giop_send_reply_buffer_use(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer)->
				    connection, NULL,
				    _ORBIT_recv_buffer->message.u.request.
				    request_id, ev->_major);
      if (_ORBIT_send_buffer) {
	 if (ev->_major == CORBA_NO_EXCEPTION) {
	    ORBit_marshal_object(_ORBIT_send_buffer, _ORBIT_retval);
	 } else
	    ORBit_send_system_exception(_ORBIT_send_buffer, ev);
	 giop_send_buffer_write(_ORBIT_send_buffer);
	 giop_send_buffer_unuse(_ORBIT_send_buffer);
      }
      if (ev->_major == CORBA_NO_EXCEPTION)
	 CORBA_Object_release(_ORBIT_retval, ev);
      CORBA_Object_release(component, ev);
   }
}
static ORBitSkeleton
get_skel_Nautilus_ComponentAdapterFactory(POA_Nautilus_ComponentAdapterFactory
					  * servant,
					  GIOPRecvBuffer * _ORBIT_recv_buffer,
					  gpointer * impl)
{
   gchar *opname = _ORBIT_recv_buffer->message.u.request.operation;

   switch (opname[0]) {
     case 'c':
	if (strcmp((opname + 1), "reate_adapter"))
	   break;
	*impl =
	   (gpointer) servant->vepv->Nautilus_ComponentAdapterFactory_epv->
	   create_adapter;
	return (ORBitSkeleton)
	   _ORBIT_skel_Nautilus_ComponentAdapterFactory_create_adapter;
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
init_local_objref_Nautilus_ComponentAdapterFactory(CORBA_Object obj,
						   POA_Nautilus_ComponentAdapterFactory
						   * servant)
{
   obj->vepv[Bonobo_Unknown__classid] = servant->vepv->Bonobo_Unknown_epv;
   obj->vepv[Nautilus_ComponentAdapterFactory__classid] =
      servant->vepv->Nautilus_ComponentAdapterFactory_epv;
}

void
POA_Nautilus_ComponentAdapterFactory__init(PortableServer_Servant servant,
					   CORBA_Environment * env)
{
   static const PortableServer_ClassInfo class_info =
      { (ORBit_impl_finder) & get_skel_Nautilus_ComponentAdapterFactory,
	 "IDL:Nautilus/ComponentAdapterFactory:1.0",
	 (ORBit_local_objref_init) &
	 init_local_objref_Nautilus_ComponentAdapterFactory };

   PortableServer_ServantBase__init(((PortableServer_ServantBase *) servant),
				    env);
   POA_Bonobo_Unknown__init(servant, env);
   ORBIT_OBJECT_KEY(((PortableServer_ServantBase *) servant)->_private)->
      class_info = (PortableServer_ClassInfo *) & class_info;
   if (!Nautilus_ComponentAdapterFactory__classid)
      Nautilus_ComponentAdapterFactory__classid =
	 ORBit_register_class(&class_info);
}

void
POA_Nautilus_ComponentAdapterFactory__fini(PortableServer_Servant servant,
					   CORBA_Environment * env)
{
   POA_Bonobo_Unknown__fini(servant, env);
   PortableServer_ServantBase__fini(servant, env);
}
