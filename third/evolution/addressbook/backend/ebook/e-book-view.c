/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * The Evolution addressbook client object.
 *
 * Author:
 *   Nat Friedman (nat@ximian.com)
 *
 * Copyright 1999, 2000, Ximian, Inc.
 */

#include <config.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmarshal.h>

#include "addressbook.h"
#include "e-card-cursor.h"
#include "e-book-view-listener.h"
#include "e-book-view.h"
#include "e-book.h"

GtkObjectClass *e_book_view_parent_class;

struct _EBookViewPrivate {
	GNOME_Evolution_Addressbook_BookView     corba_book_view;

	EBook                 *book;
	
	EBookViewListener     *listener;

	int                    responses_queued_id;
};

enum {
	CARD_CHANGED,
	CARD_REMOVED,
	CARD_ADDED,
	SEQUENCE_COMPLETE,
	STATUS_MESSAGE,
	LAST_SIGNAL
};

static guint e_book_view_signals [LAST_SIGNAL];

static void
add_book_iterator (gpointer data, gpointer closure)
{
	ECard *card = E_CARD (data);
	EBook *book = E_BOOK (closure);

	e_card_set_book (card, book);
}

static void
e_book_view_do_added_event (EBookView                 *book_view,
			    EBookViewListenerResponse *resp)
{
	if (book_view->priv->book)
		g_list_foreach (resp->cards, add_book_iterator, book_view->priv->book);

	gtk_signal_emit (GTK_OBJECT (book_view), e_book_view_signals [CARD_ADDED],
			 resp->cards);

	g_list_foreach (resp->cards, (GFunc) gtk_object_unref, NULL);
	g_list_free (resp->cards);
}

static void
e_book_view_do_modified_event (EBookView                 *book_view,
			       EBookViewListenerResponse *resp)
{
	if (book_view->priv->book)
		g_list_foreach (resp->cards, add_book_iterator, book_view->priv->book);

	gtk_signal_emit (GTK_OBJECT (book_view), e_book_view_signals [CARD_CHANGED],
			 resp->cards);

	g_list_foreach (resp->cards, (GFunc) gtk_object_unref, NULL);
	g_list_free (resp->cards);
}

static void
e_book_view_do_removed_event (EBookView                 *book_view,
			      EBookViewListenerResponse *resp)
{
	gtk_signal_emit (GTK_OBJECT (book_view), e_book_view_signals [CARD_REMOVED],
			 resp->id);

	g_free(resp->id);
}

static void
e_book_view_do_complete_event (EBookView                 *book_view,
			      EBookViewListenerResponse *resp)
{
	gtk_signal_emit (GTK_OBJECT (book_view), e_book_view_signals [SEQUENCE_COMPLETE]);
}

static void
e_book_view_do_status_message_event (EBookView                 *book_view,
				     EBookViewListenerResponse *resp)
{
	gtk_signal_emit (GTK_OBJECT (book_view), e_book_view_signals [STATUS_MESSAGE],
			 resp->message);
	g_free(resp->message);
}


/*
 * Reading notices out of the EBookViewListener's queue.
 */
static void
e_book_view_check_listener_queue (EBookViewListener *listener, EBookView *book_view)
{
	EBookViewListenerResponse *resp;

	resp = e_book_view_listener_pop_response (listener);
	
	if (resp == NULL)
		return;

	switch (resp->op) {
	case CardAddedEvent:
		e_book_view_do_added_event (book_view, resp);
		break;
	case CardModifiedEvent:
		e_book_view_do_modified_event (book_view, resp);
		break;
	case CardRemovedEvent:
		e_book_view_do_removed_event (book_view, resp);
		break;
	case SequenceCompleteEvent:
		e_book_view_do_complete_event (book_view, resp);
		break;
	case StatusMessageEvent:
		e_book_view_do_status_message_event (book_view, resp);
		break;
	default:
		g_error ("EBookView: Unknown operation %d in listener queue!\n",
			 resp->op);
		break;
	}

	g_free (resp);
}

static gboolean
e_book_view_construct (EBookView *book_view, GNOME_Evolution_Addressbook_BookView corba_book_view, EBookViewListener *listener)
{
	CORBA_Environment  ev;
	g_return_val_if_fail (book_view != NULL,     FALSE);
	g_return_val_if_fail (E_IS_BOOK_VIEW (book_view), FALSE);

	/*
	 * Copy in the corba_book_view.
	 */
	CORBA_exception_init (&ev);

	book_view->priv->corba_book_view = bonobo_object_dup_ref(corba_book_view, &ev);

	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("e_book_view_construct: Exception duplicating corba_book_view.\n");
		CORBA_exception_free (&ev);
		book_view->priv->corba_book_view = NULL;
		return FALSE;
	}

	CORBA_exception_free (&ev);

	/*
	 * Create our local BookListener interface.
	 */
	book_view->priv->listener = listener;
	book_view->priv->responses_queued_id = gtk_signal_connect (GTK_OBJECT (book_view->priv->listener), "responses_queued",
								   e_book_view_check_listener_queue, book_view);

	bonobo_object_ref(BONOBO_OBJECT(book_view->priv->listener));

	return TRUE;
}

