/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * bonobo-persist-stream.c: PersistStream implementation.  Can be used as a
 * base class, or directly for implementing objects that use PersistStream.
 *
 * Author:
 *   Miguel de Icaza (miguel@kernel.org)
 *
 * Copyright 1999 Helix Code, Inc.
 */
#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-persist-stream.h>

/* Parent GTK object class */
static BonoboPersistClass *bonobo_persist_stream_parent_class;

/* The CORBA entry point vectors */
POA_Bonobo_PersistStream__vepv bonobo_persist_stream_vepv;

static CORBA_boolean
impl_is_dirty (PortableServer_Servant servant, CORBA_Environment * ev)
{
	BonoboObject *object = bonobo_object_from_servant (servant);
	BonoboPersistStream *pstream = BONOBO_PERSIST_STREAM (object);

	return pstream->is_dirty;
}

static void
impl_load (PortableServer_Servant servant,
	   Bonobo_Stream stream,
	   Bonobo_Persist_ContentType type,
	   CORBA_Environment *ev)
{
	BonoboObject *object = bonobo_object_from_servant (servant);
	BonoboPersistStream *ps = BONOBO_PERSIST_STREAM (object);
	
	if (ps->load_fn != NULL)
		(*ps->load_fn)(ps, stream, type, ps->closure, ev);
	else {
		GtkObjectClass *oc = GTK_OBJECT (ps)->klass;
		BonoboPersistStreamClass *class = BONOBO_PERSIST_STREAM_CLASS (oc);

		if (class->load)
			(*class->load)(ps, stream, type, ev);
		else
			CORBA_exception_set (
				ev, CORBA_USER_EXCEPTION,
				ex_Bonobo_NotSupported, NULL);
	}
}

static void
impl_save (PortableServer_Servant servant,
	   Bonobo_Stream stream,
	   Bonobo_Persist_ContentType type,
	   CORBA_Environment *ev)
{
	BonoboObject *object = bonobo_object_from_servant (servant);
	BonoboPersistStream *ps = BONOBO_PERSIST_STREAM (object);
	
	if (ps->save_fn != NULL)
		(*ps->save_fn)(ps, stream, type, ps->closure, ev);
	else {
		GtkObjectClass *oc = GTK_OBJECT (ps)->klass;
		BonoboPersistStreamClass *class = BONOBO_PERSIST_STREAM_CLASS (oc);

		if (class->save)
			(*class->save)(ps, stream, type, ev);
		else
			CORBA_exception_set (
				ev, CORBA_USER_EXCEPTION,
				ex_Bonobo_NotSupported, NULL);
	}

	ps->is_dirty = FALSE;
}

static CORBA_long
impl_get_size_max (PortableServer_Servant servant, CORBA_Environment * ev)
{
	BonoboObject *object = bonobo_object_from_servant (servant);
	BonoboPersistStream *ps = BONOBO_PERSIST_STREAM (object);
	GtkObjectClass *oc = GTK_OBJECT (object)->klass;
	BonoboPersistStreamClass *class = BONOBO_PERSIST_STREAM_CLASS (oc);


	if (ps->max_fn != NULL)
		return (*ps->max_fn)(ps, ps->closure, ev);

	return (*class->get_size_max)(BONOBO_PERSIST_STREAM (object), ev);
}

/**
 * bonobo_persist_stream_get_epv:
 *
 * Returns: The EPV for the default BonoboPersistStream implementation.  
 */
POA_Bonobo_PersistStream__epv *
bonobo_persist_stream_get_epv (void)
{
	POA_Bonobo_PersistStream__epv *epv;

	epv = g_new0 (POA_Bonobo_PersistStream__epv, 1);

	epv->load       = impl_load;
	epv->save       = impl_save;
	epv->getMaxSize = impl_get_size_max;
	epv->isDirty    = impl_is_dirty;

	return epv;
}

static void
init_persist_stream_corba_class (void)
{
	bonobo_persist_stream_vepv.Bonobo_Unknown_epv = bonobo_object_get_epv ();
	bonobo_persist_stream_vepv.Bonobo_Persist_epv = bonobo_persist_get_epv ();
	bonobo_persist_stream_vepv.Bonobo_PersistStream_epv = bonobo_persist_stream_get_epv ();
}

static CORBA_long
bonobo_persist_stream_size_unknown (BonoboPersistStream *ps,
				    CORBA_Environment *ev)
{
	return -1;
}

static Bonobo_Persist_ContentTypeList *
get_content_types (BonoboPersist *persist, CORBA_Environment *ev)
{
	BonoboPersistStream *ps = BONOBO_PERSIST_STREAM (persist);

	if (ps->types_fn)
		return ps->types_fn (ps, ps->closure, ev);
	else
		return bonobo_persist_generate_content_types (1, "");
}

static void
bonobo_persist_stream_class_init (BonoboPersistStreamClass *klass)
{
	BonoboPersistClass *persist_class = BONOBO_PERSIST_CLASS (klass);

	bonobo_persist_stream_parent_class = gtk_type_class (bonobo_persist_get_type ());

	/*
	 * Override and initialize methods
	 */

	klass->save = NULL;
	klass->load = NULL;
	klass->get_size_max = bonobo_persist_stream_size_unknown;

	persist_class->get_content_types = get_content_types;

	init_persist_stream_corba_class ();
}

