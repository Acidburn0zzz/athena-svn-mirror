/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * bonobo-object.c: Bonobo Unknown interface base implementation
 *
 * Authors:
 *   Miguel de Icaza (miguel@kernel.org)
 *   Michael Meeks (michael@helixcode.com)
 *
 * Copyright 1999,2000 Helix Code, Inc.
 */
#include <config.h>
#include <stdio.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <gtk/gtktypeutils.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-object.h>
#include "Bonobo.h"
#include "bonobo-running-context.h"
#include "bonobo-object-directory.h"

/* Assumptions made: sizeof(POA_interfacename) does not change between interfaces */

POA_Bonobo_Unknown__vepv bonobo_object_vepv;

#ifdef BONOBO_OBJECT_DEBUG
#	define BONOBO_REF_HOOKS
#endif

/*
 * NB. for a quicker debugging experience simply
 * #define BONOBO_REF_HOOKS
 */
#if 0
#	define BONOBO_REF_HOOKS
#endif

#ifdef BONOBO_REF_HOOKS
typedef struct {
	const char *fn;
	gboolean    ref;
	int         line;
} BonoboDebugRefData;
#endif

typedef struct {
	int   ref_count;
	GList *objs;
#ifdef BONOBO_REF_HOOKS
	GList *refs;
	int    destroyed;
#endif
} BonoboAggregateObject;

struct _BonoboObjectPrivate {
	BonoboAggregateObject *ao;
	int destroy_id;
};

enum {
	QUERY_INTERFACE,
	SYSTEM_EXCEPTION,
	OBJECT_GONE,
	LAST_SIGNAL
};

static guint bonobo_object_signals [LAST_SIGNAL];
static GtkObjectClass *bonobo_object_parent_class;

#ifdef BONOBO_REF_HOOKS

static int         ref_indent = 0;
static GHashTable *living_ao_ht = NULL;

#endif /* BONOBO_REF_HOOKS */

/**
 * bonobo_object_from_servant:
 * @servant: Your servant.
 *
 * CORBA method implementations receive a parameter of type
 * #PortableServer_Servant which is a pointer to the servant that was
 * used to create this specific CORBA object instance.
 *
 * This routine allows the user to get the #BonoboObject (ie, Gtk
 * object wrapper) from the servant.  This #BonoboObject is the Gtk
 * object wrapper associated with the CORBA object instance whose
 * method is being invoked.
 *
 * Returns: the #BonoboObject wrapper associated with @servant.
 */
BonoboObject *
bonobo_object_from_servant (PortableServer_Servant servant)
{
	g_return_val_if_fail (servant != NULL, NULL);

	return BONOBO_OBJECT(((BonoboObjectServant *)servant)->bonobo_object);
}

/**
 * bonobo_object_get_servant:
 * @object: A #BonoboObject which is associated with a servant.
 *
 * Returns: The servant associated with @object, or %NULL if
 * no servant is bound to @object.
 */
PortableServer_Servant
bonobo_object_get_servant (BonoboObject *object)
{
	g_return_val_if_fail (BONOBO_IS_OBJECT (object), NULL);

	return object->servant;
}

/**
 * bonobo_object_bind_to_servant:
 * @object: the BonoboObject to bind to the servant.
 * @servant: A PortableServer_Servant to bind to the BonoboObject.
 *
 * This routine binds @object to @servant.  It establishes a one to
 * one mapping between the @object and the @servant.  Utility routines
 * are provided to go back and forth.  See bonobo_object_from_servant()
 * and bonobo_object_get_servant().
 *
 * This routine is used internally by bonobo_object_activate_servant().
 */
void
bonobo_object_bind_to_servant (BonoboObject *object, void *servant)
{
	g_return_if_fail (BONOBO_IS_OBJECT (object));
	g_return_if_fail (servant != NULL);

	object->servant = servant;
	((BonoboObjectServant *)servant)->bonobo_object = object;
}

