/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* NautilusUndoTransaction - An object for an undoable transaction.
 *                           Used internally by undo machinery.
 *                           Not public.
 *
 * Copyright (C) 2000 Eazel, Inc.
 *
 * Author: Gene Z. Ragan <gzr@eazel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "nautilus-undo-transaction.h"

#include "nautilus-undo-private.h"
#include <bonobo/bonobo-main.h>
#include <gtk/gtksignal.h>
#include <eel/eel-gtk-macros.h>

#define NAUTILUS_UNDO_TRANSACTION_LIST_DATA "Nautilus undo transaction list"

static void nautilus_undo_transaction_undo       (NautilusUndoTransaction      *transaction);

/* undo atoms */
static void undo_atom_list_free                  (GList                        *list);
static void undo_atom_list_undo_and_free         (GList                        *list);

BONOBO_CLASS_BOILERPLATE_FULL (NautilusUndoTransaction, nautilus_undo_transaction,
			       Nautilus_Undo_Transaction,
			       BonoboObject, BONOBO_OBJECT_TYPE)

static Nautilus_Undo_MenuItem *
impl_Nautilus_Undo_Transaction__get_undo_menu_item (PortableServer_Servant servant,
						    CORBA_Environment *ev)
{
	NautilusUndoTransaction *transaction;
	Nautilus_Undo_MenuItem *item;

	transaction = NAUTILUS_UNDO_TRANSACTION (bonobo_object_from_servant (servant));
	
	item = Nautilus_Undo_MenuItem__alloc ();
     	item->label = CORBA_string_dup (transaction->undo_menu_item_label);
     	item->hint = CORBA_string_dup (transaction->undo_menu_item_hint);
	
	return item;
}
							 				
static Nautilus_Undo_MenuItem *
impl_Nautilus_Undo_Transaction__get_redo_menu_item (PortableServer_Servant servant,
						    CORBA_Environment *ev)
{
	NautilusUndoTransaction *transaction;
	Nautilus_Undo_MenuItem *item;

	transaction = NAUTILUS_UNDO_TRANSACTION (bonobo_object_from_servant (servant));
	
	item = Nautilus_Undo_MenuItem__alloc ();
     	item->label = CORBA_string_dup (transaction->redo_menu_item_label);
     	item->hint = CORBA_string_dup (transaction->redo_menu_item_hint);
	
	return item;
}

static CORBA_char *
impl_Nautilus_Undo_Transaction__get_operation_name (PortableServer_Servant servant,
						    CORBA_Environment *ev)
{
	NautilusUndoTransaction *transaction;
	
	transaction = NAUTILUS_UNDO_TRANSACTION (bonobo_object_from_servant (servant));
	return CORBA_string_dup (transaction->operation_name);
}

static void
impl_Nautilus_Undo_Transaction__undo (PortableServer_Servant servant,
				      CORBA_Environment *ev)
{
	NautilusUndoTransaction *transaction;
	
	transaction = NAUTILUS_UNDO_TRANSACTION (bonobo_object_from_servant (servant));
	nautilus_undo_transaction_undo (transaction);
}

NautilusUndoTransaction *
nautilus_undo_transaction_new (const char *operation_name,
			       const char *undo_menu_item_label,
			       const char *undo_menu_item_hint,
			       const char *redo_menu_item_label,
			       const char *redo_menu_item_hint)
{
	NautilusUndoTransaction *transaction;

	transaction = NAUTILUS_UNDO_TRANSACTION (g_object_new (nautilus_undo_transaction_get_type (), NULL));
	
	transaction->operation_name = g_strdup (operation_name);
	transaction->undo_menu_item_label = g_strdup (undo_menu_item_label);
	transaction->undo_menu_item_hint = g_strdup (undo_menu_item_hint);
	transaction->redo_menu_item_label = g_strdup (redo_menu_item_label);
	transaction->redo_menu_item_hint = g_strdup (redo_menu_item_hint);

	return transaction;
}

