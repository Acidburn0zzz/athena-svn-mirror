#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <orbit/orbit.h>
#include <orb-core-export.h>
#include <orbit-debug.h>

#include "../util/orbit-purify.h"
#include "../orb-core/orbit-debug.h"
#include "../GIOP/giop-debug.h"
#include "poa-macros.h"

#include "orbit-poa.h"

static PortableServer_Servant ORBit_POA_ServantManager_use_servant(
				     PortableServer_POA poa,
				     ORBit_POAObject pobj,
				     CORBA_Identifier opname,
				     PortableServer_ServantLocator_Cookie *the_cookie,
				     PortableServer_ObjectId *oid,
				     CORBA_Environment *ev );

static void ORBit_POA_ServantManager_unuse_servant(
				       PortableServer_POA poa,
				       ORBit_POAObject pobj,
				       CORBA_Identifier opname,
				       PortableServer_ServantLocator_Cookie cookie,
				       PortableServer_ObjectId *oid,
				       PortableServer_Servant servant,
				       CORBA_Environment *ev );

static void ORBit_POA_handle_request (PortableServer_POA          poa,
				      GIOPRecvBuffer             *recv_buffer,
				      ORBit_ObjectKey            *objkey);


static void               ORBit_POAObject_invoke           (ORBit_POAObject    pobj,
							    gpointer           ret,
							    gpointer          *args,
							    CORBA_Context      ctx,
							    gpointer           data,
							    CORBA_Environment *ev);

static void               ORBit_POAObject_handle_request    (ORBit_POAObject    pobj,
							     CORBA_Identifier   opname,
							     gpointer           ret,
							     gpointer          *args,
							     CORBA_Context      ctx,
							     GIOPRecvBuffer    *recv_buffer,
							     CORBA_Environment *ev);

static void               ORBit_POA_deactivate_object       (PortableServer_POA  poa,
							     ORBit_POAObject     pobj,
							     CORBA_boolean       do_etherealize,
							     CORBA_boolean       is_cleanup);

static GHashTable *ORBit_class_assignments = NULL;
static guint ORBit_class_assignment_counter = 0;

/* PortableServer_Current interface */
static void
ORBit_POACurrent_free_fn (ORBit_RootObject obj_in)
{
	PortableServer_Current poacur = (PortableServer_Current) obj_in;

	ORBit_RootObject_release_T (poacur->orb);
	poacur->orb = NULL;

	p_free (poacur, struct PortableServer_Current_type);
}

static const ORBit_RootObject_Interface ORBit_POACurrent_epv = {
	ORBIT_ROT_POACURRENT,
	ORBit_POACurrent_free_fn
};

PortableServer_Current
ORBit_POACurrent_new (CORBA_ORB orb)
{
	PortableServer_Current poacur;

	poacur = (PortableServer_Current) 
		g_new0 (struct PortableServer_Current_type, 1);

	ORBit_RootObject_init (&poacur->parent, &ORBit_POACurrent_epv);

	poacur->orb = ORBit_RootObject_duplicate (orb);

	return ORBit_RootObject_duplicate (poacur);
}

static ORBit_POAObject
ORBit_POACurrent_get_object (PortableServer_Current  obj,
			     CORBA_Environment      *ev)
{
	g_assert (obj && obj->parent.interface->type == ORBIT_ROT_POACURRENT);

	poa_exception_val_if_fail (obj->orb->current_invocations != NULL,
				   ex_PortableServer_Current_NoContext,
				   CORBA_OBJECT_NIL);

	return (ORBit_POAObject) obj->orb->current_invocations->data;
}

PortableServer_ClassInfo *
ORBit_classinfo_lookup (const char *type_id)
{
	if (!ORBit_class_assignments)
		return NULL;

	return g_hash_table_lookup (ORBit_class_assignments, type_id);
}

void
ORBit_classinfo_register (PortableServer_ClassInfo *ci)
{
	if (*(ci->class_id) != 0)
		return; /* already registered! */

	/* This needs to be pre-increment - we don't want to give out
	 * classid 0, because (a) that is reserved for the base Object class
	 * (b) all the routines allocate a new id if the variable
	 * storing their ID == 0
	 */
	*(ci->class_id) = ++ORBit_class_assignment_counter;

	if (!ORBit_class_assignments)
		ORBit_class_assignments = g_hash_table_new (g_str_hash, g_str_equal);

	g_hash_table_insert (ORBit_class_assignments,
			     (gpointer) ci->class_name, ci);
}

static void
check_child_poa_inuse (char               *name,
		       PortableServer_POA  poa,
		       gboolean           *is_inuse)
{
	if (ORBit_POA_is_inuse (poa, CORBA_TRUE, NULL))
		*is_inuse = TRUE;
}

static void
check_object_inuse (PortableServer_ObjectId *oid,
		    ORBit_POAObject          pobj, 
		    gboolean                *is_inuse)
{
	if (pobj->use_cnt > 0)
		*is_inuse = TRUE;
}

gboolean
ORBit_POA_is_inuse (PortableServer_POA  poa,
		    CORBA_boolean       consider_children,
		    CORBA_Environment  *ev)
{
	gboolean is_inuse = FALSE;

	if (poa->use_cnt > 0) 
		return CORBA_TRUE;

	if (consider_children && poa->child_poas)
		g_hash_table_foreach (poa->child_poas,
				      (GHFunc) check_child_poa_inuse,
				     &is_inuse);

	if (!is_inuse && poa->oid_to_obj_map)
		g_hash_table_foreach (poa->oid_to_obj_map,
				      (GHFunc) check_object_inuse,
				      &is_inuse);

	return is_inuse;
}

static PortableServer_ObjectId*
ORBit_POA_new_system_objid (PortableServer_POA poa)
{
	PortableServer_ObjectId *objid;

	g_assert (IS_SYSTEM_ID (poa));

	objid = PortableServer_ObjectId__alloc ();
	objid->_length  = objid->_maximum = sizeof (CORBA_long) + ORBIT_OBJECT_ID_LEN;
	objid->_buffer  = PortableServer_ObjectId_allocbuf (objid->_length);
	objid->_release = CORBA_TRUE;

	ORBit_genuid_buffer (objid->_buffer + sizeof (CORBA_long),
			     ORBIT_OBJECT_ID_LEN, ORBIT_GENUID_OBJECT_ID);
	*((CORBA_long *) objid->_buffer) = ++(poa->next_sysid);

	return objid;
}

static ORBit_ObjectKey*
ORBit_POAObject_object_to_objkey (ORBit_POAObject pobj)
{
	ORBit_ObjectAdaptor  adaptor;
	ORBit_ObjectKey     *objkey;
	guchar              *mem;

	g_return_val_if_fail (pobj != NULL, NULL);

	adaptor = (ORBit_ObjectAdaptor) pobj->poa;

	objkey           = CORBA_sequence_CORBA_octet__alloc ();
	objkey->_length  = adaptor->adaptor_key._length + pobj->object_id->_length;
	objkey->_maximum = objkey->_length;
	objkey->_buffer  = CORBA_sequence_CORBA_octet_allocbuf (objkey->_length);
	objkey->_release = CORBA_TRUE;

 	mem = (guchar *) objkey->_buffer;
 	memcpy (mem, adaptor->adaptor_key._buffer, adaptor->adaptor_key._length);
 
	mem += adaptor->adaptor_key._length;
 	memcpy (mem, pobj->object_id->_buffer, pobj->object_id->_length);

	return objkey;
}

static ORBit_POAObject
ORBit_POA_object_id_lookup (PortableServer_POA             poa,
			    const PortableServer_ObjectId *oid)
{
	return g_hash_table_lookup (poa->oid_to_obj_map, oid);
}

static void
ORBit_POA_set_life (PortableServer_POA poa, 
		    CORBA_boolean      etherealize_objects,
		    int                action_do)
{
	if ((poa->life_flags &
	     (ORBit_LifeF_DeactivateDo |
	      ORBit_LifeF_DestroyDo)) == 0) {

		if (etherealize_objects)
			poa->life_flags |= ORBit_LifeF_DoEtherealize;
	}
	poa->life_flags |= action_do;
}

static void
ORBit_POA_add_child (PortableServer_POA poa,
		     PortableServer_POA child)
{
	if (!child)
		return;

	child->parent_poa = ORBit_RootObject_duplicate (poa);
	g_hash_table_insert (poa->child_poas, child->name, child);
}