/* Do not use this function, it is not what you want; see unref */
static void
bonobo_object_destroy (BonoboAggregateObject *ao)
{
	GList *l;

	g_return_if_fail (ao->ref_count > 0);

	for (l = ao->objs; l; l = l->next) {
		GtkObject *o = l->data;

		gtk_signal_disconnect (o, BONOBO_OBJECT (o)->priv->destroy_id);
		if (o->ref_count >= 1)
			gtk_object_destroy (GTK_OBJECT (o));
		else
			g_warning ("Serious ref-counting error [%p]", o);
	}
#ifdef BONOBO_REF_HOOKS
	ao->destroyed = TRUE;
#endif
}

static void
bonobo_object_finalize (BonoboAggregateObject *ao)
{
	GList *l;

	g_return_if_fail (ao->ref_count == 0);

	for (l = ao->objs; l; l = l->next) {
		GtkObject *o = GTK_OBJECT (l->data);

		if (!o)
			g_error ("Serious bonobo object corruption");
		else {
			g_assert (BONOBO_OBJECT (o)->priv->ao != NULL);
#ifdef BONOBO_REF_HOOKS
			g_assert (BONOBO_OBJECT (o)->priv->ao->destroyed);
#endif

			/*
			 * Disconnect the GTK+ object from the aggregate object
			 * and unref it so that it is possibly finalized ---
			 * other parts of GTK+ may still have references to it.
			 *
			 * The GTK+ object was already destroy()ed in
			 * bonobo_object_destroy().
			 */

			BONOBO_OBJECT (o)->priv->ao = NULL;
			gtk_object_unref (o);
		}
	}

	g_list_free (ao->objs);
	ao->objs = NULL;

#ifdef BONOBO_REF_HOOKS
	for (l = ao->refs; l; l = l->next)
		g_free (l->data);
	g_list_free (ao->refs);
#endif

	g_free (ao);
}

#ifndef bonobo_object_ref
/**
 * bonobo_object_ref:
 * @object: A BonoboObject you want to ref-count
 *
 * increments the reference count for the aggregate BonoboObject.
 */
void
bonobo_object_ref (BonoboObject *object)
{
	g_return_if_fail (BONOBO_IS_OBJECT (object));
	g_return_if_fail (object->priv->ao->ref_count > 0);

#ifdef BONOBO_REF_HOOKS
	bonobo_object_trace_refs (object, "local", 0, TRUE);
#else
	object->priv->ao->ref_count++;
#endif
}
#endif /* bonobo_object_ref */


#ifndef bonobo_object_unref
/**
 * bonobo_object_unref:
 * @object: A BonoboObject you want to unref.
 *
 * decrements the reference count for the aggregate BonoboObject.
 */
void
bonobo_object_unref (BonoboObject *object)
{
#ifdef BONOBO_REF_HOOKS
	bonobo_object_trace_refs (object, "local", 0, FALSE);
#else
	BonoboAggregateObject *ao;

	g_return_if_fail (BONOBO_IS_OBJECT (object));

	ao = object->priv->ao;
	g_return_if_fail (ao != NULL);
	g_return_if_fail (ao->ref_count > 0);

	if (ao->ref_count == 1)
		bonobo_object_destroy (ao);

	ao->ref_count--;

	if (ao->ref_count == 0)
		bonobo_object_finalize (ao);
#endif /* BONOBO_REF_HOOKS */
}
#endif /* bonobo_object_unref */

