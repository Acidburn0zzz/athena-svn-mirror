/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#include <config.h>

#include <gnome.h>

#include "gtkgs.h"
#include "gsdefaults.h"
#include "ggvutils.h"

#include <gconf/gconf-client.h>

/**
 * defaults for GtkGS widgets
 **/

static GConfClient *gconf_client;

/* Default values to pass to gtk_gs_init */
typedef struct _GtkGSDefaults 
{
        gboolean   antialiased;
        gboolean   override_size;
        gint       default_size;
	gboolean   watch_doc;
        gboolean   override_orientation;
        gboolean   respect_eof;
        gboolean   show_scroll_rect;
        gint       fallback_orientation;
        gfloat     zoom_factor;
        gfloat     scroll_step;
        gchar      *interpreter_cmd;
        gchar      *alpha_params;
        gchar      *convert_pdf_cmd;
        gchar      *ungzip_cmd;
        gchar      *unbzip2_cmd;
        GtkGSZoomMode zoom_mode;
} GtkGSDefaults;

static GtkGSDefaults gtk_gs_defaults = {
        TRUE, FALSE, 8, TRUE, FALSE, TRUE, TRUE,
        0, 1.0, 0.25, NULL, NULL, NULL, NULL, NULL,
        GTK_GS_ZOOM_ABSOLUTE
};

void 
gtk_gs_defaults_set_scroll_step(gfloat step)
{
        gtk_gs_defaults.scroll_step = step;
}

gfloat
gtk_gs_defaults_get_scroll_step()
{
        return gtk_gs_defaults.scroll_step;
}

void
gtk_gs_defaults_set_size(gint iNewPageSize)
{
        gtk_gs_defaults.default_size = iNewPageSize;
}

gint
gtk_gs_defaults_get_size()
{
        return gtk_gs_defaults.default_size;
}

void
gtk_gs_defaults_set_watch_doc(gint iNewWatchDoc)
{
        gtk_gs_defaults.watch_doc = iNewWatchDoc;
}

gint
gtk_gs_defaults_get_watch_doc()
{
        return gtk_gs_defaults.watch_doc;
}

void
gtk_gs_defaults_set_override_size(gboolean bOverrSize)
{
        gtk_gs_defaults.override_size = bOverrSize;
}

gboolean
gtk_gs_defaults_get_override_size()
{
        return gtk_gs_defaults.override_size;
}

void
gtk_gs_defaults_set_override_orientation(gboolean bOverOrien)
{
        gtk_gs_defaults.override_orientation = bOverOrien;
}

gboolean
gtk_gs_defaults_get_override_orientation()
{
        return gtk_gs_defaults.override_orientation;
}

void
gtk_gs_defaults_set_antialiased(gint iNewAntialiased)
{
        gtk_gs_defaults.antialiased = iNewAntialiased;
}

gboolean
gtk_gs_defaults_get_antialiased()
{
        return gtk_gs_defaults.antialiased;
}

void
gtk_gs_defaults_set_zoom_factor(gfloat fZoom)
{
        fZoom = MIN(fZoom, 30.0);
        fZoom = MAX(0.05, fZoom);
        gtk_gs_defaults.zoom_factor = fZoom;
}

gfloat
gtk_gs_defaults_get_zoom_factor()
{
        return gtk_gs_defaults.zoom_factor;
}

void
gtk_gs_defaults_set_orientation(gint iNewOrientation) {
        g_assert (
                  (iNewOrientation == GTK_GS_ORIENTATION_PORTRAIT) ||
                  (iNewOrientation == GTK_GS_ORIENTATION_LANDSCAPE) ||
                  (iNewOrientation == GTK_GS_ORIENTATION_UPSIDEDOWN) ||
                  (iNewOrientation == GTK_GS_ORIENTATION_SEASCAPE)
                  );
        gtk_gs_defaults.fallback_orientation = iNewOrientation;
}

gint
gtk_gs_defaults_get_orientation()
{
        return gtk_gs_defaults.fallback_orientation;
}

void
gtk_gs_defaults_set_respect_eof(gboolean resp)
{
        gtk_gs_defaults.respect_eof = resp;
}