static void
ORBit_POA_remove_child (PortableServer_POA poa,
			PortableServer_POA child_poa)
{
	if (!child_poa->parent_poa)
		return;

	g_assert (child_poa->parent_poa == poa);

	g_hash_table_remove (poa->child_poas, child_poa->name);

	child_poa->parent_poa = NULL;

	ORBit_RootObject_release (poa);
}

static gboolean
ORBit_POA_destroy (PortableServer_POA  poa,
		   CORBA_boolean       etherealize_objects,
		   CORBA_Environment  *ev)
{
	GPtrArray *adaptors;
	int        numobjs;
	int        i;

	ORBit_POA_set_life (poa, etherealize_objects, ORBit_LifeF_DestroyDo);

	if (poa->life_flags & ORBit_LifeF_Destroyed)
		return TRUE;	/* already did it */

	if (poa->life_flags & (ORBit_LifeF_Deactivating|ORBit_LifeF_Destroying))
		return FALSE;	/* recursion */

	poa->life_flags |= ORBit_LifeF_Destroying;

	adaptors = poa->orb->adaptors;

	/* Destroying the children is tricky, b/c they may die
	 * while we are traversing. We traverse over the
	 * ORB's global list (rather than poa->child_poas) 
	 * to avoid walking into dead children. */
	for (i = 0; i < adaptors->len; i++) {
		PortableServer_POA cpoa = g_ptr_array_index (adaptors, i);

		if (cpoa && cpoa->parent_poa == poa)
			ORBit_POA_destroy (cpoa, etherealize_objects, ev);
	}

	poa->default_servant = NULL;

	if (g_hash_table_size (poa->child_poas) > 0 || poa->use_cnt ||
	    !ORBit_POA_deactivate (poa, etherealize_objects, ev) ) {
		poa->life_flags &= ~ORBit_LifeF_Destroying;

		return FALSE;
	}

	ORBit_POAManager_unregister_poa (poa->poa_manager, poa);

	ORBit_POA_remove_child (poa->parent_poa, poa);

	g_ptr_array_index (adaptors, poa->poa_id) = NULL;
	poa->poa_id = -1;

	/* each objref holds a POAObj, and each POAObj holds a ref 
	 * to the POA. In addition, the app can hold open refs
	 * to the POA itself. */
	numobjs = poa->oid_to_obj_map ? g_hash_table_size (poa->oid_to_obj_map) : 0;
	g_assert (((ORBit_RootObject) poa)->refs > numobjs);

	poa->life_flags |= ORBit_LifeF_Destroyed;
	poa->life_flags &= ~ORBit_LifeF_Destroying;
	ORBit_RootObject_release (poa);

	return CORBA_TRUE;
}

typedef struct TraverseInfo {
	PortableServer_POA poa;
	gboolean           in_use;
	gboolean           do_etherealize;
} TraverseInfo;

static void
traverse_cb (PortableServer_ObjectId *oid,
	     ORBit_POAObject          pobj, 
	     TraverseInfo            *info)
{
	if (pobj->use_cnt > 0)
		info->in_use = TRUE;

	ORBit_POA_deactivate_object (info->poa, pobj, info->do_etherealize, TRUE);
}

static gboolean
remove_cb (PortableServer_ObjectId *oid,
	   ORBit_POAObject          pobj,
	   gpointer                 dummy)
{
	if (pobj->life_flags & ORBit_LifeF_Destroyed) {
		p_free (pobj, struct ORBit_POAObject_type);
		return TRUE;
	}

	return FALSE;
}

CORBA_boolean
ORBit_POA_deactivate (PortableServer_POA poa,
		      CORBA_boolean      etherealize_objects,
		      CORBA_Environment *ev)
{
	CORBA_boolean done = CORBA_TRUE;

	ORBit_POA_set_life (poa, etherealize_objects, ORBit_LifeF_DeactivateDo);

	if (poa->life_flags & ORBit_LifeF_Deactivated)
		return TRUE;	/* already did it */

	if (poa->life_flags & ORBit_LifeF_Deactivating)
		return FALSE;	/* recursion */

	poa->life_flags |= ORBit_LifeF_Deactivating;

	/* bounce all pending requested (OBJECT_NOT_EXIST
	 * exceptions raised); none should get requeued. */
	ORBit_POA_handle_held_requests (poa);
	g_assert (poa->held_requests == 0);

	if (IS_RETAIN (poa)) {
		TraverseInfo info;

		info.poa            = poa;
		info.in_use         = FALSE;
		info.do_etherealize = (poa->life_flags & ORBit_LifeF_DoEtherealize);

		g_assert (poa->oid_to_obj_map);

		g_hash_table_foreach (
			poa->oid_to_obj_map, (GHFunc) traverse_cb, &info);
		g_hash_table_foreach_remove (
			poa->oid_to_obj_map, (GHRFunc) remove_cb, NULL);

		done = !info.in_use;
	}

	if (done)
		poa->life_flags |= ORBit_LifeF_Deactivated;
	poa->life_flags &= ~ORBit_LifeF_Deactivating;

	return done;
}

/*
 * ORBit_POA_handle_held_requests:
 * @poa:
 *
 * Handle any requests that may been have been queued because the
 * POAManager was in a HOLDING state. Note that if the POAManger
 * is still in the HOLDING state, or is put into the HOLDING
 * state by one of the methods invoked, requests may be re-queued.
 */
void
ORBit_POA_handle_held_requests (PortableServer_POA poa)
{
	GSList *requests;
	GSList *l;

	requests = poa->held_requests;
	poa->held_requests = NULL;

	for (l = requests; l; l = l->next)
		ORBit_handle_request (poa->orb, l->data);

	g_slist_free (requests);
}

static void
ORBit_POA_free_fn (ORBit_RootObject obj)
{
	ORBit_ObjectAdaptor adaptor = (ORBit_ObjectAdaptor) obj;
	PortableServer_POA  poa = (PortableServer_POA) obj;

	if (adaptor->adaptor_key._buffer)
		ORBit_free_T (adaptor->adaptor_key._buffer);

	if (poa->oid_to_obj_map)
		g_hash_table_destroy (poa->oid_to_obj_map);

	if (poa->child_poas)
		g_hash_table_destroy (poa->child_poas);

	if (poa->name)
		g_free (poa->name);

	ORBit_RootObject_release_T (poa->orb);
	ORBit_RootObject_release_T (poa->poa_manager);

	p_free (poa, struct PortableServer_POA_type);
}

static const ORBit_RootObject_Interface ORBit_POA_epv = {
	ORBIT_ROT_ADAPTOR,
	ORBit_POA_free_fn
};

static guint
ORBit_ObjectId_sysid_hash (const PortableServer_ObjectId *object_id)
{
	return *(guint *) object_id->_buffer;
}

static guint
ORBit_sequence_CORBA_octet_hash (const PortableServer_ObjectId *object_id)
{
	const char *start;
	const char *end;
  	const char *p;
  	guint       g, h = 0;

	start = (char *) object_id->_buffer;
	end   = (char *) object_id->_buffer + object_id->_length;

  	for (p = start; p < end; p++) {
    		h = ( h << 4 ) + *p;
		g = h & 0xf0000000;

    		if (g != 0) {
      			h = h ^ (g >> 24);
      			h = h ^ g;
    		}
  	}

  	return h;
}

static gboolean
ORBit_sequence_CORBA_octet_equal (const PortableServer_ObjectId *o1,
				  const PortableServer_ObjectId *o2)
{
	return (o1->_length == o2->_length &&
		!memcmp (o1->_buffer, o2->_buffer, o1->_length));
}

static void
ORBit_POA_set_policy (PortableServer_POA  poa,
		      CORBA_Policy        obj)
{
	struct CORBA_Policy_type *policy = (struct CORBA_Policy_type *) obj;

	switch (policy->type) {
	case PortableServer_THREAD_POLICY_ID:
		poa->p_thread = policy->value;
		break;
	case PortableServer_LIFESPAN_POLICY_ID:
		poa->p_lifespan = policy->value;
		break;
	case PortableServer_ID_UNIQUENESS_POLICY_ID:
		poa->p_id_uniqueness = policy->value;
		break;
	case PortableServer_ID_ASSIGNMENT_POLICY_ID:
		poa->p_id_assignment = policy->value;
		break;
	case PortableServer_IMPLICIT_ACTIVATION_POLICY_ID:
		poa->p_implicit_activation = policy->value;
		break;
	case PortableServer_SERVANT_RETENTION_POLICY_ID:
		poa->p_servant_retention = policy->value;
		break;
	case PortableServer_REQUEST_PROCESSING_POLICY_ID:
		poa->p_request_processing = policy->value;
		break;
	default:
		g_warning ("Unknown policy type, cannot set it on this POA");
		break;
	}
}