void
bonobo_object_trace_refs (BonoboObject *object,
			  const char   *fn,
			  int           line,
			  gboolean      ref)
{
#ifdef BONOBO_REF_HOOKS
	char *indent;
	BonoboAggregateObject *ao;
	BonoboDebugRefData *descr;
	
	g_return_if_fail (BONOBO_IS_OBJECT (object));
	ao = object->priv->ao;
	g_return_if_fail (ao != NULL);

	descr  = g_new (BonoboDebugRefData, 1);
	ao->refs = g_list_prepend (ao->refs, descr);
	descr->fn = fn;
	descr->ref = ref;
	descr->line = line;

	if (ref) {
		g_return_if_fail (ao->ref_count > 0);
		
		object->priv->ao->ref_count++;
		
		indent = g_strnfill (++ref_indent, ' ');
		g_printerr ("%s[%d]:Ref %s:[%p] to %d at %s:%d\n", indent, getpid(),
			    gtk_type_name (GTK_OBJECT (object)->klass->type),
			    object, ao->ref_count, fn, line);

		g_free (indent);
	} else { /* unref */
		indent = g_strnfill (ref_indent--, ' ');
		g_printerr ("%s[%d]:UnRef %s:[%p] from %d at %s:%d\n", indent, getpid(),
			    gtk_type_name (GTK_OBJECT (object)->klass->type),
			    ao, ao->ref_count, fn, line);
		g_free (indent);

		g_return_if_fail (ao->ref_count > 0);
	
		if (ao->ref_count == 1) {
			bonobo_object_destroy (ao);

			g_return_if_fail (ao->ref_count > 0);
		}

		/*
		 * If this blows it is likely some loony used
		 * gtk_object_unref somewhere instead of
		 * bonobo_object_unref, send them my regards.
		 */
		g_assert (object->priv->ao == ao);
		
		ao->ref_count--;
	
		if (ao->ref_count == 0) {

			g_assert (g_hash_table_lookup (living_ao_ht, ao) == ao);
			g_hash_table_remove (living_ao_ht, ao);
			
			bonobo_object_finalize (ao);
		} else if (ao->ref_count < 0) {
			indent = g_strnfill (ref_indent + 1, ' ');
			g_printerr ("%sUnusual: [%p] already finalized\n",
				    indent, ao);
			g_free (indent);
		}
	}
#else
	if (ref)
		bonobo_object_ref (object);
	else
		bonobo_object_unref (object);
#endif
}

static void
impl_Bonobo_Unknown_ref (PortableServer_Servant servant, CORBA_Environment *ev)
{
	BonoboObject *object;

	object = bonobo_object_from_servant (servant);

#if defined(BONOBO_REF_HOOKS) && !defined(bonobo_object_ref)
	bonobo_object_trace_refs (object, "remote", 0, TRUE);
#else
	bonobo_object_ref (object);
#endif
}

/**
 * bonobo_object_dup_ref:
 * @object: a Bonobo_Unknown corba object
 * @ev: Corba_Environment
 * 
 *   This function returns a duplicated CORBA Object reference,
 * it also bumps the ref count on the object. This is ideal to
 * use in any method returning a Bonobo_Object in a CORBA impl.
 * if object is CORBA_OBJECT_NIL it is returned unaffected.
 * 
 * Return value: duplicated & ref'd corba object reference.
 **/
Bonobo_Unknown
bonobo_object_dup_ref (Bonobo_Unknown     object,
		       CORBA_Environment *ev)
{
	CORBA_Environment tmpev, *rev;
	Bonobo_Unknown    ans;

	if (object == CORBA_OBJECT_NIL)
		return CORBA_OBJECT_NIL;

	if (ev)
		rev = ev;
	else {
		rev = &tmpev;
		CORBA_exception_init (rev);
	}

	Bonobo_Unknown_ref (object, rev);
	ans = CORBA_Object_duplicate (object, rev);

	if (!ev)
		CORBA_exception_free (&tmpev);

	return ans;
}

/**
 * bonobo_object_release_unref:
 * @object: a Bonobo_Unknown corba object
 * @ev: Corba_Environment, optional
 * 
 *   This function returns releases a CORBA Object reference,
 * it also decrements the ref count on the bonobo object.
 * This is the converse of bonobo_object_dup_ref we
 * tolerate object == CORBA_OBJECT_NIL silently
 **/