static void 
nautilus_undo_transaction_instance_init (NautilusUndoTransaction *transaction)
{
}

static void
remove_transaction_from_object (gpointer list_data, gpointer callback_data)
{
	NautilusUndoAtom *atom;
	NautilusUndoTransaction *transaction;
	GList *list;
	
	g_assert (list_data != NULL);
	atom = list_data;
	transaction = NAUTILUS_UNDO_TRANSACTION (callback_data);

	/* Remove the transaction from the list on the atom. */
	list = g_object_get_data (atom->target, NAUTILUS_UNDO_TRANSACTION_LIST_DATA);

	if (list != NULL) {
		list = g_list_remove (list, transaction);
		g_object_set_data (atom->target, NAUTILUS_UNDO_TRANSACTION_LIST_DATA, list);
	}
}

static void
remove_transaction_from_atom_targets (NautilusUndoTransaction *transaction)
{

	g_list_foreach (transaction->atom_list,
			remove_transaction_from_object,
			transaction);	
}

static void
nautilus_undo_transaction_finalize (GObject *object)
{
	NautilusUndoTransaction *transaction;

	transaction = NAUTILUS_UNDO_TRANSACTION (object);
	
	remove_transaction_from_atom_targets (transaction);
	undo_atom_list_free (transaction->atom_list);

	g_free (transaction->operation_name);
	g_free (transaction->undo_menu_item_label);
	g_free (transaction->undo_menu_item_hint);
	g_free (transaction->redo_menu_item_label);
	g_free (transaction->redo_menu_item_hint);

	CORBA_Object_release (transaction->owner, NULL);
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
nautilus_undo_transaction_add_atom (NautilusUndoTransaction *transaction, 
				    const NautilusUndoAtom *atom)
{
	GList *list;
	
	g_return_if_fail (NAUTILUS_IS_UNDO_TRANSACTION (transaction));
	g_return_if_fail (atom != NULL);
	g_return_if_fail (GTK_IS_OBJECT (atom->target));

	/* Add the atom to the atom list in the transaction. */
	transaction->atom_list = g_list_append
		(transaction->atom_list, g_memdup (atom, sizeof (*atom)));

	/* Add the transaction to the list on the atoms target object. */
	list = g_object_get_data (atom->target, NAUTILUS_UNDO_TRANSACTION_LIST_DATA);
	if (g_list_find (list, transaction) != NULL) {
		return;
	}

	/* If it's not already on that atom, this object is new. */
	list = g_list_prepend (list, transaction);
	g_object_set_data (atom->target, NAUTILUS_UNDO_TRANSACTION_LIST_DATA, list);

	/* Connect a signal handler to the atom so it will unregister
	 * itself when it's destroyed.
	 */
	g_signal_connect (atom->target, "destroy",
			  G_CALLBACK (nautilus_undo_transaction_unregister_object),
			  NULL);
}

void 
nautilus_undo_transaction_undo (NautilusUndoTransaction *transaction)
{
	g_return_if_fail (NAUTILUS_IS_UNDO_TRANSACTION (transaction));

	remove_transaction_from_atom_targets (transaction);
	undo_atom_list_undo_and_free (transaction->atom_list);
	
	transaction->atom_list = NULL;
}

void
nautilus_undo_transaction_add_to_undo_manager (NautilusUndoTransaction *transaction,
					       Nautilus_Undo_Manager manager)
{
	CORBA_Environment ev;

	g_return_if_fail (NAUTILUS_IS_UNDO_TRANSACTION (transaction));
	g_return_if_fail (transaction->owner == CORBA_OBJECT_NIL);

	CORBA_exception_init (&ev);

	if (!CORBA_Object_is_nil (manager, &ev)) {
		Nautilus_Undo_Manager_append (manager, BONOBO_OBJREF (transaction), &ev);
		transaction->owner = CORBA_Object_duplicate (manager, &ev);
	}

	CORBA_exception_free (&ev);
}

static void
remove_atoms (NautilusUndoTransaction *transaction,
	      GObject *object)
{
	CORBA_Environment ev;
	GList *p, *next;
	NautilusUndoAtom *atom;

	g_assert (NAUTILUS_IS_UNDO_TRANSACTION (transaction));
	g_assert (G_IS_OBJECT (object));

	CORBA_exception_init (&ev);

	/* Destroy any atoms for this object. */
	for (p = transaction->atom_list; p != NULL; p = next) {
		atom = p->data;
		next = p->next;

		if (atom->target == object) {
			transaction->atom_list = g_list_remove_link
				(transaction->atom_list, p);
			undo_atom_list_free (p);
		}
	}

	/* If all the atoms are gone, forget this transaction.
	 * This may end up freeing the transaction.
	 */
	if (transaction->atom_list == NULL) {
		Nautilus_Undo_Manager_forget (
			transaction->owner, BONOBO_OBJREF (transaction), &ev);
	}

	CORBA_exception_free (&ev);
}

static void
remove_atoms_cover (gpointer list_data, gpointer callback_data)
{
	if (NAUTILUS_IS_UNDO_TRANSACTION (list_data)) {
		remove_atoms (NAUTILUS_UNDO_TRANSACTION (list_data), G_OBJECT (callback_data));
	}
}

void
nautilus_undo_transaction_unregister_object (GObject *object)
{
	GList *list;

	g_return_if_fail (G_IS_OBJECT (object));

	/* Remove atoms from each transaction on the list. */
	list = g_object_get_data (object, NAUTILUS_UNDO_TRANSACTION_LIST_DATA);
	if (list != NULL) {
		g_list_foreach (list, remove_atoms_cover, object);	
		g_list_free (list);
		g_object_set_data (object, NAUTILUS_UNDO_TRANSACTION_LIST_DATA, NULL);
	}
}

static void
undo_atom_free (NautilusUndoAtom *atom)
{
	/* Call the destroy-notify function if it's present. */
	if (atom->callback_data_destroy_notify != NULL) {
		(* atom->callback_data_destroy_notify) (atom->callback_data);
	}

	/* Free the atom storage. */
	g_free (atom);
}

static void
undo_atom_undo_and_free (NautilusUndoAtom *atom)
{
	/* Call the function that does the actual undo. */
	(* atom->callback) (atom->target, atom->callback_data);

	/* Get rid of the atom now that it's spent. */
	undo_atom_free (atom);
}

static void
undo_atom_free_cover (gpointer atom, gpointer callback_data)
{
	g_assert (atom != NULL);
	g_assert (callback_data == NULL);
	undo_atom_free (atom);
}

static void
undo_atom_undo_and_free_cover (gpointer atom, gpointer callback_data)
{
	g_assert (atom != NULL);
	g_assert (callback_data == NULL);
	undo_atom_undo_and_free (atom);
}

static void
undo_atom_list_free (GList *list)
{
	g_list_foreach (list, undo_atom_free_cover, NULL);
	g_list_free (list);
}

static void
undo_atom_list_undo_and_free (GList *list)
{
	g_list_foreach (list, undo_atom_undo_and_free_cover, NULL);
	g_list_free (list);
}

static void
nautilus_undo_transaction_class_init (NautilusUndoTransactionClass *klass)
{
	POA_Nautilus_Undo_Transaction__epv *epv = &klass->epv;

	G_OBJECT_CLASS (klass)->finalize = nautilus_undo_transaction_finalize;
	
	epv->_get_undo_menu_item = &impl_Nautilus_Undo_Transaction__get_undo_menu_item;
	epv->_get_redo_menu_item = &impl_Nautilus_Undo_Transaction__get_redo_menu_item;
	epv->_get_operation_name = &impl_Nautilus_Undo_Transaction__get_operation_name;
	epv->undo = &impl_Nautilus_Undo_Transaction__undo;
}