static void
ORBit_POA_set_policies (PortableServer_POA      poa,
			const CORBA_PolicyList *policies,
			CORBA_Environment      *ev)
{
	CORBA_unsigned_long i;

	poa->p_thread              = PortableServer_ORB_CTRL_MODEL;
	poa->p_lifespan            = PortableServer_TRANSIENT;
	poa->p_id_uniqueness       = PortableServer_UNIQUE_ID;
	poa->p_id_assignment       = PortableServer_SYSTEM_ID;
	poa->p_servant_retention   = PortableServer_RETAIN;
	poa->p_request_processing  = PortableServer_USE_ACTIVE_OBJECT_MAP_ONLY;
	poa->p_implicit_activation = PortableServer_NO_IMPLICIT_ACTIVATION;

	for (i = 0; policies && i < policies->_length; i++)
		ORBit_POA_set_policy (poa, policies->_buffer[i]);

	g_assert (ev->_major == CORBA_NO_EXCEPTION);

	poa_exception_if_fail (!(IS_NON_RETAIN (poa) && IS_USE_ACTIVE_OBJECT_MAP_ONLY (poa)), 
			       ex_PortableServer_POA_InvalidPolicy);

	poa_exception_if_fail (!(IS_USE_DEFAULT_SERVANT (poa) && IS_UNIQUE_ID (poa)),
			       ex_PortableServer_POA_InvalidPolicy);

	poa_exception_if_fail (!(IS_IMPLICIT_ACTIVATION (poa) && (IS_USER_ID (poa) || IS_NON_RETAIN (poa))),
			       ex_PortableServer_POA_InvalidPolicy);
}

static PortableServer_POA
ORBit_POA_new (CORBA_ORB                  orb,
	       const CORBA_char          *adaptor_name,
	       PortableServer_POAManager  manager,
	       const CORBA_PolicyList    *policies,
	       CORBA_Environment         *ev)
{
	PortableServer_POA   poa;
	ORBit_ObjectAdaptor  adaptor;
  
	poa = g_new0 (struct PortableServer_POA_type, 1);

	ORBit_RootObject_init ((ORBit_RootObject) poa, &ORBit_POA_epv);
	/* released in ORBit_POA_destroy */
	ORBit_RootObject_duplicate (poa);

	ORBit_POA_set_policies (poa, policies, ev);
	if (ev->_major != CORBA_NO_EXCEPTION) {
		ORBit_RootObject_release (poa);
		return CORBA_OBJECT_NIL;
	}

	if (!manager)
		manager = ORBit_POAManager_new (orb);

	poa->poa_manager = ORBit_RootObject_duplicate (manager);

	adaptor = (ORBit_ObjectAdaptor) poa;
	adaptor->handle_request = (ORBitReqHandlerFunc) ORBit_POA_handle_request;

	poa->name       = g_strdup (adaptor_name);
	poa->child_poas = g_hash_table_new (g_str_hash, g_str_equal);
	poa->orb        = ORBit_RootObject_duplicate (orb);
	poa->poa_id     = ORBit_adaptor_setup (adaptor, orb);

	if (IS_SYSTEM_ID (poa))
		poa->oid_to_obj_map = g_hash_table_new (
			(GHashFunc) ORBit_ObjectId_sysid_hash,
			(GEqualFunc) ORBit_sequence_CORBA_octet_equal);
	else /* USER_ID */
		poa->oid_to_obj_map = g_hash_table_new (
			(GHashFunc) ORBit_sequence_CORBA_octet_hash,
			(GEqualFunc) ORBit_sequence_CORBA_octet_equal);

	ORBit_POAManager_register_poa (manager, poa);

	return ORBit_RootObject_duplicate (poa);
}

static CORBA_Object
ORBit_POA_obj_to_ref (PortableServer_POA  poa,
		      ORBit_POAObject     pobj,
		      const CORBA_char   *intf,
		      CORBA_Environment  *ev)
{
	PortableServer_ObjectId *oid;
	const char              *type_id = intf;

	g_assert (pobj && !pobj->base.objref);

	if (!type_id) {
		g_assert (pobj->servant);
		type_id = ORBIT_SERVANT_TO_CLASSINFO (pobj->servant)->class_name;
	}

	g_assert (type_id != NULL);

	oid = pobj->object_id;

	pobj->base.objref = ORBit_objref_new (poa->poa_manager->orb,
					      g_quark_from_string (type_id));

	/* released by CORBA_Object_release */
	pobj->base.objref->adaptor_obj = ORBit_RootObject_duplicate (pobj);

	return ORBit_RootObject_duplicate (pobj->base.objref);
}

PortableServer_POA
ORBit_POA_setup_root (CORBA_ORB orb, CORBA_Environment *ev)
{
	PortableServer_POA poa;
	CORBA_Policy       policybuf[1];
	CORBA_PolicyList   policies = {1, 1, (CORBA_Object *) policybuf, CORBA_FALSE};

	policies._buffer [0] = (CORBA_Policy)
		PortableServer_POA_create_implicit_activation_policy (
			NULL, PortableServer_IMPLICIT_ACTIVATION, ev);

	poa = ORBit_POA_new (orb, "RootPOA", CORBA_OBJECT_NIL, &policies, ev);
 
	CORBA_Policy_destroy (policies._buffer [0], ev);
	CORBA_Object_release (policies._buffer [0], ev);

	return poa;
}

static gboolean
ORBit_POAObject_is_active (ORBit_POAObject pobj)
{
	if (pobj && pobj->servant)
		return TRUE;

	return FALSE;
}

/* POAObject RootObject stuff */
static void
ORBit_POAObject_release_cb (ORBit_RootObject robj)
{
	ORBit_POAObject    pobj = (ORBit_POAObject) robj;
	PortableServer_POA poa = pobj->poa;
	PortableServer_ObjectId *object_id;
 
	/* object *must* be deactivated */
	g_assert (pobj->servant == NULL);

	object_id = pobj->object_id;
	pobj->object_id = NULL;

	/*
	 * Don't want to remove from oid_to_obj_map if we 
	 * are currently traversing across it !
	 * Just mark it as destroyed
	 */
	if ((poa->life_flags & ORBit_LifeF_Deactivating) == 0) {
		g_hash_table_remove (poa->oid_to_obj_map, object_id);
		p_free (robj, struct ORBit_POAObject_type);
	} else
		pobj->life_flags = ORBit_LifeF_Destroyed;

	object_id->_release = CORBA_TRUE;
	ORBit_free_T (object_id);

	ORBit_RootObject_release_T (poa);
}

static ORBit_RootObject_Interface ORBit_POAObject_if = {
	ORBIT_ROT_OAOBJECT,
	ORBit_POAObject_release_cb
};

static struct 
ORBit_OAObject_Interface_type ORBit_POAObject_methods = {
	ORBIT_ADAPTOR_POA,
	(ORBitStateCheckFunc) ORBit_POAObject_is_active,
	(ORBitKeyGenFunc)     ORBit_POAObject_object_to_objkey,
	(ORBitInvokeFunc)     ORBit_POAObject_invoke,
	(ORBitReqFunc)        ORBit_POAObject_handle_request
};

/*
 *    If USER_ID policy, {oid} must be non-NULL.
 *  If SYSTEM_ID policy, {oid} must ether be NULL, or must have
 *  been previously created by the POA. If the user passes in
 *  a bogus oid under SYSTEM_ID, we will assert or segfault. This
 *  is allowed by the CORBA spec.
 */
