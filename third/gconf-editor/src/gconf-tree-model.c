#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gconf-tree-model.h"

typedef struct _Node Node;

#define NODE(x) ((Node *)(x))

static GdkPixbuf *closed_icon;
static GdkPixbuf *open_icon;


struct _Node {
	gint ref_count;
	gint offset;
	gchar *path;

	Node *parent;
	Node *children;
	Node *next;
	Node *prev;
};

GtkTreePath *
gconf_tree_model_get_tree_path_from_gconf_path (GConfTreeModel *tree_model, const char *key)
{
	GtkTreePath *path;
	GtkTreeIter iter, child_iter;
	gchar *tmp_str;
	gchar **key_array;
	int i;
	gboolean found;

	g_assert (key[0] == '/');

	/* special case root node */
	if (strlen (key) == 1 && key[0] == '/')
		return gtk_tree_path_new_from_string ("0");

	key_array = g_strsplit (key + 1, "/", 0);

	if (!gtk_tree_model_get_iter_root (GTK_TREE_MODEL (tree_model), &iter)) {

		/* Ugh, something is terribly wrong */
		g_strfreev (key_array);
		return NULL;
	}

	for (i = 0; key_array[i] != NULL; i++) {
		/* FIXME: this will build the level if it isn't there. But,
		 * the level can also be there, possibly incomplete. This
		 * code isn't handling those incomplete levels yet (that
		 * needs some current level/gconf directory comparing code)
		 */
		if (!gtk_tree_model_iter_children (GTK_TREE_MODEL (tree_model), &child_iter, &iter)) {
			g_strfreev (key_array);
			return NULL;
		}

		found = FALSE;
		do {
			gtk_tree_model_get (GTK_TREE_MODEL (tree_model), &child_iter,
					    GCONF_TREE_MODEL_NAME_COLUMN, &tmp_str,
					    -1);

			if (strcmp (tmp_str, key_array[i]) == 0) {
				found = TRUE;
				break;
			}
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (tree_model), &child_iter));

		if (!found) {
			g_strfreev (key_array);
			return NULL;
		}
		iter = child_iter;
	}

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (tree_model), &child_iter);

	return path;
}


gchar *
gconf_tree_model_get_gconf_path (GConfTreeModel *tree_model, GtkTreeIter *iter)
{
	Node *node = iter->user_data;

	return g_strdup (node->path);
}

gchar *
gconf_tree_model_get_gconf_name (GConfTreeModel *tree_model, GtkTreeIter *iter)
{
	gchar *ptr;
	Node *node = iter->user_data;

	ptr = node->path + strlen (node->path);

	while (ptr[-1] != '/')
		ptr--;

	return g_strdup (ptr);
}

static gboolean
gconf_tree_model_build_level (GConfTreeModel *model, Node *parent_node, gboolean emit_signals)
{
	GSList *list, *tmp;
	Node *tmp_node = NULL;
	gint i = 0;

	if (parent_node->children)
		return FALSE;

	list = gconf_client_all_dirs (model->client, parent_node->path, NULL);

	if (!list)
		return FALSE;

	for (tmp = list; tmp; tmp = tmp->next, i++) {
		Node *node;

		node = g_new0 (Node, 1);
		node->offset = i;
		node->parent = parent_node;
		node->path = tmp->data;

		if (tmp_node) {
			tmp_node->next = node;
			node->prev = tmp_node;
		} else {
			/* set parent node's children */
			parent_node->children = node;
		}

		tmp_node = node;

		/* let the model know things have changed */
		if (emit_signals) {
			GtkTreeIter tmp_iter;
			GtkTreePath *tmp_path;

			model->stamp++;
			tmp_iter.stamp = model->stamp;
			tmp_iter.user_data = tmp_node;
			tmp_path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &tmp_iter);
			gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), tmp_path, &tmp_iter);
			gtk_tree_path_free (tmp_path);
		}
	}

	g_slist_free (list);

	return TRUE;
}

static gint
gconf_tree_model_get_n_columns (GtkTreeModel *tree_model)
{
	return GCONF_TREE_MODEL_NUM_COLUMNS;
}