gboolean
gtk_gs_defaults_get_respect_eof()
{
        return gtk_gs_defaults.respect_eof;
}

GtkGSPaperSize *
gtk_gs_defaults_get_paper_sizes()
{
        return ggv_paper_sizes;
}

gint
gtk_gs_defaults_get_paper_count()
{
        gint n = 0;

        while(ggv_paper_sizes[n].name != NULL)
                n++;

        return n;
}

gboolean
gtk_gs_defaults_get_show_scroll_rect()
{
        return gtk_gs_defaults.show_scroll_rect;
}

void
gtk_gs_defaults_set_show_scroll_rect(gboolean f)
{
        gtk_gs_defaults.show_scroll_rect = f;
}

const gchar *
gtk_gs_defaults_get_interpreter_cmd()
{
        if(!gtk_gs_defaults.interpreter_cmd)
                return GS_PATH;
        return gtk_gs_defaults.interpreter_cmd;
}

const gchar *
gtk_gs_defaults_get_alpha_parameters()
{
        if(!gtk_gs_defaults.alpha_params)
                return ALPHA_PARAMS;
        return gtk_gs_defaults.alpha_params;
}

const gchar *
gtk_gs_defaults_get_convert_pdf_cmd()
{
        if(!gtk_gs_defaults.convert_pdf_cmd)
                return GS_PATH
                        " -q -dNOPAUSE -dBATCH -dSAFER"
                        " -dQUIET -sDEVICE=pswrite"
                        " -sOutputFile=%s -c save pop -f %s";
        return gtk_gs_defaults.convert_pdf_cmd;
}

const gchar *
gtk_gs_defaults_get_ungzip_cmd()
{
        if(!gtk_gs_defaults.ungzip_cmd)
                return "gzip -cd";
        return gtk_gs_defaults.ungzip_cmd;
}

const gchar *
gtk_gs_defaults_get_unbzip2_cmd()
{
        if(!gtk_gs_defaults.unbzip2_cmd)
                return "bzip2 -cd";
        return gtk_gs_defaults.unbzip2_cmd;
}

void
gtk_gs_defaults_set_interpreter_cmd(gchar *cmd)
{
        if(gtk_gs_defaults.interpreter_cmd)
                g_free(gtk_gs_defaults.interpreter_cmd);
        gtk_gs_defaults.interpreter_cmd = cmd;
}

void
gtk_gs_defaults_set_alpha_parameters(gchar *par)
{
        if(gtk_gs_defaults.alpha_params)
                g_free(gtk_gs_defaults.alpha_params);
        gtk_gs_defaults.alpha_params = par;
}

void
gtk_gs_defaults_set_convert_pdf_cmd(gchar *cmd)
{
        if(gtk_gs_defaults.convert_pdf_cmd)
                g_free(gtk_gs_defaults.convert_pdf_cmd);
        gtk_gs_defaults.convert_pdf_cmd = cmd;
}

void
gtk_gs_defaults_set_ungzip_cmd(gchar *cmd)
{
        if(gtk_gs_defaults.ungzip_cmd)
                g_free(gtk_gs_defaults.ungzip_cmd);
        
        gtk_gs_defaults.ungzip_cmd = cmd;
}

void
gtk_gs_defaults_set_unbzip2_cmd(gchar *cmd)
{
        if(gtk_gs_defaults.unbzip2_cmd)
                g_free(gtk_gs_defaults.unbzip2_cmd);
        gtk_gs_defaults.unbzip2_cmd = cmd;
}

GtkGSZoomMode
gtk_gs_defaults_get_zoom_mode()
{
        return gtk_gs_defaults.zoom_mode;
}

void
gtk_gs_defaults_set_zoom_mode(GtkGSZoomMode zoom_mode)
{
        gtk_gs_defaults.zoom_mode = zoom_mode;
}