static ORBit_POAObject
ORBit_POA_create_object (PortableServer_POA             poa,
			 const PortableServer_ObjectId *objid,
			 CORBA_Environment             *ev)
{
	ORBit_POAObject newobj;

	newobj = g_new0 (struct ORBit_POAObject_type, 1);
	ORBit_RootObject_init ((ORBit_RootObject)newobj, &ORBit_POAObject_if);

	/* released in ORBit_POAObject_release_cb */
	newobj->poa = ORBit_RootObject_duplicate (poa);

	((ORBit_OAObject)newobj)->interface = &ORBit_POAObject_methods;

	if (poa->p_id_assignment == PortableServer_SYSTEM_ID) {
		if (objid) {
			g_assert (objid->_length ==
				  sizeof (CORBA_unsigned_long) +
				  ORBIT_OBJECT_ID_LEN);

			newobj->object_id          = PortableServer_ObjectId__alloc ();
			newobj->object_id->_length = objid->_length;
			newobj->object_id->_buffer = PortableServer_ObjectId_allocbuf (objid->_length);
			newobj->object_id->_release = CORBA_TRUE;

			memcpy (newobj->object_id->_buffer, objid->_buffer, objid->_length);
		}
		else
			newobj->object_id = ORBit_POA_new_system_objid (poa);
	} else {
		newobj->object_id           = PortableServer_ObjectId__alloc ();
		newobj->object_id->_length  = objid->_length;
		newobj->object_id->_buffer  = PortableServer_ObjectId_allocbuf (objid->_length);
		newobj->object_id->_release = CORBA_TRUE;

		memcpy(newobj->object_id->_buffer, objid->_buffer, objid->_length);
	}

	g_hash_table_insert (poa->oid_to_obj_map, newobj->object_id, newobj);

	return newobj;
}

/*
 *    Normally this is called for normal servants in RETAIN mode. 
 *  However, it may also be invoked on the default servant when
 *  it is installed. In this later case, it may be either RETAIN
 *  or NON_RETAIN.
 */
static void
ORBit_POA_activate_object (PortableServer_POA          poa, 
			   ORBit_POAObject             pobj,
			   PortableServer_ServantBase *servant, 
			   CORBA_Environment          *ev) 
{
	PortableServer_ClassInfo *class_info;

	g_assert (pobj->servant == NULL);
	g_assert ((poa->life_flags & ORBit_LifeF_DeactivateDo) == 0);
	g_assert (pobj->use_cnt == 0);

	class_info = ORBIT_SERVANT_TO_CLASSINFO (servant);
	g_assert (class_info->vepvmap);
	pobj->vepvmap_cache = class_info->vepvmap;

	pobj->servant = servant;

	pobj->next = (ORBit_POAObject) servant->_private;
	servant->_private = pobj;

	/* released in ORBit_POA_deactivate_object */
	ORBit_RootObject_duplicate (pobj);
}

/*
 * Note that this doesn't necessarily remove the object from
 * the oid_to_obj_map; it just removes knowledge of the servant.
 * If the object is currently in use (servicing a request),
 * etherialization and memory release will occur later.
 */
static void
ORBit_POA_deactivate_object (PortableServer_POA poa,
			     ORBit_POAObject    pobj,
			     CORBA_boolean      do_etherealize,
			     CORBA_boolean      is_cleanup)
{
	PortableServer_ServantBase *servant = pobj->servant;

	if (!servant) /* deactivation done, or in progress */
		return;

	if (do_etherealize && !(pobj->life_flags & ORBit_LifeF_DeactivateDo))
		pobj->life_flags |= ORBit_LifeF_DoEtherealize;
	
	if (is_cleanup)
		pobj->life_flags |= ORBit_LifeF_IsCleanup;

	if (pobj->use_cnt > 0) {
		pobj->life_flags |= ORBit_LifeF_DeactivateDo;
		pobj->life_flags |= ORBit_LifeF_NeedPostInvoke;
		return;
	}
	pobj->servant = NULL;

	if ((ORBit_POAObject) servant->_private == pobj)
		servant->_private = pobj->next;
	else {
		ORBit_POAObject l = (ORBit_POAObject) servant->_private;

		for (; l && l->next != pobj; l = l->next);

		g_assert (l != NULL && l->next == pobj);

		l->next = pobj->next;
	}
	pobj->next = NULL;

	if (pobj->life_flags & ORBit_LifeF_DoEtherealize) {
		CORBA_Environment env, *ev = &env;

		CORBA_exception_init (ev);

		pobj->use_cnt++; /* prevent re-activation */
		if (poa->p_request_processing == PortableServer_USE_SERVANT_MANAGER) {
			POA_PortableServer_ServantActivator      *sm;
			POA_PortableServer_ServantActivator__epv *epv;

			sm = (POA_PortableServer_ServantActivator *) poa->servant_manager;
			epv = sm->vepv->PortableServer_ServantActivator_epv;

			epv->etherealize (sm, pobj->object_id, poa,
					  servant,
					  pobj->life_flags & ORBit_LifeF_IsCleanup,
					  /* remaining_activations */ CORBA_FALSE,
					  ev);
		}
		{
			PortableServer_ServantBase__epv *epv = servant->vepv[0];
			/* In theory, the finalize fnc should always be non-NULL;
			 * however, for backward compat. and general extended
			 * applications we dont insist on it. */
			if (epv && epv->finalize)
				epv->finalize (servant, ev);
		}
		pobj->use_cnt--; /* allow re-activation */
		g_assert (ev->_major == 0);

		CORBA_exception_free (ev);
	}

	pobj->life_flags &= ~(ORBit_LifeF_DeactivateDo |
			      ORBit_LifeF_IsCleanup |
			      ORBit_LifeF_DoEtherealize);

	ORBit_RootObject_release (pobj);
}

struct ORBit_POA_invoke_data {
	ORBitSmallSkeleton small_skel;
	gpointer           imp;
};

static void
ORBit_POAObject_invoke (ORBit_POAObject    pobj,
			gpointer           ret,
			gpointer          *args,
			CORBA_Context      ctx,
			gpointer           data,
			CORBA_Environment *ev)
{
	struct ORBit_POA_invoke_data *invoke_data = (struct ORBit_POA_invoke_data *) data;

	invoke_data->small_skel (pobj->servant, ret, args, ctx, ev, invoke_data->imp);
}

/*
 * giop_recv_buffer_return_sys_exception:
 * @recv_buffer:
 * @m_data:
 * @ev:
 *
 * Return a system exception in @ev to the client. If @m_data
 * is not nil, it used to determine whether the call is a
 * oneway and, hence, whether to return the exception. If
 * @m_data is nil, we are not far enough along in the processing
 * of the reqeust to be able to determine if this is a oneway
 * method.
 */
void
ORBit_recv_buffer_return_sys_exception (GIOPRecvBuffer    *recv_buffer,
					CORBA_Environment *ev)
{
	GIOPSendBuffer *send_buffer;

	if (!recv_buffer) /* In Proc */
		return;

	g_return_if_fail (ev->_major == CORBA_SYSTEM_EXCEPTION);

	send_buffer = giop_send_buffer_use_reply (
		recv_buffer->connection->giop_version,
		giop_recv_buffer_get_request_id (recv_buffer),
		ev->_major);

	ORBit_send_system_exception (send_buffer, ev);

	tprintf ("Return exception:\n");
	do_giop_dump_send (send_buffer);
	giop_send_buffer_write (send_buffer, recv_buffer->connection, FALSE);
	giop_send_buffer_unuse (send_buffer);
}

static void
return_exception (GIOPRecvBuffer    *recv_buffer,
		  ORBit_IMethod     *m_data,
		  CORBA_Environment *ev)
{
	if (!recv_buffer) /* In Proc */
		return;

	g_return_if_fail (ev->_major == CORBA_SYSTEM_EXCEPTION);

	if (m_data && m_data->flags & ORBit_I_METHOD_1_WAY) {
		tprintf ("A serious exception occured on a oneway method");
		return;
	}

	ORBit_recv_buffer_return_sys_exception (recv_buffer, ev);
}

/*
 * If invoked in the local case, recv_buffer == NULL.
 * If invoked in the remote cse, ret = args = ctx == NULL.
 */
