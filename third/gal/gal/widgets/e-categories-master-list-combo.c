/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * e-categories-master-list-combo.c
 * Copyright 2000, 2001, Ximian, Inc.
 *
 * Authors:
 *   Chris Lahey <clahey@ximian.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License, version 2, as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>

#include "e-categories-master-list-combo.h"

#include <gal/util/e-i18n.h>
#include <gal/util/e-util.h>
#include <gal/widgets/e-unicode.h>
#include <gtk/gtksignal.h>

struct ECategoriesMasterListComboPriv {
	ECategoriesMasterList *ecml;
	int ecml_changed_signal_id;
};

#define PARENT_TYPE (gtk_combo_get_type())

static GtkObjectClass *parent_class;

/* The arguments we take */
enum {
	ARG_0,
	ARG_ECML,
};

static void
ecmlc_ecml_changed (ECategoriesMasterList *ecml, ECategoriesMasterListCombo *ecmlc)
{
	int count = e_categories_master_list_count (ecml);
	int i;
	GList *strings = NULL;
	for (i = 0; i < count; i++) {
		char *category;

		category = e_utf8_to_gtk_string (
			GTK_WIDGET (ecmlc),
			e_categories_master_list_nth (ecml, i));
		strings = g_list_prepend (strings, category);
	}
	strings = g_list_sort (strings, (GCompareFunc) g_utf8_collate);
	strings = g_list_prepend (strings, g_strdup (""));
	gtk_combo_set_popdown_strings (GTK_COMBO (ecmlc), strings);
	g_list_foreach (strings, (GFunc) g_free, NULL);
	g_list_free (strings);
}

static void
ecmlc_add_ecml (ECategoriesMasterListCombo *ecmlc,
		ECategoriesMasterList *ecml)
{
	if (ecmlc->priv->ecml)
		return;

	ecmlc->priv->ecml = ecml;
	if (ecml) {
		gtk_object_ref (GTK_OBJECT (ecml));
		ecmlc->priv->ecml_changed_signal_id =
			gtk_signal_connect (GTK_OBJECT (ecml), "changed",
					    GTK_SIGNAL_FUNC (ecmlc_ecml_changed), ecmlc);
		ecmlc_ecml_changed (ecml, ecmlc);
	}
}

static void
ecmlc_remove_ecml (ECategoriesMasterListCombo *ecmlc)
{
	if (ecmlc->priv->ecml) {
		if (ecmlc->priv->ecml_changed_signal_id)
			gtk_signal_disconnect (GTK_OBJECT (ecmlc->priv->ecml),
					       ecmlc->priv->ecml_changed_signal_id);
		gtk_object_unref (GTK_OBJECT (ecmlc->priv->ecml));
	}
	ecmlc->priv->ecml = NULL;
	ecmlc->priv->ecml_changed_signal_id = 0;
}

static void
ecmlc_destroy (GtkObject *object)
{
	ECategoriesMasterListCombo *ecmlc = E_CATEGORIES_MASTER_LIST_COMBO (object);

	ecmlc_remove_ecml (ecmlc);
	g_free (ecmlc->priv);
	ecmlc->priv = NULL;
	
	GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
ecmlc_set_arg (GtkObject *o, GtkArg *arg, guint arg_id)
{
	ECategoriesMasterListCombo *ecmlc;

	ecmlc = E_CATEGORIES_MASTER_LIST_COMBO (o);
	
	switch (arg_id){
	case ARG_ECML:
		ecmlc_remove_ecml (ecmlc);
		ecmlc_add_ecml (ecmlc, (ECategoriesMasterList *) GTK_VALUE_OBJECT (*arg));
		break;
	}
}

static void
ecmlc_get_arg (GtkObject *o, GtkArg *arg, guint arg_id)
{
	ECategoriesMasterListCombo *ecmlc;

	ecmlc = E_CATEGORIES_MASTER_LIST_COMBO (o);

	switch (arg_id) {
	case ARG_ECML:
		GTK_VALUE_OBJECT (*arg) = (GtkObject *) ecmlc->priv->ecml;
		break;

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
ecmlc_class_init (GtkObjectClass *object_class)
{
	parent_class   = gtk_type_class (PARENT_TYPE);

	object_class->destroy = ecmlc_destroy;
	object_class->set_arg = ecmlc_set_arg;
	object_class->get_arg = ecmlc_get_arg;

	gtk_object_add_arg_type ("ECategoriesMasterListCombo::ecml", E_CATEGORIES_MASTER_LIST_TYPE,
				 GTK_ARG_READWRITE, ARG_ECML);
}

static void
ecmlc_init (ECategoriesMasterListCombo *ecmlc)
{
	ecmlc->priv                         = g_new (ECategoriesMasterListComboPriv, 1);

	ecmlc->priv->ecml                   = NULL;
	ecmlc->priv->ecml_changed_signal_id = 0;
}

/**
 * e_categories_master_list_combo_construct: Constructs a given combo object.
 * @ecmlc: The combo to construct.
 * @ecml: The master list to use.
 * 
 * Construct the given combo.  Sets the ecml.
 * 
 * Return value: the given combo as a GtkWidget.
 **/
GtkWidget *
e_categories_master_list_combo_construct (ECategoriesMasterListCombo *ecmlc,
					  ECategoriesMasterList       *ecml)
{
	g_return_val_if_fail (ecmlc != NULL, NULL);
	g_return_val_if_fail (ecml != NULL, NULL);

	gtk_object_set (GTK_OBJECT (ecmlc),
			"ecml", ecml,
			NULL);

	return GTK_WIDGET (ecmlc);
}

/**
 * e_categories_master_list_combo_new:
 *
 * Creates a new ECategoriesMasterListCombo object.
 *
 * Returns: The ECategoriesMasterListCombo object.
 */
GtkWidget *
e_categories_master_list_combo_new (ECategoriesMasterList *ecml)
{
	ECategoriesMasterListCombo *ecmlc = gtk_type_new (E_CATEGORIES_MASTER_LIST_COMBO_TYPE);

	if (e_categories_master_list_combo_construct (ecmlc, ecml) == NULL){
		gtk_object_destroy (GTK_OBJECT (ecmlc));
		return NULL;
	}

	return GTK_WIDGET (ecmlc);
}

E_MAKE_TYPE(e_categories_master_list_combo, "ECategoriesMasterListCombo", ECategoriesMasterListCombo, ecmlc_class_init, ecmlc_init, PARENT_TYPE);