static void
bonobo_persist_stream_init (BonoboPersistStream *ps)
{
}

/**
 * bonobo_persist_stream_get_type:
 *
 * Returns: The GtkType for the BonoboPersistStream class.
 */
GtkType
bonobo_persist_stream_get_type (void)
{
	static GtkType type = 0;

	if (!type){
		GtkTypeInfo info = {
			"BonoboPersistStream",
			sizeof (BonoboPersistStream),
			sizeof (BonoboPersistStreamClass),
			(GtkClassInitFunc) bonobo_persist_stream_class_init,
			(GtkObjectInitFunc) bonobo_persist_stream_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (bonobo_persist_get_type (), &info);
	}

	return type;
}

/**
 * bonobo_persist_stream_construct:
 * @ps: A GnomerPersistStream object
 * @load_fn: Loading routine
 * @save_fn: Saving routine
 * @closure: Data passed to IO routines.
 *
 * Initializes the BonoboPersistStream object.  The load and save
 * operations for the object are performed by the provided @load_fn
 * and @save_fn callback functions, which are passed @closure when
 * they are invoked.  If either @load_fn or @save_fn is %NULL, the
 * corresponding operation is performed by the class load and save
 * routines.
 *
 * Returns: The initialized BonoboPersistStream object.
 */
BonoboPersistStream *
bonobo_persist_stream_construct (BonoboPersistStream *ps,
				 Bonobo_PersistStream corba_ps,
				 BonoboPersistStreamIOFn load_fn,
				 BonoboPersistStreamIOFn save_fn,
				 BonoboPersistStreamMaxFn max_fn,
				 BonoboPersistStreamTypesFn types_fn,
				 void *closure)
{
	g_return_val_if_fail (ps != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_PERSIST_STREAM (ps), NULL);
	g_return_val_if_fail (corba_ps != CORBA_OBJECT_NIL, NULL);

	bonobo_persist_construct (BONOBO_PERSIST (ps), corba_ps);
	
	ps->load_fn = load_fn;
	ps->save_fn = save_fn;
	ps->max_fn = max_fn;
	ps->types_fn = types_fn;
	ps->closure = closure;
	
	return ps;
}

static Bonobo_PersistStream
create_bonobo_persist_stream (BonoboObject *object)
{
	POA_Bonobo_PersistStream *servant;
	CORBA_Environment ev;

	servant = (POA_Bonobo_PersistStream *) g_new0 (BonoboObjectServant, 1);
	servant->vepv = &bonobo_persist_stream_vepv;

	CORBA_exception_init (&ev);

	POA_Bonobo_PersistStream__init ((PortableServer_Servant) servant, &ev);
	if (BONOBO_EX (&ev)){
		g_free (servant);
		CORBA_exception_free (&ev);
		return CORBA_OBJECT_NIL;
	}

	CORBA_exception_free (&ev);
	return (Bonobo_PersistStream) bonobo_object_activate_servant (object, servant);
}


/**
 * bonobo_persist_stream_new:
 * @load_fn: Loading routine
 * @save_fn: Saving routine
 * @max_fn: get_max_size routine
 * @types_fn: get_content_types routine
 * @closure: Data passed to IO routines.
 *
 * Creates a new BonoboPersistStream object. The various operations
 * for the object are performed by the provided callback functions,
 * which are passed @closure when they are invoked. If any callback is
 * %NULL, the corresponding operation is performed by the class load
 * and save routines.
 *
 * Returns: the newly-created BonoboPersistStream object.
 */
BonoboPersistStream *
bonobo_persist_stream_new (BonoboPersistStreamIOFn load_fn,
			   BonoboPersistStreamIOFn save_fn,
			   BonoboPersistStreamMaxFn max_fn,
			   BonoboPersistStreamTypesFn types_fn,
			   void *closure)
{
	BonoboPersistStream *ps;
	Bonobo_PersistStream corba_ps;

	ps = gtk_type_new (bonobo_persist_stream_get_type ());
	corba_ps = create_bonobo_persist_stream (
		BONOBO_OBJECT (ps));
	if (corba_ps == CORBA_OBJECT_NIL) {
		bonobo_object_unref (BONOBO_OBJECT (ps));
		return NULL;
	}

	bonobo_persist_stream_construct (ps, corba_ps, load_fn, save_fn,
					 max_fn, types_fn, closure);

	return ps;
}

/**
 * bonobo_persist_stream_set_dirty:
 * @ps: A GnomerPersistStream object
 * @dirty: A boolean value representing whether the object is dirty or not
 *
 * This routine sets the dirty bit for the PersistStream object.
 */
void
bonobo_persist_stream_set_dirty (BonoboPersistStream *pstream, gboolean dirty)
{
	g_return_if_fail (pstream != NULL);
	g_return_if_fail (BONOBO_IS_PERSIST_STREAM (pstream));

	pstream->is_dirty = dirty;
}