static void
ORBit_POAObject_handle_request (ORBit_POAObject    pobj,
				CORBA_Identifier   opname,
				gpointer           ret,
				gpointer          *args,
				CORBA_Context      ctx,
				GIOPRecvBuffer    *recv_buffer,
				CORBA_Environment *ev)
{
	PortableServer_POA                   poa = pobj->poa;
	PortableServer_ServantLocator_Cookie cookie;
	PortableServer_ObjectId             *oid;
	PortableServer_ClassInfo            *klass;
	ORBit_IMethod                       *m_data = NULL;
	ORBitSkeleton                        skel = NULL;
	ORBitSmallSkeleton                   small_skel = NULL;
	gpointer                             imp = NULL;

	if (!poa || !poa->poa_manager)
		CORBA_exception_set_system (
			ev, ex_CORBA_OBJECT_NOT_EXIST, 
			CORBA_COMPLETED_NO);
	else {
		switch (poa->poa_manager->state) {

		case PortableServer_POAManager_HOLDING:
			if (recv_buffer) {
				g_warning ("POAManager in holding state. "
					   "Queueing '%s' method request", opname);
						
				poa->held_requests = g_slist_prepend (
					poa->held_requests, recv_buffer);
			}
			else
				CORBA_exception_set_system (
					ev, ex_CORBA_TRANSIENT,
					CORBA_COMPLETED_NO);
			return;
			
		case PortableServer_POAManager_DISCARDING:
			CORBA_exception_set_system (
				ev, ex_CORBA_TRANSIENT,
				CORBA_COMPLETED_NO);
			return;

		case PortableServer_POAManager_INACTIVE:
			CORBA_exception_set_system (
				ev, ex_CORBA_OBJ_ADAPTER,
				CORBA_COMPLETED_NO);
			return;

		case PortableServer_POAManager_ACTIVE:
			break;

		default:
			g_assert_not_reached ();
			break;
		}
	}
	oid = pobj->object_id;

	if (!pobj->servant) {
		switch (poa->p_request_processing) {

		case PortableServer_USE_ACTIVE_OBJECT_MAP_ONLY:
			CORBA_exception_set_system (
				ev, ex_CORBA_OBJECT_NOT_EXIST, 
				CORBA_COMPLETED_NO);
			break;

		case PortableServer_USE_DEFAULT_SERVANT:
			ORBit_POA_activate_object (
				poa, pobj, poa->default_servant, ev);
			break;

		case PortableServer_USE_SERVANT_MANAGER:
			ORBit_POA_ServantManager_use_servant (
				poa, pobj, opname,  &cookie, oid, ev);
			break;
		default:
			g_assert_not_reached();
			break;
		}
	}

	pobj->use_cnt++;
	poa->orb->current_invocations =
		g_slist_prepend (poa->orb->current_invocations, pobj);
	
	if (ev->_major == CORBA_NO_EXCEPTION && !pobj->servant)
		CORBA_exception_set_system (
			ev, ex_CORBA_OBJECT_NOT_EXIST, 
			CORBA_COMPLETED_NO);

	if (ev->_major != CORBA_NO_EXCEPTION) {
		return_exception (recv_buffer, m_data, ev);
		goto clean_out;
	}

	klass = ORBIT_SERVANT_TO_CLASSINFO (pobj->servant);

	if (klass->relay_call)
		skel = klass->relay_call (
			pobj->servant, recv_buffer, &imp);

	if (!skel && klass->small_relay_call)
		small_skel = klass->small_relay_call (
			pobj->servant, opname, (gpointer *)&m_data, &imp);

	if (!skel && !small_skel)
		small_skel = get_small_skel_CORBA_Object (
			pobj->servant, opname,
			(gpointer *)&m_data, &imp);
	
	if ((!small_skel && !skel) || !imp) {
		if (!imp && (small_skel || skel)) {
			tprintf ("'%s' not implemented on %p",
				 opname, pobj);
			CORBA_exception_set_system (
				ev, ex_CORBA_NO_IMPLEMENT,
				CORBA_COMPLETED_NO);
		} else {
			tprintf ("Bad operation '%s' on %p",
				 opname, pobj);
			CORBA_exception_set_system (
				ev, ex_CORBA_BAD_OPERATION,
				CORBA_COMPLETED_NO);
		}
	}

	if (ev->_major != CORBA_NO_EXCEPTION) {
		return_exception (recv_buffer, m_data, ev);
		goto clean_out;
	}

	if (skel)
		skel (pobj->servant, recv_buffer, ev, imp);

	else if (recv_buffer) {
		struct ORBit_POA_invoke_data invoke_data;

		invoke_data.small_skel = small_skel;
		invoke_data.imp        = imp;

		ORBit_small_invoke_adaptor (
			(ORBit_OAObject) pobj, recv_buffer,
			m_data, &invoke_data, ev);
	} else
		small_skel (pobj->servant, ret, args, ctx, ev, imp);

 clean_out:
	if (recv_buffer)
		CORBA_exception_free (ev);

	if (IS_NON_RETAIN (poa))
		switch (poa->p_request_processing) {
		case PortableServer_USE_SERVANT_MANAGER:
			ORBit_POA_ServantManager_unuse_servant (
				poa, pobj, opname, cookie, 
				oid, pobj->servant, ev);
			break;
		case PortableServer_USE_DEFAULT_SERVANT:
			ORBit_POA_deactivate_object (poa, pobj, FALSE, FALSE);
			break;
		default:
			g_assert_not_reached ();
			break;
		}

	g_assert ((ORBit_POAObject)poa->orb->current_invocations->data == pobj);
	poa->orb->current_invocations =
		g_slist_remove (poa->orb->current_invocations, pobj);
	pobj->use_cnt--;

	if (pobj->life_flags & ORBit_LifeF_NeedPostInvoke)
		ORBit_POAObject_post_invoke (pobj);             
}

static ORBit_POAObject
ORBit_POA_object_key_lookup (PortableServer_POA       poa,
			     ORBit_ObjectKey         *objkey,
			     PortableServer_ObjectId *object_id)
{
	object_id->_buffer  = objkey->_buffer + ORBIT_ADAPTOR_PREFIX_LEN;
	object_id->_length  = objkey->_length - ORBIT_ADAPTOR_PREFIX_LEN; 
	object_id->_maximum = object_id->_length;
	object_id->_release = CORBA_FALSE;

	return ORBit_POA_object_id_lookup (poa, object_id);
}

static void
ORBit_POA_handle_request (PortableServer_POA poa,
			  GIOPRecvBuffer    *recv_buffer,
			  ORBit_ObjectKey   *objkey)
{
	PortableServer_ObjectId object_id;
	ORBit_POAObject         pobj;
	CORBA_Identifier        opname;
	CORBA_Environment       env;

	CORBA_exception_init (&env);

	pobj = ORBit_POA_object_key_lookup (poa, objkey, &object_id);

	if (!pobj)
		switch (poa->p_request_processing) {
		case PortableServer_USE_ACTIVE_OBJECT_MAP_ONLY:
			CORBA_exception_set_system (
				&env, ex_CORBA_OBJECT_NOT_EXIST, 
				CORBA_COMPLETED_NO);
			goto send_sys_ex;
			break;

		case PortableServer_USE_DEFAULT_SERVANT: /* drop through */
		case PortableServer_USE_SERVANT_MANAGER:
			pobj = ORBit_POA_create_object (poa, &object_id, &env);
			break;

		default:
			g_assert_not_reached ();
			break;
		}

	if (!pobj)
		CORBA_exception_set_system (
			&env, ex_CORBA_OBJECT_NOT_EXIST, 
			CORBA_COMPLETED_NO);
	else {
		opname = giop_recv_buffer_get_opname (recv_buffer);
		
		ORBit_POAObject_handle_request (pobj, opname, NULL, NULL, 
						NULL, recv_buffer, &env);
	}

 send_sys_ex:
	if (env._major != CORBA_NO_EXCEPTION) {
		tprintf ("p %d, Method '%p' invoked with exception '%s'",
			 getpid (), giop_recv_buffer_get_opname (recv_buffer), env._id);

		return_exception (recv_buffer, NULL, &env);
	}

	CORBA_exception_free (&env);
}

