#ifndef __ORBIT_ADAPTOR_H__
#define __ORBIT_ADAPTOR_H__

#include <glib.h>

G_BEGIN_DECLS

#ifdef ORBIT2_INTERNAL_API

void                ORBit_handle_request            (CORBA_ORB          orb, 
						     GIOPRecvBuffer    *recv_buffer);

void                ORBit_small_handle_request      (ORBit_OAObject     adaptor_obj,
						     CORBA_Identifier   opname,
						     gpointer           ret,
						     gpointer          *args, 
						     CORBA_Context      ctx,
						     GIOPRecvBuffer    *recv_buffer,
						     CORBA_Environment *ev);

gboolean            ORBit_OAObject_is_active        (ORBit_OAObject     adaptor_obj);

ORBit_ObjectKey    *ORBit_OAObject_object_to_objkey (ORBit_OAObject     adaptor_obj);

void                ORBit_OAObject_invoke           (ORBit_OAObject     adaptor_obj,
						     gpointer           ret,
						     gpointer          *args,
						     CORBA_Context      ctx,
						     gpointer           data,
						     CORBA_Environment *ev);
#endif /* ORBIT2_INTERNAL_API */

/*
 * ORBit_OAObject
 */
#if defined(ORBIT2_INTERNAL_API) || defined (ORBIT2_STUBS_API)

#ifndef ORBIT2_INTERNAL_API
#define GIOPRecvBuffer void
#endif

typedef gboolean            (*ORBitStateCheckFunc) (ORBit_OAObject     adaptor_obj);

typedef ORBit_ObjectKey    *(*ORBitKeyGenFunc)     (ORBit_OAObject     adaptor_obj);

typedef void                (*ORBitInvokeFunc)     (ORBit_OAObject     adaptor_obj,
						    gpointer           ret,
						    gpointer          *args,
						    CORBA_Context      ctx,
						    gpointer           data, 
						    CORBA_Environment *ev);

typedef void                (*ORBitReqFunc)        (ORBit_OAObject     adaptor_obj,
						    CORBA_Identifier   opname,
						    gpointer           ret,
						    gpointer          *args,
						    CORBA_Context      ctx,
						    GIOPRecvBuffer    *recv_buffer,
						    CORBA_Environment *ev);

typedef enum {
	ORBIT_ADAPTOR_POA = 1 << 0
} ORBit_Adaptor_type;

struct ORBit_OAObject_Interface_type {
	ORBit_Adaptor_type  adaptor_type;

	ORBitStateCheckFunc is_active;
	ORBitKeyGenFunc     object_to_objkey;
	ORBitInvokeFunc     invoke;
	ORBitReqFunc        handle_request;
};

typedef struct ORBit_OAObject_Interface_type *ORBit_OAObject_Interface;

struct ORBit_OAObject_type {
	struct ORBit_RootObject_struct parent;

	CORBA_Object                   objref;

	ORBit_OAObject_Interface       interface;
};

#endif /* defined(ORBIT2_INTERNAL_API) || defined (ORBIT2_STUBS_API) */

/*
 * ORBit_ObjectAdaptor
 */

#ifdef ORBIT2_INTERNAL_API

typedef struct ORBit_ObjectAdaptor_type *ORBit_ObjectAdaptor;
typedef CORBA_sequence_CORBA_octet       ORBit_AdaptorKey;

typedef void (*ORBitReqHandlerFunc) (ORBit_ObjectAdaptor         adaptor,
				     GIOPRecvBuffer             *recv_buffer,
				     ORBit_ObjectKey            *objkey);

struct ORBit_ObjectAdaptor_type {
	struct ORBit_RootObject_struct parent;

	ORBitReqHandlerFunc            handle_request;

	ORBit_AdaptorKey               adaptor_key;
};

int ORBit_adaptor_setup (ORBit_ObjectAdaptor adaptor, CORBA_ORB orb);

#endif /* ORBIT2_INTERNAL_API */

G_END_DECLS

#endif /* __ORBIT_ADAPTOR_H__ */