void
bonobo_object_release_unref (Bonobo_Unknown     object,
			     CORBA_Environment *ev)
{
	CORBA_Environment tmpev, *rev;

	if (object == CORBA_OBJECT_NIL)
		return;

	if (ev)
		rev = ev;
	else {
		rev = &tmpev;
		CORBA_exception_init (rev);
	}

	Bonobo_Unknown_unref (object, rev);
	CORBA_Object_release (object, rev);

	if (!ev)
		CORBA_exception_free (&tmpev);
}

static void
impl_Bonobo_Unknown_unref (PortableServer_Servant servant, CORBA_Environment *ev)
{
	BonoboObject *object;

	object = bonobo_object_from_servant (servant);

#if defined(BONOBO_REF_HOOKS) && !defined(bonobo_object_unref)
	bonobo_object_trace_refs (object, "remote", 0, FALSE);
#else
	bonobo_object_unref (object);
#endif
}

static BonoboObject *
bonobo_object_get_local_interface_from_objref (BonoboObject *object,
					       CORBA_Object  interface)
{
	CORBA_Environment  ev;
	GList             *l;

	if (interface == CORBA_OBJECT_NIL)
		return NULL;

	CORBA_exception_init (&ev);

	for (l = object->priv->ao->objs; l; l = l->next) {
		BonoboObject *tryme = l->data;

		if (CORBA_Object_is_equivalent (interface, tryme->corba_objref, &ev)) {
			CORBA_exception_free (&ev);
			return tryme;
		}

		if (BONOBO_EX (&ev)) {
			CORBA_exception_free (&ev);
			return NULL;
		}

	}

	CORBA_exception_free (&ev);

	return NULL;
}

/**
 * bonobo_object_query_local_interface:
 * @object: A #BonoboObject which is the aggregate of multiple objects.
 * @repo_id: The id of the interface being queried.
 *
 * Returns: A #BonoboObject for the requested interface.
 */
BonoboObject *
bonobo_object_query_local_interface (BonoboObject *object,
				     const char   *repo_id)
{
	CORBA_Environment  ev;
	BonoboObject      *retval;
	CORBA_Object       corba_retval;
	GtkType            type;
	GList             *l;

	g_return_val_if_fail (BONOBO_IS_OBJECT (object), NULL);

	retval       = NULL;
	corba_retval = CORBA_OBJECT_NIL;

	gtk_signal_emit (
		GTK_OBJECT (object), bonobo_object_signals [QUERY_INTERFACE],
		repo_id, &corba_retval);

	CORBA_exception_init (&ev);

	if (! CORBA_Object_is_nil (corba_retval, &ev)) {
		BonoboObject *local_interface;

		local_interface = bonobo_object_get_local_interface_from_objref (
			object, corba_retval);

		if (local_interface != NULL)
			bonobo_object_ref (object);

		return local_interface;
	}

	type = gtk_type_from_name (repo_id);

	/* Try looking at the gtk types */
	for (l = object->priv->ao->objs; l; l = l->next){
		BonoboObject *tryme = l->data;

		if ((type && gtk_type_is_a (GTK_OBJECT (tryme)->klass->type, type)) ||
#ifdef ORBIT_IMPLEMENTS_IS_A
		    CORBA_Object_is_a (tryme->corba_objref, (char *) repo_id, &ev)
#else
		    !strcmp (tryme->corba_objref->object_id, repo_id)
#endif
			) {
			retval = tryme;
			break;
		}
	}

	if (retval != NULL)
		bonobo_object_ref (object);

	CORBA_exception_free (&ev);

	return retval;
}