static PortableServer_Servant
ORBit_POA_ServantManager_use_servant (PortableServer_POA                    poa,
				      ORBit_POAObject                       pobj,
				      CORBA_Identifier                      opname,
				      PortableServer_ServantLocator_Cookie *the_cookie,
				      PortableServer_ObjectId              *oid,
				      CORBA_Environment                    *ev)
{
	PortableServer_ServantBase *retval;

	if (IS_RETAIN (poa)) {
		POA_PortableServer_ServantActivator__epv *epv;
		POA_PortableServer_ServantActivator *sm;
		
		sm = (POA_PortableServer_ServantActivator *) poa->servant_manager;
		epv = sm->vepv->PortableServer_ServantActivator_epv;

		retval = epv->incarnate (sm, oid, poa, ev);

		if (retval) {

			/* XXX: two POAs sharing servant and having
			 *      different uniqueness policies ??
			 *  see note 11.3.5.1
			 */
			if (IS_UNIQUE_ID (poa) && retval->_private != NULL) {
				CORBA_exception_set_system (ev, ex_CORBA_OBJ_ADAPTER,
							    CORBA_COMPLETED_NO);
				return NULL;
			}

			pobj->next = retval->_private;
			retval->_private = pobj;

			/* released by ORBit_POA_deactivate_object */
			ORBit_RootObject_duplicate (pobj);
			pobj->servant = retval;
		}
	} else { 
		POA_PortableServer_ServantLocator__epv *epv;
		POA_PortableServer_ServantLocator      *sm;

		sm = (POA_PortableServer_ServantLocator *) poa->servant_manager;
		epv = sm->vepv->PortableServer_ServantLocator_epv;

		retval = epv->preinvoke (sm, oid, poa, opname, the_cookie, ev);

		if (retval) {
			/* FIXME: Is this right?
			 *        Is it the same as above?
			 */
			if (IS_UNIQUE_ID (poa) && retval->_private != NULL) {
				CORBA_exception_set_system (ev, ex_CORBA_OBJ_ADAPTER,
							    CORBA_COMPLETED_NO);
				return NULL;
			}

			pobj->next = retval->_private;
			retval->_private = pobj;

			/* released by ORBit_POA_ServantManager_unuse_servant */
			ORBit_RootObject_duplicate (pobj);
			pobj->servant = retval;
		}
	}

	return retval;
}

static void
ORBit_POA_ServantManager_unuse_servant (PortableServer_POA                    poa,
					ORBit_POAObject                       pobj,
					CORBA_Identifier                      opname,
					PortableServer_ServantLocator_Cookie  cookie,
					PortableServer_ObjectId              *oid,
					PortableServer_Servant                serv,
					CORBA_Environment                    *ev)
{
	POA_PortableServer_ServantLocator      *sm;
	POA_PortableServer_ServantLocator__epv *epv;
	PortableServer_ServantBase             *servant = serv;

	g_assert (IS_NON_RETAIN (poa));

	sm = (POA_PortableServer_ServantLocator *) poa->servant_manager;
	epv = sm->vepv->PortableServer_ServantLocator_epv;

	pobj->servant = NULL;
	
	if ((ORBit_POAObject) servant->_private == pobj)
		servant->_private = pobj->next;
	else {
		ORBit_POAObject l = (ORBit_POAObject) servant->_private;

		for (; l && l->next != pobj; l = l->next);

		g_assert (l != NULL && l->next == pobj);

		l->next = pobj->next;
	}
	pobj->next = NULL;

	ORBit_RootObject_release (pobj);

	epv->postinvoke (sm, oid, poa, opname, cookie, servant, ev);
}

/*
 * This function is invoked from the generated stub code, after
 * invoking the servant method.
 */
void
ORBit_POAObject_post_invoke (ORBit_POAObject pobj)
{
	if (pobj->use_cnt > 0)
		return;

	if (pobj->life_flags & ORBit_LifeF_DeactivateDo)  {
		/* NOTE that the "desired" values of etherealize and cleanup
		 * are stored in pobj->life_flags and they dont need
		 * to be passed in again!
		 */
		ORBit_POA_deactivate_object (
			pobj->poa, pobj, /*ether*/0, /*cleanup*/0);

		/* WATCHOUT: pobj may not exist anymore! */
	}
}

/*
 * C Language Mapping Specific Methods.
 * Section 1.26.2 (C Language Mapping Specification).
 */
CORBA_char *
PortableServer_ObjectId_to_string (PortableServer_ObjectId *id, 
				   CORBA_Environment       *ev)
{
	CORBA_char *str;

	poa_sys_exception_val_if_fail (id != NULL, ex_CORBA_BAD_PARAM, NULL);
	poa_sys_exception_val_if_fail (memchr (id->_buffer, '\0', id->_length),
				       ex_CORBA_BAD_PARAM, NULL);

	str = CORBA_string_alloc (id->_length);
	memcpy (str, id->_buffer, id->_length);
	str [id->_length] = '\0';

	return str;
}

CORBA_wchar *
PortableServer_ObjectId_to_wstring (PortableServer_ObjectId *id,
				    CORBA_Environment       *ev)
{
	CORBA_wchar *retval;
	int          i;

	poa_sys_exception_val_if_fail (id != NULL, ex_CORBA_BAD_PARAM, NULL);
	poa_sys_exception_val_if_fail (memchr (id->_buffer, '\0', id->_length),
				       ex_CORBA_BAD_PARAM, NULL);
  
	retval = CORBA_wstring_alloc (id->_length);
	for (i = 0; i < id->_length; i++)
		retval [i] = id->_buffer [i];
	retval [id->_length] = '\0';

	return retval;
}

PortableServer_ObjectId *
PortableServer_string_to_ObjectId (CORBA_char        *str,
				   CORBA_Environment *ev)
{
	PortableServer_ObjectId tmp;

	poa_sys_exception_val_if_fail (str != NULL, ex_CORBA_BAD_PARAM, NULL);

	tmp._length  = strlen (str);
	tmp._buffer  = str;
  
	return (PortableServer_ObjectId *) ORBit_sequence_CORBA_octet_dup (&tmp);
}

PortableServer_ObjectId *
PortableServer_wstring_to_ObjectId (CORBA_wchar       *str,
				    CORBA_Environment *ev)
{
	PortableServer_ObjectId tmp;
	int                     i;

	poa_sys_exception_val_if_fail (str != NULL, ex_CORBA_BAD_PARAM, NULL);

	for (i = 0; str[i]; i++);

	tmp._length = i*2;
	tmp._buffer = g_alloca (tmp._length);

	for (i = 0; str[i]; i++)
		tmp._buffer[i] = str[i];

	return (PortableServer_ObjectId *) ORBit_sequence_CORBA_octet_dup (&tmp);
}

/*
 * Current Operations.
 * Section 11.3.9
 */

PortableServer_POA
PortableServer_Current_get_POA (PortableServer_Current  obj,
				CORBA_Environment      *ev)
{
	ORBit_POAObject pobj;

	poa_sys_exception_val_if_fail (obj != NULL, ex_CORBA_INV_OBJREF, NULL);

	pobj = ORBit_POACurrent_get_object (obj, ev);

	return ORBit_RootObject_duplicate (pobj->poa);
}

PortableServer_ObjectId *
PortableServer_Current_get_object_id (PortableServer_Current  obj,
				      CORBA_Environment      *ev)
{
	ORBit_POAObject pobj;

	poa_sys_exception_val_if_fail (obj != NULL, ex_CORBA_INV_OBJREF, NULL);

	pobj = ORBit_POACurrent_get_object (obj, ev);

	if (!pobj) 
		return NULL;

	return (PortableServer_ObjectId *)ORBit_sequence_CORBA_octet_dup (pobj->object_id);
}

/*
 * PortableServer::POA interface
 * Section 11.3.8
 */

PortableServer_POA
PortableServer_POA_create_POA (PortableServer_POA               poa,
			       const CORBA_char                *adaptor_name,
			       const PortableServer_POAManager  a_POAManager,
			       const CORBA_PolicyList          *policies,
			       CORBA_Environment               *ev)
{
	PortableServer_POA retval;

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);
	poa_sys_exception_val_if_fail (adaptor_name != NULL, ex_CORBA_BAD_PARAM, NULL);
	poa_sys_exception_val_if_fail (policies != NULL, ex_CORBA_BAD_PARAM, NULL);

	if (g_hash_table_lookup (poa->child_poas, adaptor_name)) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_PortableServer_POA_AdapterAlreadyExists,
				     NULL);
		return CORBA_OBJECT_NIL;
	}

	retval = ORBit_POA_new (poa->orb, adaptor_name, a_POAManager, policies, ev);

	ORBit_POA_add_child (poa, retval);

	return retval;
}

PortableServer_POA
PortableServer_POA_find_POA (PortableServer_POA   poa,
			     const CORBA_char    *adaptor_name,
			     const CORBA_boolean  activate_it,
			     CORBA_Environment   *ev)
{
	PortableServer_POA child_poa = NULL;

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);
	poa_sys_exception_val_if_fail (adaptor_name != NULL, ex_CORBA_BAD_PARAM, NULL);

	if (poa->child_poas)
		child_poa = g_hash_table_lookup (poa->child_poas, adaptor_name);

	if (activate_it)
		g_warning ("Don't yet know how to activate POA named \"%s\"",
			   adaptor_name);

	if (!child_poa)
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_PortableServer_POA_AdapterNonExistent,
				     NULL);	

	return ORBit_RootObject_duplicate (child_poa);
}

