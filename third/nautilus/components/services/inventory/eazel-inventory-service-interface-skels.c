/*
 * This file was generated by orbit-idl - DO NOT EDIT!
 */

#include <string.h>
#include "eazel-inventory-service-interface.h"

void
_ORBIT_skel_Trilobite_Eazel_InventoryUploadCallback_done_uploading
   (POA_Trilobite_Eazel_InventoryUploadCallback * _ORBIT_servant,
    GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
    void (*_impl_done_uploading) (PortableServer_Servant _servant,
				  const CORBA_boolean succeeded,
				  CORBA_Environment * ev))
{
   CORBA_boolean succeeded;

   {				/* demarshalling */
      guchar *_ORBIT_curptr;

      _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      if (giop_msg_conversion_needed(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer))) {
	 succeeded = *((CORBA_boolean *) _ORBIT_curptr);
      } else {
	 succeeded = *((CORBA_boolean *) _ORBIT_curptr);
      }
   }
   _impl_done_uploading(_ORBIT_servant, succeeded, ev);
}

void
_ORBIT_skel_Trilobite_Eazel_Inventory__get_enabled
   (POA_Trilobite_Eazel_Inventory * _ORBIT_servant,
    GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
    CORBA_boolean(*_impl__get_enabled) (PortableServer_Servant _servant,
					CORBA_Environment * ev))
{
   CORBA_boolean _ORBIT_retval;

   _ORBIT_retval = _impl__get_enabled(_ORBIT_servant, ev);
   {				/* marshalling */
      register GIOPSendBuffer *_ORBIT_send_buffer;

      _ORBIT_send_buffer =
	 giop_send_reply_buffer_use(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer)->
				    connection, NULL,
				    _ORBIT_recv_buffer->message.u.request.
				    request_id, ev->_major);
      if (_ORBIT_send_buffer) {
	 if (ev->_major == CORBA_NO_EXCEPTION) {
	    {
	       guchar *_ORBIT_t;

	       _ORBIT_t = alloca(sizeof(_ORBIT_retval));
	       memcpy(_ORBIT_t, &(_ORBIT_retval), sizeof(_ORBIT_retval));
	       giop_message_buffer_append_mem(GIOP_MESSAGE_BUFFER
					      (_ORBIT_send_buffer),
					      (_ORBIT_t),
					      sizeof(_ORBIT_retval));
	    }
	 } else
	    ORBit_send_system_exception(_ORBIT_send_buffer, ev);
	 giop_send_buffer_write(_ORBIT_send_buffer);
	 giop_send_buffer_unuse(_ORBIT_send_buffer);
      }
   }
}
void
_ORBIT_skel_Trilobite_Eazel_Inventory__set_enabled
   (POA_Trilobite_Eazel_Inventory * _ORBIT_servant,
    GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
    void (*_impl__set_enabled) (PortableServer_Servant _servant,
				const CORBA_boolean value,
				CORBA_Environment * ev))
{
   CORBA_boolean value;

   {				/* demarshalling */
      guchar *_ORBIT_curptr;

      _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      if (giop_msg_conversion_needed(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer))) {
	 value = *((CORBA_boolean *) _ORBIT_curptr);
      } else {
	 value = *((CORBA_boolean *) _ORBIT_curptr);
      }
   }
   _impl__set_enabled(_ORBIT_servant, value, ev);
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
void
_ORBIT_skel_Trilobite_Eazel_Inventory__get_machine_id
   (POA_Trilobite_Eazel_Inventory * _ORBIT_servant,
    GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
    CORBA_char * (*_impl__get_machine_id) (PortableServer_Servant _servant,
					   CORBA_Environment * ev))
{
   CORBA_char *_ORBIT_retval;

   _ORBIT_retval = _impl__get_machine_id(_ORBIT_servant, ev);
   {				/* marshalling */
      register GIOPSendBuffer *_ORBIT_send_buffer;

      _ORBIT_send_buffer =
	 giop_send_reply_buffer_use(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer)->
				    connection, NULL,
				    _ORBIT_recv_buffer->message.u.request.
				    request_id, ev->_major);
      if (_ORBIT_send_buffer) {
	 if (ev->_major == CORBA_NO_EXCEPTION) {
	    register CORBA_unsigned_long _ORBIT_tmpvar_0;
	    CORBA_unsigned_long _ORBIT_tmpvar_1;

	    _ORBIT_tmpvar_1 = strlen(_ORBIT_retval) + 1;
	    giop_message_buffer_do_alignment(GIOP_MESSAGE_BUFFER
					     (_ORBIT_send_buffer), 4);
	    {
	       guchar *_ORBIT_t;

	       _ORBIT_t = alloca(sizeof(_ORBIT_tmpvar_1));
	       memcpy(_ORBIT_t, &(_ORBIT_tmpvar_1), sizeof(_ORBIT_tmpvar_1));
	       giop_message_buffer_append_mem(GIOP_MESSAGE_BUFFER
					      (_ORBIT_send_buffer),
					      (_ORBIT_t),
					      sizeof(_ORBIT_tmpvar_1));
	    }
	    giop_message_buffer_append_mem(GIOP_MESSAGE_BUFFER
					   (_ORBIT_send_buffer),
					   (_ORBIT_retval),
					   sizeof(_ORBIT_retval
						  [_ORBIT_tmpvar_0]) *
					   _ORBIT_tmpvar_1);
	 } else
	    ORBit_send_system_exception(_ORBIT_send_buffer, ev);
	 giop_send_buffer_write(_ORBIT_send_buffer);
	 giop_send_buffer_unuse(_ORBIT_send_buffer);
      }
      if (ev->_major == CORBA_NO_EXCEPTION)
	 CORBA_free(_ORBIT_retval);
   }
}
void
_ORBIT_skel_Trilobite_Eazel_Inventory_upload(POA_Trilobite_Eazel_Inventory *
					     _ORBIT_servant,
					     GIOPRecvBuffer *
					     _ORBIT_recv_buffer,
					     CORBA_Environment * ev,
					     void (*_impl_upload)
					     (PortableServer_Servant _servant,
					      const
					      Trilobite_Eazel_InventoryUploadCallback
					      listener,
					      CORBA_Environment * ev))
{
   Trilobite_Eazel_InventoryUploadCallback listener;

   {				/* demarshalling */
      guchar *_ORBIT_curptr;

      _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      if (giop_msg_conversion_needed(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer))) {
	 GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur = _ORBIT_curptr;
	 listener =
	    ORBit_demarshal_object(_ORBIT_recv_buffer,
				   (((ORBit_ObjectKey
				      *) _ORBIT_servant->_private)->object->
				    orb));
	 _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      } else {
	 GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur = _ORBIT_curptr;
	 listener =
	    ORBit_demarshal_object(_ORBIT_recv_buffer,
				   (((ORBit_ObjectKey
				      *) _ORBIT_servant->_private)->object->
				    orb));
	 _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      }
   }
   _impl_upload(_ORBIT_servant, listener, ev);
   CORBA_Object_release(listener, ev);
}
static ORBitSkeleton
get_skel_Trilobite_Eazel_InventoryUploadCallback
   (POA_Trilobite_Eazel_InventoryUploadCallback * servant,
    GIOPRecvBuffer * _ORBIT_recv_buffer, gpointer * impl)
{
   gchar *opname = _ORBIT_recv_buffer->message.u.request.operation;

   switch (opname[0]) {
     case 'd':
	if (strcmp((opname + 1), "one_uploading"))
	   break;
	*impl =
	   (gpointer) servant->vepv->
	   Trilobite_Eazel_InventoryUploadCallback_epv->done_uploading;
	return (ORBitSkeleton)
	   _ORBIT_skel_Trilobite_Eazel_InventoryUploadCallback_done_uploading;
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
init_local_objref_Trilobite_Eazel_InventoryUploadCallback(CORBA_Object obj,
							  POA_Trilobite_Eazel_InventoryUploadCallback
							  * servant)
{
   obj->vepv[Bonobo_Unknown__classid] = servant->vepv->Bonobo_Unknown_epv;
   obj->vepv[Trilobite_Eazel_InventoryUploadCallback__classid] =
      servant->vepv->Trilobite_Eazel_InventoryUploadCallback_epv;
}

void
POA_Trilobite_Eazel_InventoryUploadCallback__init(PortableServer_Servant
						  servant,
						  CORBA_Environment * env)
{
   static const PortableServer_ClassInfo class_info =
      { (ORBit_impl_finder) &
get_skel_Trilobite_Eazel_InventoryUploadCallback, "IDL:Trilobite/Eazel/InventoryUploadCallback:1.0",
	 (ORBit_local_objref_init) &
	 init_local_objref_Trilobite_Eazel_InventoryUploadCallback };

   PortableServer_ServantBase__init(((PortableServer_ServantBase *) servant),
				    env);
   POA_Bonobo_Unknown__init(servant, env);
   ORBIT_OBJECT_KEY(((PortableServer_ServantBase *) servant)->_private)->
      class_info = (PortableServer_ClassInfo *) & class_info;
   if (!Trilobite_Eazel_InventoryUploadCallback__classid)
      Trilobite_Eazel_InventoryUploadCallback__classid =
	 ORBit_register_class(&class_info);
}

void
POA_Trilobite_Eazel_InventoryUploadCallback__fini(PortableServer_Servant
						  servant,
						  CORBA_Environment * env)
{
   POA_Bonobo_Unknown__fini(servant, env);
   PortableServer_ServantBase__fini(servant, env);
}

static ORBitSkeleton
get_skel_Trilobite_Eazel_Inventory(POA_Trilobite_Eazel_Inventory * servant,
				   GIOPRecvBuffer * _ORBIT_recv_buffer,
				   gpointer * impl)
{
   gchar *opname = _ORBIT_recv_buffer->message.u.request.operation;

   switch (opname[0]) {
     case '_':
	switch (opname[1]) {
	  case 'g':
	     switch (opname[2]) {
	       case 'e':
		  switch (opname[3]) {
		    case 't':
		       switch (opname[4]) {
			 case '_':
			    switch (opname[5]) {
			      case 'e':
				 if (strcmp((opname + 6), "nabled"))
				    break;
				 *impl =
				    (gpointer) servant->vepv->
				    Trilobite_Eazel_Inventory_epv->
				    _get_enabled;
				 return (ORBitSkeleton)
				    _ORBIT_skel_Trilobite_Eazel_Inventory__get_enabled;
				 break;
			      case 'm':
				 if (strcmp((opname + 6), "achine_id"))
				    break;
				 *impl =
				    (gpointer) servant->vepv->
				    Trilobite_Eazel_Inventory_epv->
				    _get_machine_id;
				 return (ORBitSkeleton)
				    _ORBIT_skel_Trilobite_Eazel_Inventory__get_machine_id;
				 break;
			      default:
				 break;
			    }
			    break;
			 default:
			    break;
		       }
		       break;
		    default:
		       break;
		  }
		  break;
	       default:
		  break;
	     }
	     break;
	  case 's':
	     if (strcmp((opname + 2), "et_enabled"))
		break;
	     *impl =
		(gpointer) servant->vepv->Trilobite_Eazel_Inventory_epv->
		_set_enabled;
	     return (ORBitSkeleton)
		_ORBIT_skel_Trilobite_Eazel_Inventory__set_enabled;
	     break;
	  default:
	     break;
	}
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
	switch (opname[1]) {
	  case 'n':
	     if (strcmp((opname + 2), "ref"))
		break;
	     *impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->unref;
	     return (ORBitSkeleton) _ORBIT_skel_Bonobo_Unknown_unref;
	     break;
	  case 'p':
	     if (strcmp((opname + 2), "load"))
		break;
	     *impl =
		(gpointer) servant->vepv->Trilobite_Eazel_Inventory_epv->
		upload;
	     return (ORBitSkeleton)
		_ORBIT_skel_Trilobite_Eazel_Inventory_upload;
	     break;
	  default:
	     break;
	}
	break;
     default:
	break;
   }
   return NULL;
}

static void
init_local_objref_Trilobite_Eazel_Inventory(CORBA_Object obj,
					    POA_Trilobite_Eazel_Inventory *
					    servant)
{
   obj->vepv[Bonobo_Unknown__classid] = servant->vepv->Bonobo_Unknown_epv;
   obj->vepv[Trilobite_Eazel_Inventory__classid] =
      servant->vepv->Trilobite_Eazel_Inventory_epv;
}

void
POA_Trilobite_Eazel_Inventory__init(PortableServer_Servant servant,
				    CORBA_Environment * env)
{
   static const PortableServer_ClassInfo class_info =
      { (ORBit_impl_finder) & get_skel_Trilobite_Eazel_Inventory,
	 "IDL:Trilobite/Eazel/Inventory:1.0",
	 (ORBit_local_objref_init) &

	 init_local_objref_Trilobite_Eazel_Inventory };
   PortableServer_ServantBase__init(((PortableServer_ServantBase *) servant),
				    env);
   POA_Bonobo_Unknown__init(servant, env);
   ORBIT_OBJECT_KEY(((PortableServer_ServantBase *) servant)->_private)->
      class_info = (PortableServer_ClassInfo *) & class_info;
   if (!Trilobite_Eazel_Inventory__classid)
      Trilobite_Eazel_Inventory__classid = ORBit_register_class(&class_info);
}

void
POA_Trilobite_Eazel_Inventory__fini(PortableServer_Servant servant,
				    CORBA_Environment * env)
{
   POA_Bonobo_Unknown__fini(servant, env);
   PortableServer_ServantBase__fini(servant, env);
}