static CORBA_Object
impl_Bonobo_Unknown_queryInterface (PortableServer_Servant  servant,
				     const CORBA_char       *repoid,
				     CORBA_Environment      *ev)
{
	BonoboObject *object = bonobo_object_from_servant (servant);
	BonoboObject *local_interface;

	local_interface = bonobo_object_query_local_interface (
		object, repoid);

	if (local_interface == NULL)
		return CORBA_OBJECT_NIL;

	return CORBA_Object_duplicate (local_interface->corba_objref, ev);
}

/**
 * bonobo_object_get_epv:
 *
 * Returns: the Bonobo Object epv.
 */
POA_Bonobo_Unknown__epv *
bonobo_object_get_epv (void)
{
	POA_Bonobo_Unknown__epv *epv;

	epv = g_new0 (POA_Bonobo_Unknown__epv, 1);

	epv->ref            = impl_Bonobo_Unknown_ref;
	epv->unref          = impl_Bonobo_Unknown_unref;
	epv->queryInterface = impl_Bonobo_Unknown_queryInterface;

	return epv;
}

static void
init_object_corba_class (void)
{
	bonobo_object_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
}

static void
bonobo_object_finalize_real (GtkObject *object)
{
	BonoboObject *bonobo_object = BONOBO_OBJECT (object);
	void *servant = bonobo_object->servant;
	CORBA_Environment ev;

	g_assert (bonobo_object->priv->ao == NULL);

	CORBA_exception_init (&ev);

	if (bonobo_object->corba_objref != CORBA_OBJECT_NIL) {
		bonobo_running_context_remove_object (bonobo_object->corba_objref);
		CORBA_Object_release (bonobo_object->corba_objref, &ev);
		bonobo_object->corba_objref = CORBA_OBJECT_NIL;
	}

	if (servant) {
		PortableServer_ObjectId *oid;

		oid = PortableServer_POA_servant_to_id (bonobo_poa(), servant, &ev);
		PortableServer_POA_deactivate_object (bonobo_poa (), oid, &ev);

		POA_Bonobo_Unknown__fini (servant, &ev);
		CORBA_free (oid);
	}
	CORBA_exception_free (&ev);

	g_free (bonobo_object->priv);

	bonobo_object_parent_class->finalize (object);
}

static void
bonobo_object_class_init (BonoboObjectClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	bonobo_object_parent_class = gtk_type_class (gtk_object_get_type ());

	bonobo_object_signals [QUERY_INTERFACE] =
		gtk_signal_new ("query_interface",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET(BonoboObjectClass,query_interface),
				gtk_marshal_NONE__POINTER_POINTER,
				GTK_TYPE_NONE, 2, GTK_TYPE_POINTER, GTK_TYPE_POINTER);
	bonobo_object_signals [SYSTEM_EXCEPTION] =
		gtk_signal_new ("system_exception",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET(BonoboObjectClass,system_exception),
				gtk_marshal_NONE__POINTER_POINTER,
				GTK_TYPE_NONE, 2, GTK_TYPE_POINTER, GTK_TYPE_POINTER);
	bonobo_object_signals [OBJECT_GONE] =
		gtk_signal_new ("object_gone",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET(BonoboObjectClass,object_gone),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1, GTK_TYPE_POINTER);


	gtk_object_class_add_signals (object_class, bonobo_object_signals, LAST_SIGNAL);

	object_class->finalize = bonobo_object_finalize_real;

	init_object_corba_class ();
}

static void
bonobo_object_usage_error (BonoboObject *object)
{
	g_error ("Aggregate bonobo_object member [%p] has been "
		 "destroyed using gtk_object_* methods", object);
}