/**
 * e_book_view_new:
 */
EBookView *
e_book_view_new (GNOME_Evolution_Addressbook_BookView corba_book_view, EBookViewListener *listener)
{
	EBookView *book_view;

	book_view = gtk_type_new (E_BOOK_VIEW_TYPE);

	if (! e_book_view_construct (book_view, corba_book_view, listener)) {
		gtk_object_unref (GTK_OBJECT (book_view));
		return NULL;
	}

	return book_view;
}

void
e_book_view_set_book (EBookView *book_view, EBook *book)
{
	g_return_if_fail (book_view && E_IS_BOOK_VIEW (book_view));
	g_return_if_fail (book && E_IS_BOOK (book));
	g_return_if_fail (book_view->priv->book == NULL);

	book_view->priv->book = book;
	gtk_object_ref (GTK_OBJECT (book));
}

void
e_book_view_stop (EBookView *book_view)
{
	g_return_if_fail (book_view && E_IS_BOOK_VIEW (book_view));
	if (book_view->priv->listener)
		e_book_view_listener_stop (book_view->priv->listener);
}

static void
e_book_view_init (EBookView *book_view)
{
	book_view->priv                      = g_new0 (EBookViewPrivate, 1);
	book_view->priv->book                = NULL;
	book_view->priv->corba_book_view     = CORBA_OBJECT_NIL;
	book_view->priv->listener            = NULL;
	book_view->priv->responses_queued_id = 0;
}

static void
e_book_view_destroy (GtkObject *object)
{
	EBookView             *book_view = E_BOOK_VIEW (object);
	CORBA_Environment  ev;

	if (book_view->priv->book) {
		gtk_object_unref (GTK_OBJECT (book_view->priv->book));
	}

	if (book_view->priv->corba_book_view) {
		CORBA_exception_init (&ev);

		bonobo_object_release_unref (book_view->priv->corba_book_view, &ev);

		if (ev._major != CORBA_NO_EXCEPTION) {
			g_warning ("EBookView: Exception while releasing BookView\n");
		}

		CORBA_exception_free (&ev);
	}

	if (book_view->priv->listener) {
		if (book_view->priv->responses_queued_id)
			gtk_signal_disconnect(GTK_OBJECT(book_view->priv->listener),
					      book_view->priv->responses_queued_id);
		e_book_view_listener_stop (book_view->priv->listener);
		bonobo_object_unref (BONOBO_OBJECT(book_view->priv->listener));
	}

	g_free (book_view->priv);

	if (GTK_OBJECT_CLASS (e_book_view_parent_class)->destroy)
		GTK_OBJECT_CLASS (e_book_view_parent_class)->destroy (object);
}

static void
e_book_view_class_init (EBookViewClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *) klass;

	e_book_view_parent_class = gtk_type_class (gtk_object_get_type ());

	e_book_view_signals [CARD_CHANGED] =
		gtk_signal_new ("card_changed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (EBookViewClass, card_changed),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_POINTER);

	e_book_view_signals [CARD_ADDED] =
		gtk_signal_new ("card_added",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (EBookViewClass, card_added),
				gtk_marshal_NONE__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);

	e_book_view_signals [CARD_REMOVED] =
		gtk_signal_new ("card_removed",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (EBookViewClass, card_removed),
				gtk_marshal_NONE__POINTER,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_POINTER);

	e_book_view_signals [SEQUENCE_COMPLETE] =
		gtk_signal_new ("sequence_complete",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (EBookViewClass, sequence_complete),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);

	e_book_view_signals [STATUS_MESSAGE] =
		gtk_signal_new ("status_message",
				GTK_RUN_LAST,
				object_class->type,
				GTK_SIGNAL_OFFSET (EBookViewClass, status_message),
				gtk_marshal_NONE__STRING,
				GTK_TYPE_NONE, 1,
				GTK_TYPE_STRING);

	gtk_object_class_add_signals (object_class, e_book_view_signals,
				      LAST_SIGNAL);

	klass->card_changed = NULL;
	klass->card_added = NULL;
	klass->card_removed = NULL;
	klass->sequence_complete = NULL;
	klass->status_message = NULL;

	object_class->destroy = e_book_view_destroy;
}

/**
 * e_book_view_get_type:
 */
GtkType
e_book_view_get_type (void)
{
	static GtkType type = 0;

	if (! type) {
		GtkTypeInfo info = {
			"EBookView",
			sizeof (EBookView),
			sizeof (EBookViewClass),
			(GtkClassInitFunc)  e_book_view_class_init,
			(GtkObjectInitFunc) e_book_view_init,
			NULL, /* reserved 1 */
			NULL, /* reserved 2 */
			(GtkClassInitFunc) NULL
		};

		type = gtk_type_unique (gtk_object_get_type (), &info);
	}

	return type;
}
