/*
 * This file was generated by orbit-idl - DO NOT EDIT!
 */

#include <string.h>
#include "Mailer.h"

void
_ORBIT_skel_GNOME_Evolution_FolderInfo_getInfo(POA_GNOME_Evolution_FolderInfo
					       * _ORBIT_servant,
					       GIOPRecvBuffer *
					       _ORBIT_recv_buffer,
					       CORBA_Environment * ev,
					       void (*_impl_getInfo)
					       (PortableServer_Servant
						_servant,
						const CORBA_char * foldername,
						const Bonobo_Listener
						listener,
						CORBA_Environment * ev))
{
   CORBA_char *foldername;
   Bonobo_Listener listener;

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
	 foldername = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(foldername[_ORBIT_tmpvar_2]) * _ORBIT_tmpvar_3;
	 GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur = _ORBIT_curptr;
	 listener =
	    ORBit_demarshal_object(_ORBIT_recv_buffer,
				   (((ORBit_ObjectKey *) _ORBIT_servant->
				     _private)->object->orb));
	 _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      } else {
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_3 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 foldername = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(foldername[_ORBIT_tmpvar_2]) * _ORBIT_tmpvar_3;
	 GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur = _ORBIT_curptr;
	 listener =
	    ORBit_demarshal_object(_ORBIT_recv_buffer,
				   (((ORBit_ObjectKey *) _ORBIT_servant->
				     _private)->object->orb));
	 _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      }
   }
   _impl_getInfo(_ORBIT_servant, foldername, listener, ev);
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
      CORBA_Object_release((CORBA_Object) listener, ev);
   }
}
void
_ORBIT_skel_GNOME_Evolution_MailConfig_addAccount
   (POA_GNOME_Evolution_MailConfig * _ORBIT_servant,
    GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
    void (*_impl_addAccount) (PortableServer_Servant _servant,
			      const GNOME_Evolution_MailConfig_Account * acc,
			      CORBA_Environment * ev))
{
   GNOME_Evolution_MailConfig_Account acc;

   {				/* demarshalling */
      guchar *_ORBIT_curptr;
      register CORBA_unsigned_long _ORBIT_tmpvar_18;
      CORBA_unsigned_long _ORBIT_tmpvar_19;
      register CORBA_unsigned_long _ORBIT_tmpvar_20;
      CORBA_unsigned_long _ORBIT_tmpvar_21;
      register CORBA_unsigned_long _ORBIT_tmpvar_22;
      CORBA_unsigned_long _ORBIT_tmpvar_23;
      register CORBA_unsigned_long _ORBIT_tmpvar_24;
      CORBA_unsigned_long _ORBIT_tmpvar_25;
      register CORBA_unsigned_long _ORBIT_tmpvar_26;
      CORBA_unsigned_long _ORBIT_tmpvar_27;
      register CORBA_unsigned_long _ORBIT_tmpvar_28;
      CORBA_unsigned_long _ORBIT_tmpvar_29;
      register CORBA_unsigned_long _ORBIT_tmpvar_30;
      CORBA_unsigned_long _ORBIT_tmpvar_31;
      register CORBA_unsigned_long _ORBIT_tmpvar_32;
      CORBA_unsigned_long _ORBIT_tmpvar_33;
      register CORBA_unsigned_long _ORBIT_tmpvar_34;
      CORBA_unsigned_long _ORBIT_tmpvar_35;

      _ORBIT_curptr = GIOP_RECV_BUFFER(_ORBIT_recv_buffer)->cur;
      if (giop_msg_conversion_needed(GIOP_MESSAGE_BUFFER(_ORBIT_recv_buffer))) {
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (_ORBIT_tmpvar_19))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.name = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.name[_ORBIT_tmpvar_18]) * _ORBIT_tmpvar_19;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (_ORBIT_tmpvar_21))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.id.name = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.id.name[_ORBIT_tmpvar_20]) * _ORBIT_tmpvar_21;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (_ORBIT_tmpvar_23))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.id.address = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.id.address[_ORBIT_tmpvar_22]) * _ORBIT_tmpvar_23;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (_ORBIT_tmpvar_25))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.id.reply_to = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.id.reply_to[_ORBIT_tmpvar_24]) * _ORBIT_tmpvar_25;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (_ORBIT_tmpvar_27))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.id.organization = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.id.organization[_ORBIT_tmpvar_26]) * _ORBIT_tmpvar_27;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (_ORBIT_tmpvar_29))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.source.url = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.source.url[_ORBIT_tmpvar_28]) * _ORBIT_tmpvar_29;
	 acc.source.keep_on_server = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 acc.source.auto_check = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (acc.source.auto_check_time))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.source.save_passwd = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 acc.source.enabled = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (_ORBIT_tmpvar_31))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.transport.url = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.transport.url[_ORBIT_tmpvar_30]) * _ORBIT_tmpvar_31;
	 acc.transport.keep_on_server = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 acc.transport.auto_check = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (acc.transport.auto_check_time))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.transport.save_passwd = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 acc.transport.enabled = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (_ORBIT_tmpvar_33))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.drafts_folder_uri = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.drafts_folder_uri[_ORBIT_tmpvar_32]) *
	    _ORBIT_tmpvar_33;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 (*((guint32 *) & (_ORBIT_tmpvar_35))) =
	    GUINT32_SWAP_LE_BE(*((guint32 *) _ORBIT_curptr));
	 _ORBIT_curptr += 4;
	 acc.sent_folder_uri = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.sent_folder_uri[_ORBIT_tmpvar_34]) * _ORBIT_tmpvar_35;
      } else {
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_19 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.name = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.name[_ORBIT_tmpvar_18]) * _ORBIT_tmpvar_19;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_21 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.id.name = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.id.name[_ORBIT_tmpvar_20]) * _ORBIT_tmpvar_21;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_23 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.id.address = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.id.address[_ORBIT_tmpvar_22]) * _ORBIT_tmpvar_23;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_25 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.id.reply_to = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.id.reply_to[_ORBIT_tmpvar_24]) * _ORBIT_tmpvar_25;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_27 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.id.organization = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.id.organization[_ORBIT_tmpvar_26]) * _ORBIT_tmpvar_27;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_29 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.source.url = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.source.url[_ORBIT_tmpvar_28]) * _ORBIT_tmpvar_29;
	 acc.source.keep_on_server = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 acc.source.auto_check = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 acc.source.auto_check_time = *((CORBA_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.source.save_passwd = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 acc.source.enabled = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_31 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.transport.url = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.transport.url[_ORBIT_tmpvar_30]) * _ORBIT_tmpvar_31;
	 acc.transport.keep_on_server = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 acc.transport.auto_check = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 acc.transport.auto_check_time = *((CORBA_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.transport.save_passwd = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 acc.transport.enabled = *((CORBA_boolean *) _ORBIT_curptr);
	 _ORBIT_curptr += 1;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_33 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.drafts_folder_uri = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.drafts_folder_uri[_ORBIT_tmpvar_32]) *
	    _ORBIT_tmpvar_33;
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_35 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 acc.sent_folder_uri = (void *) _ORBIT_curptr;
	 _ORBIT_curptr +=
	    sizeof(acc.sent_folder_uri[_ORBIT_tmpvar_34]) * _ORBIT_tmpvar_35;
      }
   }
   _impl_addAccount(_ORBIT_servant, &(acc), ev);
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
_ORBIT_skel_GNOME_Evolution_MailConfig_removeAccount
   (POA_GNOME_Evolution_MailConfig * _ORBIT_servant,
    GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
    void (*_impl_removeAccount) (PortableServer_Servant _servant,
				 const CORBA_char * name,
				 CORBA_Environment * ev))
{
   CORBA_char *name;

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
	 name = (void *) _ORBIT_curptr;
	 _ORBIT_curptr += sizeof(name[_ORBIT_tmpvar_2]) * _ORBIT_tmpvar_3;
      } else {
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_3 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 name = (void *) _ORBIT_curptr;
	 _ORBIT_curptr += sizeof(name[_ORBIT_tmpvar_2]) * _ORBIT_tmpvar_3;
      }
   }
   _impl_removeAccount(_ORBIT_servant, name, ev);
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
_ORBIT_skel_GNOME_Evolution_MailFilter_addFilter
   (POA_GNOME_Evolution_MailFilter * _ORBIT_servant,
    GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
    void (*_impl_addFilter) (PortableServer_Servant _servant,
			     const CORBA_char * rule, CORBA_Environment * ev))
{
   CORBA_char *rule;

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
	 rule = (void *) _ORBIT_curptr;
	 _ORBIT_curptr += sizeof(rule[_ORBIT_tmpvar_2]) * _ORBIT_tmpvar_3;
      } else {
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_3 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 rule = (void *) _ORBIT_curptr;
	 _ORBIT_curptr += sizeof(rule[_ORBIT_tmpvar_2]) * _ORBIT_tmpvar_3;
      }
   }
   _impl_addFilter(_ORBIT_servant, rule, ev);
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
_ORBIT_skel_GNOME_Evolution_MailFilter_removeFilter
   (POA_GNOME_Evolution_MailFilter * _ORBIT_servant,
    GIOPRecvBuffer * _ORBIT_recv_buffer, CORBA_Environment * ev,
    void (*_impl_removeFilter) (PortableServer_Servant _servant,
				const CORBA_char * rule,
				CORBA_Environment * ev))
{
   CORBA_char *rule;

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
	 rule = (void *) _ORBIT_curptr;
	 _ORBIT_curptr += sizeof(rule[_ORBIT_tmpvar_2]) * _ORBIT_tmpvar_3;
      } else {
	 _ORBIT_curptr = ALIGN_ADDRESS(_ORBIT_curptr, 4);
	 _ORBIT_tmpvar_3 = *((CORBA_unsigned_long *) _ORBIT_curptr);
	 _ORBIT_curptr += 4;
	 rule = (void *) _ORBIT_curptr;
	 _ORBIT_curptr += sizeof(rule[_ORBIT_tmpvar_2]) * _ORBIT_tmpvar_3;
      }
   }
   _impl_removeFilter(_ORBIT_servant, rule, ev);
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
get_skel_GNOME_Evolution_FolderInfo(POA_GNOME_Evolution_FolderInfo * servant,
				    GIOPRecvBuffer * _ORBIT_recv_buffer,
				    gpointer * impl)
{
   gchar *opname = _ORBIT_recv_buffer->message.u.request.operation;

   switch (opname[0]) {
     case 'g':
	if (strcmp((opname + 1), "etInfo"))
	   break;
	*impl =
	   (gpointer) servant->vepv->GNOME_Evolution_FolderInfo_epv->getInfo;
	return (ORBitSkeleton) _ORBIT_skel_GNOME_Evolution_FolderInfo_getInfo;
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
init_local_objref_GNOME_Evolution_FolderInfo(CORBA_Object obj,
					     POA_GNOME_Evolution_FolderInfo *
					     servant)
{
   obj->vepv[Bonobo_Unknown__classid] = servant->vepv->Bonobo_Unknown_epv;
   obj->vepv[GNOME_Evolution_FolderInfo__classid] =
      servant->vepv->GNOME_Evolution_FolderInfo_epv;
}

void
POA_GNOME_Evolution_FolderInfo__init(PortableServer_Servant servant,
				     CORBA_Environment * env)
{
   static const PortableServer_ClassInfo class_info =
      { (ORBit_impl_finder) & get_skel_GNOME_Evolution_FolderInfo,
"IDL:GNOME/Evolution/FolderInfo:1.0", (ORBit_local_objref_init) & init_local_objref_GNOME_Evolution_FolderInfo };
   PortableServer_ServantBase__init(((PortableServer_ServantBase *) servant),
				    env);
   POA_Bonobo_Unknown__init(servant, env);
   ORBIT_OBJECT_KEY(((PortableServer_ServantBase *) servant)->_private)->
      class_info = (PortableServer_ClassInfo *) & class_info;
   if (!GNOME_Evolution_FolderInfo__classid)
      GNOME_Evolution_FolderInfo__classid = ORBit_register_class(&class_info);
}

void
POA_GNOME_Evolution_FolderInfo__fini(PortableServer_Servant servant,
				     CORBA_Environment * env)
{
   POA_Bonobo_Unknown__fini(servant, env);
   PortableServer_ServantBase__fini(servant, env);
}

static ORBitSkeleton
get_skel_GNOME_Evolution_MailConfig(POA_GNOME_Evolution_MailConfig * servant,
				    GIOPRecvBuffer * _ORBIT_recv_buffer,
				    gpointer * impl)
{
   gchar *opname = _ORBIT_recv_buffer->message.u.request.operation;

   switch (opname[0]) {
     case 'a':
	if (strcmp((opname + 1), "ddAccount"))
	   break;
	*impl =
	   (gpointer) servant->vepv->GNOME_Evolution_MailConfig_epv->
	   addAccount;
	return (ORBitSkeleton)
	   _ORBIT_skel_GNOME_Evolution_MailConfig_addAccount;
	break;
     case 'q':
	if (strcmp((opname + 1), "ueryInterface"))
	   break;
	*impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->queryInterface;
	return (ORBitSkeleton) _ORBIT_skel_Bonobo_Unknown_queryInterface;
	break;
     case 'r':
	switch (opname[1]) {
	  case 'e':
	     switch (opname[2]) {
	       case 'f':
		  if (strcmp((opname + 3), ""))
		     break;
		  *impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->ref;
		  return (ORBitSkeleton) _ORBIT_skel_Bonobo_Unknown_ref;
		  break;
	       case 'm':
		  if (strcmp((opname + 3), "oveAccount"))
		     break;
		  *impl =
		     (gpointer) servant->vepv->
		     GNOME_Evolution_MailConfig_epv->removeAccount;
		  return (ORBitSkeleton)
		     _ORBIT_skel_GNOME_Evolution_MailConfig_removeAccount;
		  break;
	       default:
		  break;
	     }
	     break;
	  default:
	     break;
	}
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
init_local_objref_GNOME_Evolution_MailConfig(CORBA_Object obj,
					     POA_GNOME_Evolution_MailConfig *
					     servant)
{
   obj->vepv[Bonobo_Unknown__classid] = servant->vepv->Bonobo_Unknown_epv;
   obj->vepv[GNOME_Evolution_MailConfig__classid] =
      servant->vepv->GNOME_Evolution_MailConfig_epv;
}

void
POA_GNOME_Evolution_MailConfig__init(PortableServer_Servant servant,
				     CORBA_Environment * env)
{
   static const PortableServer_ClassInfo class_info =
      { (ORBit_impl_finder) & get_skel_GNOME_Evolution_MailConfig,
"IDL:GNOME/Evolution/MailConfig:1.0", (ORBit_local_objref_init) & init_local_objref_GNOME_Evolution_MailConfig };
   PortableServer_ServantBase__init(((PortableServer_ServantBase *) servant),
				    env);
   POA_Bonobo_Unknown__init(servant, env);
   ORBIT_OBJECT_KEY(((PortableServer_ServantBase *) servant)->_private)->
      class_info = (PortableServer_ClassInfo *) & class_info;
   if (!GNOME_Evolution_MailConfig__classid)
      GNOME_Evolution_MailConfig__classid = ORBit_register_class(&class_info);
}

void
POA_GNOME_Evolution_MailConfig__fini(PortableServer_Servant servant,
				     CORBA_Environment * env)
{
   POA_Bonobo_Unknown__fini(servant, env);
   PortableServer_ServantBase__fini(servant, env);
}

static ORBitSkeleton
get_skel_GNOME_Evolution_MailFilter(POA_GNOME_Evolution_MailFilter * servant,
				    GIOPRecvBuffer * _ORBIT_recv_buffer,
				    gpointer * impl)
{
   gchar *opname = _ORBIT_recv_buffer->message.u.request.operation;

   switch (opname[0]) {
     case 'a':
	if (strcmp((opname + 1), "ddFilter"))
	   break;
	*impl =
	   (gpointer) servant->vepv->GNOME_Evolution_MailFilter_epv->
	   addFilter;
	return (ORBitSkeleton)
	   _ORBIT_skel_GNOME_Evolution_MailFilter_addFilter;
	break;
     case 'q':
	if (strcmp((opname + 1), "ueryInterface"))
	   break;
	*impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->queryInterface;
	return (ORBitSkeleton) _ORBIT_skel_Bonobo_Unknown_queryInterface;
	break;
     case 'r':
	switch (opname[1]) {
	  case 'e':
	     switch (opname[2]) {
	       case 'f':
		  if (strcmp((opname + 3), ""))
		     break;
		  *impl = (gpointer) servant->vepv->Bonobo_Unknown_epv->ref;
		  return (ORBitSkeleton) _ORBIT_skel_Bonobo_Unknown_ref;
		  break;
	       case 'm':
		  if (strcmp((opname + 3), "oveFilter"))
		     break;
		  *impl =
		     (gpointer) servant->vepv->
		     GNOME_Evolution_MailFilter_epv->removeFilter;
		  return (ORBitSkeleton)
		     _ORBIT_skel_GNOME_Evolution_MailFilter_removeFilter;
		  break;
	       default:
		  break;
	     }
	     break;
	  default:
	     break;
	}
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
init_local_objref_GNOME_Evolution_MailFilter(CORBA_Object obj,
					     POA_GNOME_Evolution_MailFilter *
					     servant)
{
   obj->vepv[Bonobo_Unknown__classid] = servant->vepv->Bonobo_Unknown_epv;
   obj->vepv[GNOME_Evolution_MailFilter__classid] =
      servant->vepv->GNOME_Evolution_MailFilter_epv;
}

void
POA_GNOME_Evolution_MailFilter__init(PortableServer_Servant servant,
				     CORBA_Environment * env)
{
   static const PortableServer_ClassInfo class_info =
      { (ORBit_impl_finder) & get_skel_GNOME_Evolution_MailFilter,
"IDL:GNOME/Evolution/MailFilter:1.0", (ORBit_local_objref_init) & init_local_objref_GNOME_Evolution_MailFilter };
   PortableServer_ServantBase__init(((PortableServer_ServantBase *) servant),
				    env);
   POA_Bonobo_Unknown__init(servant, env);
   ORBIT_OBJECT_KEY(((PortableServer_ServantBase *) servant)->_private)->
      class_info = (PortableServer_ClassInfo *) & class_info;
   if (!GNOME_Evolution_MailFilter__classid)
      GNOME_Evolution_MailFilter__classid = ORBit_register_class(&class_info);
}

void
POA_GNOME_Evolution_MailFilter__fini(PortableServer_Servant servant,
				     CORBA_Environment * env)
{
   POA_Bonobo_Unknown__fini(servant, env);
   PortableServer_ServantBase__fini(servant, env);
}
