/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#include "config.h"
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>

#include <libgnomevfs/gnome-vfs-utils.h>

#include "ggvutils.h"

GtkGSPaperSize ggv_paper_sizes[] = {
        {"BBox",         0,  0},
        {"Letter",       612,  792,},
        {"Tabloid",      792, 1224,},
        {"Ledger",       1224,  792,},
        {"Legal",        612, 1008,},
        {"Statement",	 396,  612,},
        {"Executive",	 540,  720,},
        {"A0",           2380, 3368,},
        {"A1",           1684, 2380,},
        {"A2",           1190, 1684,},
        {"A3",		 842, 1190,},
        {"A4",		 595,  842,},
        {"A5",		 420,  595,},
        {"B4",		 729, 1032,},
        {"B5",		 516,  729,},
        {"Folio",        612,  936,},
        {"Quarto",       610,  780,},
        {"10x14",        720, 1008,},
        {NULL,		   0,    0}
};

const gfloat ggv_unit_factors[] = {
        1.0,
        25.4,
        2.54,
        72.0
};

const gchar *ggv_orientation_labels[] = {
        N_("Portrait"),
        N_("Landscape"),
        N_("Upside Down"),
        N_("Seascape"),
        NULL,
};

const gint ggv_max_orientation_labels =
    (sizeof(ggv_orientation_labels) / sizeof(gchar *)) - 2;

const gchar *ggv_unit_labels[] = {
        N_("inch"),
        N_("mm"),
        N_("cm"),
        N_("point"),
        NULL
};

const gint ggv_max_unit_labels =
    (sizeof(ggv_unit_labels) / sizeof(gchar *)) - 2;

gfloat ggv_zoom_levels[] = {
	1.0 / 6.0, 1.0 / 5.0, 1.0 / 4.0, 1.0 / 3.0, 1.0 / 2.0, 3.0 / 4.0, 1.0,
        3.0 / 2.0, 2.0,	3.0, 4.0, 5.0, 6.0
};

const gchar *ggv_zoom_level_names[] = {
	"1:6", "1:5", "1:4", "1:3",
	"1:2", "3:4", "1:1", "3:2",
        "2:1", "3:1", "4:1", "5:1",
        "6:1",
};

const gint ggv_max_zoom_levels =
    (sizeof (ggv_zoom_levels) / sizeof (gfloat)) - 1;

const gchar *ggv_auto_fit_modes[] = {
        N_("None"), N_("Fit to page width"), N_("Fit to page size")
};

const gint ggv_max_auto_fit_modes =
    (sizeof (ggv_auto_fit_modes) / sizeof (gchar *)) - 1;

gint
ggv_zoom_index_from_float (gfloat zoom_level)
{
	int i;

	for (i = 0; i <= ggv_max_zoom_levels; i++) {
		float this, epsilon;

		/* if we're close to a zoom level */
		this = ggv_zoom_levels [i];
		epsilon = this * 0.01;

		if (zoom_level < this+epsilon)
			return i;
	}

	return ggv_max_zoom_levels;
}

gfloat
ggv_zoom_level_from_index (gint index)
{
	if (index > ggv_max_zoom_levels)
		index = ggv_max_zoom_levels;

	return ggv_zoom_levels [index];
}

GSList *
ggv_split_string (const gchar *string, const gchar *delimiter)
{
	const gchar *ptr = string;
	int pos = 0, escape = 0;
	char buffer [BUFSIZ];
	GSList *list = NULL;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (delimiter != NULL, NULL);

	while (*ptr) {
		char c = *ptr++;
		const gchar *d;
		int found = 0;

		if (pos >= BUFSIZ) {
			g_warning ("string too long, aborting");
			return list;
		}

		if (escape) {
			buffer [pos++] = c;
			escape = 0;
			continue;
		}

		if (c == '\\') {
			escape = 1;
			continue;
		}

		for (d = delimiter; *d; d++) {
			if (c == *d) {
				buffer [pos++] = 0;
				list = g_slist_prepend (list, g_strdup (buffer));
				pos = 0; found = 1;
				break;
			}
		}

		if (!found)
			buffer [pos++] = c;
	}

	buffer [pos++] = 0;
	list = g_slist_prepend (list, g_strdup (buffer));

	return list;
}

gint
ggv_get_index_of_string(gchar *string, gchar **strings)
{
        guint idx = 0;

        while(strings[idx] != NULL) {
                if(strcmp(strings[idx], string) == 0)
                        return idx;
                idx++;
        }

        return -1;
}

/* Quote filename for system call */
gchar *
ggv_quote_filename (const gchar *str)
{
        return g_shell_quote(str);
}

/* escapes filename to form a proper URI: works conservatively - anything
   except [a-zA-Z0-9_] will be escaped with a %XX escape sequence where
   XX is the hex value of the char. */
gchar *
ggv_filename_to_uri(const gchar *fname)
{
        gchar *full_path, *ret_val;
        
        if(*fname != '/') {
                gchar *cwd;
                /* relative file name - we will have to absolutize it */
                cwd = g_get_current_dir();
                full_path = g_strconcat(cwd, "/", fname, NULL);
        }
        else full_path = NULL;
        ret_val = gnome_vfs_get_uri_from_local_path(full_path?full_path:fname);
        if(full_path)
                g_free(full_path);
        return ret_val;
}

/* If file exists and is a regular file then return its length, else -1 */
gint
ggv_file_length (const gchar *filename)
{
        struct stat stat_rec;

	if (filename && (stat (filename, &stat_rec) == 0)
	    && S_ISREG (stat_rec.st_mode))
		return stat_rec.st_size;
	else
                return -1;
}

/* Test if file exists, is a regular file and its length is > 0 */
gboolean
ggv_file_readable (const char *filename)
{
        return (ggv_file_length (filename) > 0);
}

/* Set a tooltip for a widget */
void
ggv_set_tooltip(GtkWidget* w, const gchar* tip)
{
	GtkTooltips* t = gtk_tooltips_new();

	gtk_tooltips_set_tip(t, w, tip, NULL);
}

gfloat
ggv_compute_zoom(gint zoom_spec)
{
	return pow(1.2, zoom_spec); /* The Knuth magstep formula rules */
}

gint
ggv_compute_spec(gfloat zoom)
{
        zoom = MAX(0.02, zoom);
        zoom = MIN(10.0, zoom);

        zoom = log(zoom)/log(1.2);
        return (gint)rint(zoom);
}

void
ggv_raise_and_focus_widget (GtkWidget *widget)
{
	g_assert (GTK_WIDGET_REALIZED (widget));
	gdk_window_raise (widget->window);
	gtk_widget_grab_focus (widget);
}

void
ggv_get_window_size(GtkWidget *widget, gint *width, gint *height)
{
        *width = widget->allocation.width;
        *height = widget->allocation.height;
}