static void
bonobo_object_instance_init (GtkObject    *gtk_object,
			     GtkTypeClass *klass)
{
	BonoboObject *object = BONOBO_OBJECT (gtk_object);
	BonoboAggregateObject *ao;

	object->priv = g_new (BonoboObjectPrivate, 1);
	object->priv->destroy_id = gtk_signal_connect (
		gtk_object, "destroy", GTK_SIGNAL_FUNC (bonobo_object_usage_error),
		NULL);

	object->priv->ao = ao = g_new0 (BonoboAggregateObject, 1);

	ao->objs = g_list_append (object->priv->ao->objs, object);
	ao->ref_count = 1;

	object->corba_objref = CORBA_OBJECT_NIL;
	object->servant = NULL;

#ifdef BONOBO_REF_HOOKS
	{
		char *indent = g_strnfill (++ref_indent, ' ');
		g_printerr ("%s[%d]:Create %s:[%p] to %d\n", indent, getpid(),
			    gtk_type_name (klass->type),
			    ao, ao->ref_count);
		g_free (indent);

		g_assert (g_hash_table_lookup (living_ao_ht, ao) == NULL);
		g_hash_table_insert (living_ao_ht, ao, ao);
	}
#endif
}

/**
 * bonobo_object_get_type:
 *
 * Returns: the GtkType associated with the base BonoboObject class type.
 */
GtkType
bonobo_object_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		GtkTypeInfo info = {
			"BonoboObject",
			sizeof (BonoboObject),
			sizeof (BonoboObjectClass),
			(GtkClassInitFunc) bonobo_object_class_init,
			(GtkObjectInitFunc) bonobo_object_instance_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gtk_object_get_type (), &info);

#ifdef BONOBO_REF_HOOKS
		living_ao_ht = g_hash_table_new (NULL, NULL);
#endif
	}

	return type;
}

#ifdef BONOBO_REF_HOOKS
static void
bonobo_ao_debug_foreach (gpointer key, gpointer value, gpointer user_data)
{
	BonoboAggregateObject *ao = value;
	GList *l;

	g_return_if_fail (ao != NULL);

	g_print ("[%p]\tref_count=%d, %d interfaces:\n", ao,
		   ao->ref_count, g_list_length (ao->objs));
		
	for (l = ao->objs; l; l = l->next) {
		BonoboObject *object = BONOBO_OBJECT (l->data);

		g_print ("- [%p] %20s\tgtk_ref_count=%d\n", object,
			   gtk_type_name (GTK_OBJECT_TYPE (object)),
			   GTK_OBJECT (object)->ref_count);
	}
	g_print ("Referencing: \n");
	for (l = g_list_last (ao->refs); l; l = l->prev) {
		BonoboDebugRefData *descr = l->data;

		g_print ("%s -\t%s:%d\n", descr->ref?"ref":"unref",
			 descr->fn, descr->line);
	}
	g_print ("\n");
}
#endif

static void
bonobo_object_shutdown (void)
{
#ifdef BONOBO_REF_HOOKS

	if (living_ao_ht)
		g_hash_table_foreach (living_ao_ht,
				      bonobo_ao_debug_foreach, NULL);

	if (g_hash_table_size (living_ao_ht) > 0)
		g_print ("living bonobo objects count = %d\n",
			   g_hash_table_size (living_ao_ht));
	else
		g_print ("No object references leaked\n");

#endif
}

void
bonobo_object_init (void)
{
	g_atexit (bonobo_object_shutdown);
}

/**
 * bonobo_object_activate_servant:
 * @object: a BonoboObject
 * @servant: The servant to activate.
 *
 * This routine activates the @servant which is wrapped inside the
 * @object on the bonobo_poa (which is the default POA).
 *
 * Returns: The CORBA_Object that is wrapped by @object and whose
 * servant is @servant.  Might return CORBA_OBJECT_NIL on failure.
 */