static GType
gconf_tree_model_get_column_type (GtkTreeModel *tree_model, gint index)
{
	switch (index) {
	case GCONF_TREE_MODEL_NAME_COLUMN:
		return G_TYPE_STRING;
	case GCONF_TREE_MODEL_CLOSED_ICON_COLUMN:
	case GCONF_TREE_MODEL_OPEN_ICON_COLUMN:
		return GDK_TYPE_PIXBUF;
	default:
		return G_TYPE_INVALID;
	}
}

static gboolean
gconf_tree_model_get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
	gint *indices;
	gint depth, i;
	GtkTreeIter parent;
	GConfTreeModel *model;

	model = (GConfTreeModel *)tree_model;

	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);

	parent.stamp = model->stamp;
	parent.user_data = NULL;

	if (!gtk_tree_model_iter_nth_child (tree_model, iter, NULL, indices[0]))
		return FALSE;

	for (i = 1; i < depth; i++) {
		parent = *iter;

		if (!gtk_tree_model_iter_nth_child (tree_model, iter, &parent, indices[i]))
			return FALSE;
	}

	return TRUE;
}

static GtkTreePath *
gconf_tree_model_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	GConfTreeModel *model = (GConfTreeModel *)tree_model;
	Node *tmp_node;
	GtkTreePath *tree_path;
	gint i = 0;

	tmp_node = iter->user_data;

	if (NODE (iter->user_data)->parent == NULL) {
		tree_path = gtk_tree_path_new ();
		tmp_node = model->root;
	}
	else {
		GtkTreeIter tmp_iter = *iter;

		tmp_iter.user_data = NODE (iter->user_data)->parent;

		tree_path = gconf_tree_model_get_path (tree_model, &tmp_iter);

		tmp_node = NODE (iter->user_data)->parent->children;
	}

	for (; tmp_node; tmp_node = tmp_node->next) {
		if (tmp_node == NODE (iter->user_data))
			break;

		i++;
	}

	gtk_tree_path_append_index (tree_path, i);

	return tree_path;
}


static void
gconf_tree_model_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value)
{
	Node *node = iter->user_data;

	switch (column) {
	case GCONF_TREE_MODEL_NAME_COLUMN:
		g_value_init (value, G_TYPE_STRING);

		if (node->parent == NULL)
			g_value_set_string (value, "/");
		else {
			gchar *ptr;

			ptr = node->path + strlen (node->path);

			while (ptr[-1] != '/')
				ptr--;

			g_value_set_string (value, ptr);
		}
		break;
	case GCONF_TREE_MODEL_CLOSED_ICON_COLUMN:
		g_value_init (value, GDK_TYPE_PIXBUF);
		g_value_set_object (value, closed_icon);
		break;

	case GCONF_TREE_MODEL_OPEN_ICON_COLUMN:
		g_value_init (value, GDK_TYPE_PIXBUF);
		g_value_set_object (value, open_icon);
		break;
	default:
		break;
	}
}

static gboolean
gconf_tree_model_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n)
{
	GConfTreeModel *model;
	Node *node;
	Node *parent_node = NULL;

	model = (GConfTreeModel *)tree_model;

	g_return_val_if_fail (parent == NULL || parent->user_data != NULL, FALSE);
	g_return_val_if_fail (parent == NULL || parent->stamp == model->stamp, FALSE);

	if (parent == NULL)
		node = model->root;
	else {
		parent_node = (Node *)parent->user_data;
		node = parent_node->children;
	}

	if (!node && parent && gconf_tree_model_build_level (model, parent_node, FALSE)) {
		node = parent_node->children;
	}

	for (; node != NULL; node = node->next)
		if (node->offset == n) {
			iter->stamp = model->stamp;
			iter->user_data = node;

			return TRUE;
		}

	iter->stamp = 0;

	return FALSE;
}

static gboolean
gconf_tree_model_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	if (NODE (iter->user_data)->next != NULL) {
		iter->user_data = NODE (iter->user_data)->next;

		return TRUE;
	}
	else
		return FALSE;
}

static gboolean
gconf_tree_model_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
	GConfTreeModel *model = (GConfTreeModel *)tree_model;
	Node *parent_node = parent->user_data;

	if (parent_node->children != NULL) {
		iter->stamp = model->stamp;
		iter->user_data = parent_node->children;

		return TRUE;
	}

	if (!gconf_tree_model_build_level (model, parent_node, TRUE)) {
		iter->stamp = 0;
		iter->user_data = NULL;

		return FALSE;
	}

	iter->stamp = model->stamp;
	iter->user_data = parent_node->children;

	return TRUE;
}