void
PortableServer_POA_destroy (PortableServer_POA   poa,
			    const CORBA_boolean  etherealize_objects,
			    const CORBA_boolean  wait_for_completion,
			    CORBA_Environment   *ev)
{
	gboolean done;

	poa_sys_exception_if_fail (poa != NULL, ex_CORBA_INV_OBJREF);

	if (poa->life_flags & ORBit_LifeF_Destroyed)
		return;

	if (wait_for_completion && ORBit_POA_is_inuse (poa, CORBA_TRUE, ev)) {
		CORBA_exception_set_system (ev, ex_CORBA_BAD_INV_ORDER,
					    CORBA_COMPLETED_NO);
		return;
	}

	done = ORBit_POA_destroy (poa, etherealize_objects, ev);

	g_assert (done || !wait_for_completion);
}

CORBA_string
PortableServer_POA__get_the_name (PortableServer_POA  poa,
				  CORBA_Environment  *ev)
{
	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);

	return CORBA_string_dup (poa->name);
}

PortableServer_POA
PortableServer_POA__get_the_parent (PortableServer_POA  poa,
				    CORBA_Environment  *ev)
{
	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);

	return ORBit_RootObject_duplicate (poa->parent_poa);
}

static void
ORBit_POAList_add_child (char                   *name,
			 PortableServer_POA      poa,
			 PortableServer_POAList *list)
{
	list->_buffer [list->_length++] = ORBit_RootObject_duplicate (poa);
}

PortableServer_POAList *
PortableServer_POA__get_the_children (PortableServer_POA  poa,
				      CORBA_Environment  *ev)
{
	PortableServer_POAList *retval;
	int                     length;

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);

	length = g_hash_table_size (poa->child_poas);

	retval           = PortableServer_POAList__alloc ();
	retval->_length  = 0;
	retval->_maximum = length;
	retval->_buffer  = (CORBA_Object *) PortableServer_POAList_allocbuf (length);
	retval->_release = CORBA_TRUE;

	g_hash_table_foreach (poa->child_poas, (GHFunc) ORBit_POAList_add_child, retval);

	g_assert (retval->_length == length);

	return retval;
}

PortableServer_POAManager
PortableServer_POA__get_the_POAManager (PortableServer_POA  poa,
					CORBA_Environment  *ev)
{
	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);

	return ORBit_RootObject_duplicate (poa->poa_manager);
}

PortableServer_AdapterActivator
PortableServer_POA__get_the_activator (PortableServer_POA  poa,
				       CORBA_Environment  *ev)
{
	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);

	return ORBit_RootObject_duplicate (poa->the_activator);
}

void
PortableServer_POA__set_the_activator (PortableServer_POA                    poa,
				       const PortableServer_AdapterActivator activator,
				       CORBA_Environment * ev)
{
	poa_sys_exception_if_fail (poa != NULL, ex_CORBA_INV_OBJREF);
	poa_sys_exception_if_fail (activator != NULL, ex_CORBA_BAD_PARAM);

	if (poa->the_activator)
		ORBit_RootObject_release (poa->the_activator);

	poa->the_activator = (PortableServer_AdapterActivator)
					ORBit_RootObject_duplicate (activator);
}

PortableServer_ServantManager
PortableServer_POA_get_servant_manager (PortableServer_POA  poa,
					CORBA_Environment  *ev)
{
	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);

	return ORBit_RootObject_duplicate (poa->servant_manager);
}

void
PortableServer_POA_set_servant_manager (PortableServer_POA                   poa,
					const PortableServer_ServantManager  manager,
					CORBA_Environment                   *ev)
{
	poa_sys_exception_if_fail (poa != NULL, ex_CORBA_INV_OBJREF);
	poa_sys_exception_if_fail (manager != NULL, ex_CORBA_BAD_PARAM);
	poa_sys_exception_if_fail (poa->servant_manager == NULL, ex_CORBA_BAD_INV_ORDER);

	poa->servant_manager = (PortableServer_ServantManager)
					ORBit_RootObject_duplicate (manager);
}

PortableServer_Servant
PortableServer_POA_get_servant (PortableServer_POA  poa,
				CORBA_Environment  *ev)
{
	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);

	return poa->default_servant;
}

void
PortableServer_POA_set_servant (PortableServer_POA            poa,
				const PortableServer_Servant  servant,
				CORBA_Environment            *ev)
{
	poa_sys_exception_if_fail (poa != NULL, ex_CORBA_INV_OBJREF);
	poa_sys_exception_if_fail (servant != NULL, ex_CORBA_BAD_PARAM);

	poa->default_servant = servant;
}

PortableServer_ObjectId *
PortableServer_POA_activate_object (PortableServer_POA            poa,
				    const PortableServer_Servant  p_servant,
				    CORBA_Environment            *ev)
{
	PortableServer_ServantBase *servant = p_servant;
	ORBit_POAObject             newobj;

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);
	poa_sys_exception_val_if_fail (servant != NULL, ex_CORBA_BAD_PARAM, NULL);

	poa_exception_val_if_fail (IS_RETAIN (poa), ex_PortableServer_POA_WrongPolicy, NULL);
	poa_exception_val_if_fail (IS_SYSTEM_ID (poa), ex_PortableServer_POA_WrongPolicy, NULL);
	poa_exception_val_if_fail (IS_MULTIPLE_ID (poa) || (IS_UNIQUE_ID (poa) && servant->_private == NULL),
				   ex_PortableServer_POA_ServantAlreadyActive, NULL);

	newobj = ORBit_POA_create_object (poa, NULL, ev);
	ORBit_POA_activate_object (poa, newobj, servant, ev);

	return (PortableServer_ObjectId *)
			ORBit_sequence_CORBA_octet_dup (newobj->object_id);
}

void
PortableServer_POA_activate_object_with_id (PortableServer_POA             poa,
					    const PortableServer_ObjectId *objid,
					    const PortableServer_Servant   p_servant,
					    CORBA_Environment             *ev)
{
	PortableServer_ServantBase *servant = p_servant;
	ORBit_POAObject             newobj;

	poa_sys_exception_if_fail (poa != NULL, ex_CORBA_INV_OBJREF);
	poa_sys_exception_if_fail (objid != NULL, ex_CORBA_BAD_PARAM);
	poa_sys_exception_if_fail (servant != NULL, ex_CORBA_BAD_PARAM);

	poa_exception_if_fail (IS_RETAIN (poa), ex_PortableServer_POA_WrongPolicy);

	newobj = ORBit_POA_object_id_lookup (poa, objid);

	if (newobj && newobj->servant) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_PortableServer_POA_ObjectAlreadyActive, 
				     NULL);
		return;
	}

	if (IS_UNIQUE_ID (poa) && servant->_private != NULL) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_PortableServer_POA_ServantAlreadyActive,
				     NULL);
		return;
	}

	if (!newobj)
		newobj = ORBit_POA_create_object(poa, objid, ev);

	ORBit_POA_activate_object (poa, newobj, servant, ev);
}

void
PortableServer_POA_deactivate_object (PortableServer_POA             poa,
				      const PortableServer_ObjectId *oid,
				      CORBA_Environment             *ev)
{
	ORBit_POAObject pobj;

	poa_sys_exception_if_fail (poa != NULL, ex_CORBA_INV_OBJREF);
	poa_sys_exception_if_fail (oid != NULL, ex_CORBA_BAD_PARAM);

	poa_exception_if_fail (IS_RETAIN (poa), ex_PortableServer_POA_WrongPolicy);

	pobj = ORBit_POA_object_id_lookup (poa, oid);

	if (pobj && pobj->servant)
		ORBit_POA_deactivate_object (poa, pobj, CORBA_TRUE, CORBA_FALSE);
}

CORBA_Object
PortableServer_POA_create_reference (PortableServer_POA  poa,
				     const CORBA_char   *intf,
				     CORBA_Environment  *ev)
{
	ORBit_POAObject pobj;

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);

	poa_exception_val_if_fail (IS_SYSTEM_ID (poa), ex_PortableServer_POA_WrongPolicy, NULL);

	pobj = ORBit_POA_create_object (poa, NULL, ev);

	return ORBit_POA_obj_to_ref (poa, pobj, intf, ev);
}