void
gtk_gs_defaults_load()
{
        gtk_gs_defaults_gconf_client();

        gtk_gs_defaults_set_respect_eof
                (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/respect_eof",
                                       NULL));
        gtk_gs_defaults_set_watch_doc
                (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/watch_doc",
                                       NULL));
        gtk_gs_defaults_set_override_size
                (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/override_size",
                                       NULL));
        gtk_gs_defaults_set_override_orientation
                (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/override_orientation",
                                       NULL));
        gtk_gs_defaults_set_antialiased
                (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/antialiasing",
                                       NULL));
        gtk_gs_defaults_set_size
                (gconf_client_get_int(gconf_client, "/apps/ggv/gtkgs/size",
                                      NULL));
        gtk_gs_defaults_set_zoom_factor
                (gconf_client_get_float(gconf_client, "/apps/ggv/gtkgs/zoom",
                                        NULL));
        gtk_gs_defaults_set_orientation
                (gconf_client_get_int(gconf_client, "/apps/ggv/gtkgs/orientation",
                                      NULL));
        gtk_gs_defaults_set_interpreter_cmd
                (gconf_client_get_string(gconf_client, "/apps/ggv/gtkgs/interpreter",
                                         NULL));
        gtk_gs_defaults_set_alpha_parameters
                (gconf_client_get_string(gconf_client, "/apps/ggv/gtkgs/alphaparams",
                                         NULL));
        gtk_gs_defaults_set_convert_pdf_cmd
                (gconf_client_get_string(gconf_client, "/apps/ggv/gtkgs/convertpdf",
                                         NULL));
        gtk_gs_defaults_set_ungzip_cmd
                (gconf_client_get_string(gconf_client, "/apps/ggv/gtkgs/ungzip",
                                         NULL));
        gtk_gs_defaults_set_unbzip2_cmd
                (gconf_client_get_string(gconf_client, "/apps/ggv/gtkgs/unbzip2",
                                         NULL));
        gtk_gs_defaults_set_show_scroll_rect
                (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/show_scroll_rect",
                                       NULL));
        gtk_gs_defaults_set_scroll_step
                (gconf_client_get_float(gconf_client, "/apps/ggv/gtkgs/scrollstep",
                                       NULL));
        gtk_gs_defaults_set_zoom_mode
                (gconf_client_get_int(gconf_client, "/apps/ggv/gtkgs/zoommode",
                                       NULL));
}

void
gtk_gs_defaults_save()
{
        gtk_gs_defaults_gconf_client();

        gconf_client_set_bool(gconf_client, "/apps/ggv/gtkgs/respect_eof",
                              gtk_gs_defaults_get_respect_eof(), NULL);
        gconf_client_set_bool(gconf_client, "/apps/ggv/gtkgs/watch_doc",
                              gtk_gs_defaults_get_watch_doc(), NULL);
        gconf_client_set_bool(gconf_client, "/apps/ggv/gtkgs/override_size",
                              gtk_gs_defaults_get_override_size(), NULL);
        gconf_client_set_bool(gconf_client, "/apps/ggv/gtkgs/override_orientation",
                              gtk_gs_defaults_get_override_orientation(), NULL);
        gconf_client_set_bool(gconf_client, "/apps/ggv/gtkgs/antialiasing",
                              gtk_gs_defaults_get_antialiased(), NULL);
        gconf_client_set_int(gconf_client, "/apps/ggv/gtkgs/size",
                             gtk_gs_defaults_get_size(), NULL);
        gconf_client_set_int(gconf_client, "/apps/ggv/gtkgs/orientation",
                             gtk_gs_defaults_get_orientation(), NULL);
        gconf_client_set_float(gconf_client, "/apps/ggv/gtkgs/zoom",
                               gtk_gs_defaults_get_zoom_factor(), NULL);
        gconf_client_set_string(gconf_client, "/apps/ggv/gtkgs/interpreter",
                                gtk_gs_defaults_get_interpreter_cmd(), NULL);
        gconf_client_set_string(gconf_client, "/apps/ggv/gtkgs/alphaparams",
                                gtk_gs_defaults_get_alpha_parameters(), NULL);
        gconf_client_set_string(gconf_client, "/apps/ggv/gtkgs/convertpdf",
                                gtk_gs_defaults_get_convert_pdf_cmd(), NULL);
        gconf_client_set_string(gconf_client, "/apps/ggv/gtkgs/ungzip",
                                gtk_gs_defaults_get_ungzip_cmd(), NULL);
        gconf_client_set_string(gconf_client, "/apps/ggv/gtkgs/unbzip2",
                                gtk_gs_defaults_get_unbzip2_cmd(), NULL);
        gconf_client_set_bool(gconf_client, "/apps/ggv/gtkgs/show_scroll_rect",
                              gtk_gs_defaults_get_show_scroll_rect(), NULL);
        gconf_client_set_float(gconf_client, "/apps/ggv/gtkgs/scrollstep",
                              gtk_gs_defaults_get_scroll_step(), NULL);
        gconf_client_set_int(gconf_client, "/apps/ggv/gtkgs/zoommode",
                             gtk_gs_defaults_get_zoom_mode(), NULL);
}