CORBA_Object
bonobo_object_activate_servant (BonoboObject *object, void *servant)
{
	CORBA_Environment ev;
	Bonobo_Unknown o;

	g_return_val_if_fail (BONOBO_IS_OBJECT (object), CORBA_OBJECT_NIL);
	g_return_val_if_fail (servant != NULL, CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	CORBA_free (PortableServer_POA_activate_object (
		bonobo_poa (), servant, &ev));

	o = PortableServer_POA_servant_to_reference (
		bonobo_poa(), servant, &ev);

	CORBA_exception_free (&ev);

	if (o) {
		object->corba_objref = o;
		bonobo_object_bind_to_servant (object, servant);
		bonobo_running_context_add_object (o);
		return o;
	} else
		return CORBA_OBJECT_NIL;

}

/**
 * bonobo_object_construct:
 * @object: The GTK object server wrapper for the CORBA service.
 * @corba_object: the reference to the real CORBA object.
 *
 * Initializes the provided BonoboObject @object. This method is
 * usually invoked from the construct method for other Gtk-based CORBA
 * wrappers that derive from the Bonobo::Unknown interface
 *
 * Returns: the initialized BonoboObject.
 */
BonoboObject *
bonobo_object_construct (BonoboObject *object, CORBA_Object corba_object)
{
	g_return_val_if_fail (BONOBO_IS_OBJECT (object), NULL);
	g_return_val_if_fail (corba_object != CORBA_OBJECT_NIL, NULL);

	object->corba_objref = corba_object;

	/* BonoboObjects are self-owned */
	GTK_OBJECT_UNSET_FLAGS (GTK_OBJECT (object), GTK_FLOATING);

	return object;
}

/**
 * bonobo_object_add_interface:
 * @object: The BonoboObject to which an interface is going to be added.
 * @newobj: The BonoboObject containing the new interface to be added.
 *
 * Adds the interfaces supported by @newobj to the list of interfaces
 * for @object.  This function adds the interfaces supported by
 * @newobj to the list of interfaces support by @object. It should never
 * be used when the object has been exposed to the world. This is a firm
 * part of the contract.
 */
void
bonobo_object_add_interface (BonoboObject *object, BonoboObject *newobj)
{
       BonoboAggregateObject *oldao, *ao;
       GList *l;

       if (object->priv->ao == newobj->priv->ao)
               return;

       if (newobj->corba_objref == CORBA_OBJECT_NIL)
	       g_warning ("Adding an interface with a NULL Corba objref");

       /*
	* Explanation:
	*   Bonobo Objects should not be assembled after they have been
	*   exposed, or we would be breaking the contract we have with
	*   the other side.
	*
	*   This check is not perfect, but might help some people.
	*/

       ao = object->priv->ao;
       oldao = newobj->priv->ao;

       /* Merge the two AggregateObject lists */
       for (l = oldao->objs; l; l = l->next) {
	       BonoboObject *new_if = l->data;

	       /* FIXME: we prolly also want to check for duplicate interfaces */
               if (!g_list_find (ao->objs, new_if)) {
                       ao->objs = g_list_prepend (ao->objs, new_if);

		       new_if->priv->ao = ao;
               } else
		       g_warning ("attempting to merge identical interfaces [%p]", new_if);
       }

       g_assert (newobj->priv->ao == ao);

#ifdef BONOBO_REF_HOOKS
       g_assert (g_hash_table_lookup (living_ao_ht, oldao) == oldao);
       g_hash_table_remove (living_ao_ht, oldao);
#endif

       g_list_free (oldao->objs);
       g_free (oldao);
}

/**
 * bonobo_object_query_interface:
 * @object: A BonoboObject to be queried for a given interface.
 * @repo_id: The name of the interface to be queried.
 *
 * Returns: The CORBA interface named @repo_id for @object.
 */
CORBA_Object
bonobo_object_query_interface (BonoboObject *object, const char *repo_id)
{
       CORBA_Environment ev;
       CORBA_Object retval;

       CORBA_exception_init(&ev);
       retval = Bonobo_Unknown_queryInterface (object->corba_objref, (CORBA_char *)repo_id, &ev);
       if (BONOBO_EX (&ev))
               retval = CORBA_OBJECT_NIL;
       CORBA_exception_free (&ev);

       return retval;
}

/**
 * bonobo_object_corba_objref:
 * @object: A BonoboObject whose CORBA object is requested.
 *
 * Returns: The CORBA interface object for which @object is a wrapper.
 */
CORBA_Object
bonobo_object_corba_objref (BonoboObject *object)
{
	g_return_val_if_fail (BONOBO_IS_OBJECT (object), NULL);

	return object->corba_objref;
}

/**
 * bonobo_object_check_env:
 * @object: The object on which we operate
 * @ev: CORBA Environment to check
 *
 * This routine verifies the @ev environment for any fatal system
 * exceptions.  If a system exception occurs, the object raises a
 * "system_exception" signal.  The idea is that GtkObjects which are
 * used to wrap a CORBA interface can use this function to notify
 * the user if a fatal exception has occurred, causing the object
 * to become defunct.
 */
void
bonobo_object_check_env (BonoboObject *object, CORBA_Object obj, CORBA_Environment *ev)
{
	g_return_if_fail (ev != NULL);
	g_return_if_fail (BONOBO_IS_OBJECT (object));

	if (!BONOBO_EX (ev))
		return;

	if (ev->_major == CORBA_SYSTEM_EXCEPTION)
		gtk_signal_emit (
			GTK_OBJECT (object),
			bonobo_object_signals [SYSTEM_EXCEPTION],
			obj, ev);
}

/**
 * gnome_unknown_ping:
 * @object: a CORBA object reference of type Bonobo::Unknown
 *
 * Pings the object @object using the ref/unref methods from Bonobo::Unknown.
 * You can use this one to see if a remote object has gone away.
 *
 * Returns: %TRUE if the Bonobo::Unknown @object is alive.
 */
gboolean
gnome_unknown_ping (Bonobo_Unknown object)
{
	CORBA_Environment ev;
	gboolean alive;

	g_return_val_if_fail (object != NULL, FALSE);

	alive = FALSE;
	CORBA_exception_init (&ev);
	Bonobo_Unknown_ref (object, &ev);
	if (!BONOBO_EX (&ev)) {
		Bonobo_Unknown_unref (object, &ev);
		if (!BONOBO_EX (&ev))
			alive = TRUE;
	}
	CORBA_exception_free (&ev);

	return alive;
}

/**
 * bonobo_object_from_servant:
 * @servant: A Servant that implements the Bonobo::Unknown interface
 *
 * Returns: The servant @servant wrapped in a BonoboObject.
 */
BonoboObject *
bonobo_object_new_from_servant (void *servant)
{
	BonoboObject *object;
	CORBA_Object corba_object;

	g_return_val_if_fail (servant != NULL, NULL);

	object = gtk_type_new (bonobo_object_get_type ());
	if (object == NULL)
		return NULL;

	corba_object = bonobo_object_activate_servant (object, servant);
	bonobo_object_construct (object, corba_object);

	return object;
}

void
bonobo_object_dump_interfaces (BonoboObject *object)
{
	BonoboAggregateObject *ao;
	GList                 *l;

	g_return_if_fail (BONOBO_IS_OBJECT (object));

	ao = object->priv->ao;
	
	fprintf (stderr, "references %d\n", ao->ref_count);
	for (l = ao->objs; l; l = l->next) {
		BonoboObject *o = l->data;
		
		g_return_if_fail (BONOBO_IS_OBJECT (o));

		if (o->corba_objref && o->corba_objref->object_id)
			fprintf (stderr, "I/F: '%s'\n", o->corba_objref->object_id);
		else
			fprintf (stderr, "I/F: NIL error\n");
	}
}

static gboolean
idle_unref_fn (BonoboObject *object)
{
	bonobo_object_unref (object);

	return FALSE;
}

void
bonobo_object_idle_unref (BonoboObject *object)
{
	g_idle_add ((GSourceFunc) idle_unref_fn, object);
}