CORBA_Object
PortableServer_POA_create_reference_with_id (PortableServer_POA             poa,
					     const PortableServer_ObjectId *oid,
					     const CORBA_char              *intf,
					     CORBA_Environment             *ev)
{
	ORBit_POAObject	pobj;

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);
	poa_sys_exception_val_if_fail (oid != NULL, ex_CORBA_BAD_PARAM, NULL);

	pobj = ORBit_POA_object_id_lookup (poa, oid);
	if (!pobj)
		pobj = ORBit_POA_create_object (poa, oid, ev);

	return ORBit_POA_obj_to_ref (poa, pobj, intf, ev);
}

PortableServer_ObjectId *
PortableServer_POA_servant_to_id (PortableServer_POA            poa,
				  const PortableServer_Servant  p_servant,
				  CORBA_Environment            *ev)
{
	PortableServer_ServantBase *servant = p_servant;
	ORBit_POAObject             pobj = servant->_private;
	gboolean                    defserv = IS_USE_DEFAULT_SERVANT (poa);
	gboolean                    retain = IS_RETAIN (poa);
	gboolean                    implicit = IS_IMPLICIT_ACTIVATION (poa);
	gboolean                    unique = IS_UNIQUE_ID (poa);

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);
	poa_sys_exception_val_if_fail (servant != NULL, ex_CORBA_BAD_PARAM, NULL);

	poa_exception_val_if_fail (defserv || (retain && (unique || implicit)),
				   ex_PortableServer_POA_WrongPolicy, NULL);

	if (retain && unique && pobj && pobj->servant == servant)
		return ORBit_sequence_CORBA_octet_dup (pobj->object_id);

	else if (retain && implicit && (!unique || !pobj)) {
		pobj = ORBit_POA_create_object (poa, NULL, ev);
		ORBit_POA_activate_object (poa, pobj, servant, ev);

		return ORBit_sequence_CORBA_octet_dup (pobj->object_id);
	}
	else {
		/*
		 * FIXME:
		 * This handles case 3 of the spec; but is broader:
		 * it matches invokations on any type of servant, not
		 * just the default servant.
		 * The stricter form could be implemented, 
		 * but it would only add more code...
		 */
		GSList *l = poa->orb->current_invocations;

		for ( ; l; l = l->next) {
			ORBit_POAObject pobj = l->data;

			if (pobj->servant == servant)
				return ORBit_sequence_CORBA_octet_dup (pobj->object_id);
		}
	}

	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_PortableServer_POA_ServantNotActive,
			     NULL);

	return NULL;
}

CORBA_Object
PortableServer_POA_servant_to_reference (PortableServer_POA            poa,
					 const PortableServer_Servant  p_servant,
					 CORBA_Environment            *ev)
{
	PortableServer_ServantBase *servant = p_servant;
	ORBit_POAObject             pobj = servant->_private;
	gboolean                    retain = IS_RETAIN (poa);
	gboolean                    implicit = IS_IMPLICIT_ACTIVATION (poa);
	gboolean                    unique = IS_UNIQUE_ID (poa);

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, CORBA_OBJECT_NIL);
	poa_sys_exception_val_if_fail (servant != NULL, ex_CORBA_BAD_PARAM, CORBA_OBJECT_NIL);

	poa_exception_val_if_fail (retain && (unique || implicit),
				   ex_PortableServer_POA_WrongPolicy,
				   CORBA_OBJECT_NIL);

	if (retain && unique && pobj)
		if (pobj->base.objref)
			return ORBit_RootObject_duplicate (
				pobj->base.objref);
		else
			return ORBit_POA_obj_to_ref (poa, pobj, NULL, ev);

	else if (retain && implicit && (!unique || !pobj)) {
		pobj = ORBit_POA_create_object (poa, NULL, ev);
		ORBit_POA_activate_object (poa, pobj, servant, ev);

		return ORBit_POA_obj_to_ref (poa, pobj, NULL, ev);
	}
	else {
		/*
		 * FIXME:
		 * This case deals with "invoked in the context of
		 * executing a request." Note that there are no policy
		 * restrictions for this case. We must do a forward search
		 * looking for matching {servant}. If unique, we could 
		 * go backward from servant to pobj to use_cnt, but we
		 * dont do this since forward search is more general 
		 */
		GSList *l = poa->orb->current_invocations;

		for ( ; l; l = l->next) {
			ORBit_POAObject pobj = l->data;

			if (pobj->servant == servant)
				return ORBit_POA_obj_to_ref (poa, pobj, NULL, ev);
		}
	}

	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_PortableServer_POA_ServantNotActive,
			     NULL);

	return NULL;
}

PortableServer_Servant
PortableServer_POA_reference_to_servant (PortableServer_POA  poa,
					 const CORBA_Object  reference,
					 CORBA_Environment  *ev)
{

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);
	poa_sys_exception_val_if_fail (reference != NULL, ex_CORBA_BAD_PARAM, NULL);

	poa_exception_val_if_fail (IS_USE_DEFAULT_SERVANT (poa) || IS_RETAIN (poa),
				   ex_PortableServer_POA_WrongPolicy, NULL);

	if (IS_RETAIN (poa)) {
		ORBit_POAObject pobj;

		poa_exception_val_if_fail (reference->adaptor_obj != NULL,
					   ex_PortableServer_POA_WrongAdapter,
					   NULL);

		pobj = (ORBit_POAObject) reference->adaptor_obj;

		if (pobj->servant)
			return pobj->servant;

	} else if (poa->default_servant)
		return poa->default_servant;

	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_PortableServer_POA_ObjectNotActive,
			     NULL);

	return NULL;
}

PortableServer_ObjectId *
PortableServer_POA_reference_to_id (PortableServer_POA  poa,
				    const CORBA_Object  reference,
				    CORBA_Environment  *ev)
{
	ORBit_POAObject pobj;

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);
	poa_sys_exception_val_if_fail (reference != NULL, ex_CORBA_BAD_PARAM, NULL);

	poa_exception_val_if_fail (reference->adaptor_obj != NULL,
				   ex_PortableServer_POA_WrongAdapter, 
				   NULL);

	pobj = (ORBit_POAObject) reference->adaptor_obj;

	return (PortableServer_ObjectId *)
				ORBit_sequence_CORBA_octet_dup (pobj->object_id);
}

PortableServer_Servant
PortableServer_POA_id_to_servant (PortableServer_POA             poa,
				  const PortableServer_ObjectId *object_id,
				  CORBA_Environment             *ev)
{
	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);
	poa_sys_exception_val_if_fail (object_id != NULL, ex_CORBA_BAD_PARAM, NULL);

	poa_exception_val_if_fail (IS_USE_DEFAULT_SERVANT (poa) || IS_RETAIN (poa),
				   ex_PortableServer_POA_WrongPolicy, NULL);

	if (IS_RETAIN (poa)) {
		ORBit_POAObject pobj = ORBit_POA_object_id_lookup (poa, object_id);

		if (pobj && pobj->servant)
			return pobj->servant;

	} else if (poa->default_servant)
		return poa->default_servant;

	CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
			     ex_PortableServer_POA_ObjectNotActive,
			     NULL);

	return NULL;
}

CORBA_Object
PortableServer_POA_id_to_reference (PortableServer_POA             poa,
				    const PortableServer_ObjectId *object_id,
				    CORBA_Environment             *ev)
{
	ORBit_POAObject pobj;

	poa_sys_exception_val_if_fail (poa != NULL, ex_CORBA_INV_OBJREF, NULL);
	poa_sys_exception_val_if_fail (object_id != NULL, ex_CORBA_BAD_PARAM, NULL);

	poa_exception_val_if_fail (IS_RETAIN (poa), ex_PortableServer_POA_WrongPolicy, NULL);

	pobj = ORBit_POA_object_id_lookup (poa, object_id);
	if (!pobj || !pobj->servant) {
		CORBA_exception_set (ev, CORBA_USER_EXCEPTION,
				     ex_PortableServer_POA_ObjectNotActive, NULL);
		return CORBA_OBJECT_NIL;
	}

	if (pobj->base.objref)
		return ORBit_RootObject_duplicate (pobj->base.objref);

	return ORBit_POA_obj_to_ref (poa, pobj, NULL, ev);
}