static gint
gconf_tree_model_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	GConfTreeModel *model = (GConfTreeModel *)tree_model;
	Node *node;
	GtkTreeIter tmp;
	gint i = 0;

	g_return_val_if_fail (GCONF_IS_TREE_MODEL (tree_model), 0);
	if (iter) g_return_val_if_fail (model->stamp == iter->stamp, 0);

	if (iter == NULL)
		node = model->root;
	else
		node = ((Node *)iter->user_data)->children;

	if (!node && iter && gconf_tree_model_iter_children (tree_model, &tmp, iter)) {
		g_return_val_if_fail (tmp.stamp == model->stamp, 0);
		node = ((Node *)tmp.user_data);
	}

	if (!node)
		return 0;

	for (; node != NULL; node = node->next)
		i++;

	return i;
}

static gboolean
gconf_tree_model_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
	Node *child_node = child->user_data;

	if (child_node->parent == NULL)
		return FALSE;

	iter->stamp = ((GConfTreeModel *)tree_model)->stamp;
	iter->user_data = child_node->parent;

	return TRUE;
}

static gboolean
gconf_tree_model_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	GConfTreeModel *model = (GConfTreeModel *)tree_model;
	Node *node = iter->user_data;
	GSList *list;

	list = gconf_client_all_dirs (model->client, node->path, NULL);

	if (list == NULL)
		return FALSE;
	else
		return TRUE;

}

static void
gconf_tree_model_ref_node (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	Node *node = iter->user_data;

	node->ref_count += 1;
}

static void
gconf_tree_model_unref_node (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
	Node *node = iter->user_data;
	node->ref_count -= 1;

	if (node->ref_count == 0) {

		if ((node->parent != NULL) && (node->parent->children == node))
			node->parent->children = node->next;
		if (node->next != NULL)
			node->next->prev = node->prev;
		if (node->prev != NULL)
			node->prev->next = node->next;

		g_free (node->path);
		g_free (node);
	}
}

static void
gconf_tree_model_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_n_columns = gconf_tree_model_get_n_columns;
	iface->get_column_type = gconf_tree_model_get_column_type;
	iface->get_iter = gconf_tree_model_get_iter;
	iface->get_path = gconf_tree_model_get_path;
	iface->get_value = gconf_tree_model_get_value;
	iface->iter_nth_child = gconf_tree_model_iter_nth_child;
	iface->iter_next = gconf_tree_model_iter_next;
	iface->iter_has_child = gconf_tree_model_iter_has_child;
	iface->iter_n_children = gconf_tree_model_iter_n_children;
	iface->iter_children = gconf_tree_model_iter_children;
	iface->iter_parent = gconf_tree_model_iter_parent;
	iface->ref_node = gconf_tree_model_ref_node;
	iface->unref_node = gconf_tree_model_unref_node;
}

static void
gconf_tree_model_class_init (GConfTreeModelClass *klass)
{
	closed_icon = gdk_pixbuf_new_from_file (GCONF_EDITOR_IMAGEDIR"/folder-closed.png", NULL);
	open_icon = gdk_pixbuf_new_from_file (GCONF_EDITOR_IMAGEDIR"/folder-open.png", NULL);
}

static void
gconf_tree_model_init (GConfTreeModel *model)
{
	Node *root;

	root = g_new0 (Node, 1);
	root->path = g_strdup ("/");
	root->offset = 0;

	model->root = root;
	model->client = gconf_client_get_default ();
}

GType
gconf_tree_model_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (GConfTreeModelClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gconf_tree_model_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (GConfTreeModel),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gconf_tree_model_init
		};

		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) gconf_tree_model_tree_model_init,
			NULL,
			NULL
		};

		object_type = g_type_register_static (G_TYPE_OBJECT, "GConfTreeModel", &object_info, 0);

		g_type_add_interface_static (object_type,
					     GTK_TYPE_TREE_MODEL,
					     &tree_model_info);
	}

	return object_type;
}

GtkTreeModel *
gconf_tree_model_new (void)
{
	GConfTreeModel *model;

	model = g_object_new (GCONF_TYPE_TREE_MODEL, NULL);

	return GTK_TREE_MODEL (model);
}