static void
gtk_gs_defaults_changed(GConfClient* client, guint cnxn_id,
                        GConfEntry *entry, gpointer user_data)
{
        if(!strcmp(entry->key, "/apps/ggv/gtkgs/respect_eof"))
                gtk_gs_defaults_set_respect_eof
                        (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/respect_eof",
                                               NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/override_orientation"))
                gtk_gs_defaults_set_override_orientation
                        (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/override_orientation",
                                               NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/orientation"))
                gtk_gs_defaults_set_orientation
                        (gconf_client_get_int(gconf_client, "/apps/ggv/gtkgs/orientation",
                                              NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/zoom"))
                gtk_gs_defaults_set_zoom_factor
                        (gconf_client_get_float(gconf_client, "/apps/ggv/gtkgs/zoom",
                                                NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/size"))
                gtk_gs_defaults_set_size
                        (gconf_client_get_int(gconf_client, "/apps/ggv/gtkgs/size",
                                              NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/antialiasing"))
                gtk_gs_defaults_set_antialiased
                        (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/antialiasing",
                                               NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/override_size"))
                gtk_gs_defaults_set_override_size
                        (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/override_size",
                                               NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/watch_doc"))
                gtk_gs_defaults_set_watch_doc
                        (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/watch_doc",
                                               NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/show_scroll_rect"))
                gtk_gs_defaults_set_show_scroll_rect
                        (gconf_client_get_bool(gconf_client, "/apps/ggv/gtkgs/show_scroll_rect",
                                               NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/interpreter"))
                gtk_gs_defaults_set_interpreter_cmd
                        (gconf_client_get_string(gconf_client, "/apps/ggv/gtkgs/interpreter",
                                                 NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/alphaparams"))
                gtk_gs_defaults_set_alpha_parameters
                        (gconf_client_get_string(gconf_client, "/apps/ggv/gtkgs/alphaparams",
                                                 NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/convertpdf"))
                gtk_gs_defaults_set_convert_pdf_cmd
                        (gconf_client_get_string(gconf_client, "/apps/ggv/gtkgs/convertpdf",
                                                 NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/ungzip"))
                gtk_gs_defaults_set_ungzip_cmd
                        (gconf_client_get_string(gconf_client, "/apps/ggv/gtkgs/ungzip",
                                                 NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/unbzip2"))
                gtk_gs_defaults_set_unbzip2_cmd
                        (gconf_client_get_string(gconf_client, "/apps/ggv/gtkgs/unbzip2",
                                                 NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/zoommode"))
                gtk_gs_defaults_set_zoom_mode
                        (gconf_client_get_int(gconf_client, "/apps/ggv/gtkgs/zoommode",
                                                 NULL));
        else if(!strcmp(entry->key, "/apps/ggv/gtkgs/scrollstep"))
                gtk_gs_defaults_set_scroll_step
                        (gconf_client_get_int(gconf_client, "/apps/ggv/gtkgs/scrollstep",
                                                 NULL));
}

GConfClient *
gtk_gs_defaults_gconf_client()
{
        if(!gconf_client) {
                g_assert(gconf_is_initialized());
                gconf_client = gconf_client_get_default();
                g_assert(gconf_client != NULL);
                gconf_client_add_dir(gconf_client, "/apps/ggv/gtkgs",
                                     GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
                gconf_client_notify_add(gconf_client,
                                        "/apps/ggv/gtkgs",
                                        (GConfClientNotifyFunc)gtk_gs_defaults_changed,
                                        NULL, NULL, NULL);
        }

        return gconf_client;
}
