/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * Copyright (C) 2000 Eazel, Inc
 * Copyright (C) 2000 Helix Code, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: J Shane Culpepper <pepper@eazel.com>
 *          Joe Shaw <joe@helixcode.com>
 *          Eskil Heyn Olsen <eskil@eazel.com>
 */

/* eazel-install - services command line install/update/uninstall
 * component.  This program will parse the eazel-services-config.xml
 * file and install a services generated package-list.xml.
 */

#include <config.h>
#include "eazel-softcat.h"	/* for softcat sense flags */
#include "eazel-install-xml-package-list.h"

#include <libtrilobite/trilobite-core-utils.h>
#include <libtrilobite/trilobite-core-network.h>
#include <gnome-xml/parser.h>
#include <gnome-xml/xmlmemory.h>

static PackageData* parse_package (xmlNode* package, gboolean set_toplevel);
static CategoryData* parse_category (xmlNode* cat);
static int parse_pkg_template (const char* pkg_template_file, char** result);
static PackageData* osd_parse_softpkg (xmlNodePtr softpkg);



static PackageData*
parse_package (xmlNode* package, gboolean set_toplevel) {

	xmlNodePtr dep;
	PackageData* rv;
	char *tmp;

	rv = packagedata_new ();

	rv->name = trilobite_xml_get_string (package, "NAME");
	rv->version = trilobite_xml_get_string (package, "VERSION");
	rv->minor = trilobite_xml_get_string (package, "MINOR");
	rv->archtype = trilobite_xml_get_string (package, "ARCH");
	
	tmp = trilobite_xml_get_string (package, "STATUS");
	if (tmp) {
		rv->status = packagedata_status_str_to_enum (tmp);
		g_free (tmp);
	}

	tmp = trilobite_xml_get_string (package, "MODSTATUS");
	if (tmp) {
		rv->modify_status = packagedata_modstatus_str_to_enum (tmp);
		g_free (tmp);
	}

	tmp = trilobite_xml_get_string (package, "BYTESIZE");
	if (tmp) {
		rv->bytesize = atoi (tmp);
		g_free (tmp);
	} else {
		rv->bytesize = 0;
	}

	tmp = trilobite_xml_get_string (package, "PROVIDES");
	if (tmp) {
		char **files;
		int iterator;

		files = g_strsplit (tmp, G_SEARCHPATH_SEPARATOR_S, 0);
		if (files) {
			/* we don't free the individual files[x]'s, as
			   they end up in the package data object */
			for (iterator = 0; files[iterator]; iterator++) {
				rv->provides = g_list_prepend (rv->provides,
							       files[iterator]);
			}
			rv->provides = g_list_reverse (rv->provides);
			g_free (files);
		}
		g_free (tmp);
	}

	rv->summary = trilobite_xml_get_string (package, "SUMMARY");
	rv->description = trilobite_xml_get_string (package, "DESCRIPTION");
	rv->distribution = trilobite_get_distribution ();
	if (set_toplevel) {
		rv->toplevel = TRUE;
	}
	
	/* Dependency Lists */
	rv->soft_depends = NULL;
	rv->hard_depends = NULL;
	rv->breaks = NULL;
	rv->modifies = NULL;

	dep = package->xmlChildrenNode;
	while (dep) {
		if (g_strcasecmp (dep->name, "SOFT_DEPEND") == 0) {
			PackageData* depend;

			depend = parse_package (dep, FALSE);
			packagedata_add_pack_to_soft_depends (rv, depend);
		} else if (g_strcasecmp (dep->name, "HARD_DEPEND") == 0) {
			PackageData* depend;

			depend = parse_package (dep, FALSE);
			packagedata_add_pack_to_hard_depends (rv, depend);
		} else if (g_strcasecmp (dep->name, "BREAKS") == 0) {
			PackageData* depend;

			depend = parse_package (dep, FALSE);
			packagedata_add_pack_to_breaks (rv, depend);
		} else if (g_strcasecmp (dep->name, "MODIFIES") == 0) {
			PackageData* depend;

			depend = parse_package (dep, FALSE);
			packagedata_add_pack_to_modifies (rv, depend);
		} 

		dep = dep->next;

	}

	return rv;

} /* end parse package */

static CategoryData*
parse_category (xmlNode* cat) {

	CategoryData* category;
	PackageData* pakdat;
	xmlNode* pkg;
	xmlNode* pkg2;
	char *text, *textp;

	category = categorydata_new ();
	category->name = trilobite_xml_get_string (cat, "name");

	pkg = cat->xmlChildrenNode;
	if (pkg == NULL) {
		g_print (_("*** No package nodes! (cat has no children) ***"));
		g_free (category);
		g_error (_("*** Bailing from package parse! ***"));
	}
	while (pkg) {
		pkg2 = pkg->xmlChildrenNode;
		if (g_strcasecmp (pkg->name, "PACKAGES") == 0) {
			if (pkg2 == NULL) {
				g_print (_("*** No package nodes! ***"));
				g_free (category);
				g_error (_("*** Bailing from package parse! ***"));
			}
			while (pkg2) {
				if (g_strcasecmp (pkg2->name, "PACKAGE") != 0) {
					g_error (_("*** Malformed package node!"));
				} else {
					pakdat = parse_package (pkg2, TRUE);
					category->packages = g_list_append (category->packages, pakdat);
				}
				pkg2 = pkg2->next;
			}
		} else if (g_strcasecmp (pkg->name, "DEPENDS") == 0) {
			if (pkg2 == NULL) {
				g_print (_("*** No depends nodes! ***"));
				g_free (category);
				g_error (_("*** Bailing from package parse! ***"));
			}
			while (pkg2) {
				if (g_strcasecmp (pkg2->name, "ON") != 0) {
					g_error (_("*** Malformed depends node!"));
				} else {
					text = xmlNodeGetContent (pkg2);
					trilobite_debug ("%s deps on %s", category->name, text);
					category->depends = g_list_prepend (category->depends, g_strdup (text));
					free (text);
				}
				pkg2 = pkg2->next;
			}
		} else if (g_strcasecmp (pkg->name, "EXCLUSIVE") == 0) {
			category->exclusive = TRUE;
		} else if (g_strcasecmp (pkg->name, "DEFAULT") == 0) {
			category->default_choice = TRUE;
		} else if (g_strcasecmp (pkg->name, "DESCRIPTION") == 0) {
			text = xmlNodeGetContent (pkg);
			/* remove leading and trailing linefeeds (xml artifact) */
			while ((strlen (text) > 0) && (text[strlen (text) - 1] == '\n')) {
					text[strlen (text) - 1] = '\0';
			}
			for (textp = text; (*textp == '\n'); textp++)
				;
			category->description = g_strdup (textp);
			free (text);
		} else {
			g_error (_("*** Unknown node type '%s'"), pkg->name);
		}
		pkg = pkg->next;
	}

	return category;

} /* end parse_category */

/* parse the contents of the CATEGORIES node */
static GList* parse_shared (xmlNodePtr base) 
{
	xmlNodePtr category;
	GList* rv;

	rv = NULL;
	if (base == NULL) {
		g_warning (_("*** The pkg list file contains no data! ***\n"));
		return NULL;
	}
	
	if (g_strcasecmp (base->name, "CATEGORIES") != 0) {
		g_print (_("*** Cannot find the CATEGORIES xmlnode! ***\n"));
		g_warning (_("*** Bailing from categories parse! ***\n"));
		return NULL;
	}
	
	category = base->xmlChildrenNode;
	if (category == NULL) {
		g_print (_("*** No Categories! ***\n"));
		g_warning (_("*** Bailing from category parse! ***\n"));
		return NULL;
	}
	
	while (category) {
		CategoryData* catdat;

		catdat = parse_category (category);
		rv = g_list_append (rv, catdat);
		category = category->next;
	}

	return rv;
}

GList*
parse_memory_xml_package_list (const char *mem, int size)
{
	xmlDocPtr doc;
	GList *list;

	g_return_val_if_fail (mem!=NULL, NULL);

	doc = xmlParseMemory ((char*)mem, size);
	if (doc == NULL) {
		xmlFreeDoc (doc);
		return NULL;
	}

	list = parse_shared (doc->root);
	xmlFreeDoc (doc);
	return list;
}

GList*
parse_memory_transaction_file (const char *mem, 
			       int size)
{
	xmlDocPtr doc;
	xmlNodePtr base;
	xmlNodePtr packages;
	GList* rv;

	g_return_val_if_fail (mem!=NULL, NULL);

	doc = xmlParseMemory ((char*)mem, size);
	rv = NULL;
	if (doc == NULL) {
		xmlFreeDoc (doc);
		return NULL;
	}

	base = doc->root;
	if (g_strcasecmp (base->name, "TRANSACTION")) {
		g_print (_("*** Cannot find the TRANSACTION xmlnode! ***\n"));
		xmlFreeDoc (doc);
		g_warning (_("*** Bailing from transaction parse! ***\n"));
		return NULL;
	}
	
	packages = doc->root->xmlChildrenNode->xmlChildrenNode;
	if(packages == NULL) {
		g_print (_("*** No packages! ***\n"));
		xmlFreeDoc (doc);
		g_warning (_("*** Bailing from transaction parse! ***\n"));
		return NULL;
	}

	while (packages) {
		PackageData *pack;
		pack = parse_package (packages, TRUE);
		rv = g_list_append (rv, pack);
		packages = packages->next;
	}
	return rv;
}

GList*
parse_local_xml_package_list (const char* pkg_list_file, char **splash_text, char **finish_text)
{
	xmlDocPtr doc;
	xmlNodePtr base, node;
	GList *list;
	CategoryData *catdat;
	char *text, *textp;

	g_return_val_if_fail (pkg_list_file != NULL, NULL);

	doc = xmlParseFile (pkg_list_file);
	list = NULL;

	if (doc == NULL) {
		fprintf (stderr, "*** Unable to open pkg list file %s\n", pkg_list_file);
		goto out;
	}

	base = doc->root;
	if (base == NULL) {
		g_print (_("*** No category nodes! ***"));
		goto out;
	}
	if (g_strcasecmp (base->name, "CATEGORIES") != 0) {
		g_print (_("*** Cannot find the CATEGORIES xmlnode! ***"));
		goto out;
	}
	node = base->xmlChildrenNode;
	list = NULL;
	while (node) {
		if (g_strcasecmp (node->name, "CATEGORY") == 0) {
			catdat = parse_category (node);
			list = g_list_append (list, catdat);
		} else if (g_strcasecmp (node->name, "SPLASH-TEXT") == 0) {
			if (splash_text != NULL) {
				text = xmlNodeGetContent (node);
				/* remove leading and trailing linefeeds (xml artifact) */
				while ((strlen (text) > 0) && (text[strlen (text) - 1] == '\n')) {
					text[strlen (text) - 1] = '\0';
				}
				for (textp = text; (*textp == '\n'); textp++)
					;
				*splash_text = g_strdup (textp);
				free (text);
			}
		} else if (g_strcasecmp (node->name, "FINISH-TEXT") == 0) {
			if (finish_text != NULL) {
				text = xmlNodeGetContent (node);
				/* remove leading and trailing linefeeds (xml artifact) */
				while ((strlen (text) > 0) && (text[strlen (text) - 1] == '\n')) {
					text[strlen (text) - 1] = '\0';
				}
				for (textp = text; (*textp == '\n'); textp++)
					;
				*finish_text = g_strdup (textp);
				free (text);
			}
		} else {
			g_print (_("*** Unknown node %s"), node->name);
		}
		node = node->next;
	}

out:
	xmlFreeDoc (doc);
	return list;
}

gboolean
generate_xml_package_list (const char* pkg_template_file,
                           const char* target_file) {

/* This function will accept a colon delimited list of packages and generate
 * an xml package list for eazel-install.  The pkg_template_function should
 * be of the following format:
 * 
 * category name : package name : version : minor : archtype : bytesize : summary
 * 
 * Example:
 * Essential Packages:anaconda:7.0:1:i386:2722261:The redhat installer
 */

	xmlDocPtr doc;
	char* retbuf;
	int index;
	char** entry_array;
	char** package_array;
	char *tags[] = {"NAME", "VERSION", "MINOR", "ARCH", "BYTESIZE", "SUMMARY", NULL};
	int num_tags = 5;
	int lines;
	char *cur_category = g_strdup ("");
	
	doc = xmlNewDoc ("1.0");
	doc->root = xmlNewDocNode (doc, NULL, "CATEGORIES", NULL);
	
	lines = parse_pkg_template (pkg_template_file, &retbuf);

	if (lines) {
		entry_array = g_strsplit (retbuf, "\n", lines);

		for (index = 0; index < lines; index++) {
			if (entry_array[index] == NULL) {
				break;
			}
			
			package_array = g_strsplit (entry_array[index], ":", num_tags+1);
			
			if (package_array && package_array[0]) {
				xmlNodePtr packages;
				xmlNodePtr category;
				xmlNodePtr package;
				xmlNodePtr data;
				int i;

				/* FIXME: I added this to get rid of
				 * an uninitialized variable warning,
				 * but I think it's probably
				 * wrong. (Darin)
				 */
				packages = NULL;

				/* NOTE: This xmlGetProp leaks, since its return value 
				   is forgotten */
				if ((doc->root->xmlChildrenNode == NULL) ||
				    (strlen (package_array[0]) && strcmp (cur_category, package_array[0]))) {
					g_free (cur_category);
					cur_category = g_strdup (package_array[0]);
					category = xmlNewChild (doc->root, NULL, "CATEGORY", NULL);
					xmlSetProp (category, "name", package_array[0]);
					packages = xmlNewChild (category, NULL, "PACKAGES", NULL);
					g_message ("Category %s...", cur_category);
				}
				
				package = xmlNewChild (packages, NULL, "PACKAGE", NULL);
				
				for (i = 0; i <= num_tags; i++) {
					if (package_array[i+1]) {
						data = xmlNewChild (package, NULL, tags[i], package_array[i+1]);
					} else {
						trilobite_debug ("line %d, tag %d (%s) is missing", index+1, i+1, tags[i]);
					}
				}
				g_strfreev (package_array);
			}
		}
	}
	
	if (doc == NULL) {
		g_warning (_("*** Error generating xml package list! ***\n"));
		xmlFreeDoc (doc);
		return FALSE;
	}

	/* Check for existing file and if, rename, trying to find a .x
	   extension (x being a number) that isn't taken */
	if (g_file_exists (target_file)) {
		char *new_name;
		int c;
		c = 0;
		new_name = NULL;
		do {
			g_free (new_name);
			c++;
			new_name = g_strdup_printf ("%s.%d", target_file, c);
		} while (g_file_exists (new_name));		
		rename (target_file, new_name);
		g_free (new_name);
	}
	xmlSaveFile (target_file, doc);
	xmlFreeDoc (doc);
	return TRUE;

} /* end generate_xml_package_list */

/**
   This just opens the specified file, read it and returns the number of lines
   and reads the contents into "result".
 */
static int
parse_pkg_template (const char* pkg_template_file, char **result) {

	FILE* input_file;
	char buffer[256];
	GString* string_data;
	int lines_read;
	
	g_assert (result!=NULL);

	string_data = g_string_new("");
	(*result) = NULL;
	lines_read = 0;

	input_file = fopen (pkg_template_file, "r");

	if (input_file == NULL) {
		g_warning (_("*** Error reading package list! ***\n"));
		return 0;
	}

	while (fgets (buffer, 255, input_file) != NULL) {
		lines_read++;
		g_string_append (string_data, buffer);
	}

	fclose (input_file);

	(*result) = g_strdup (string_data->str);
	g_string_free (string_data, TRUE);

	return lines_read;
} /* end parse_pkg_template */

/* Creates and returns an xmlnode for a packagedata struct.
   If given a droot and a title, uses that so create a subnode
   (primarily used for the recursiveness of PackageData objects.
   If not, creates a node with the name "PACKAGE" */
xmlNodePtr
eazel_install_packagedata_to_xml (const PackageData *pack, 
				  char *title, 
				  xmlNodePtr droot, 
				  gboolean include_provides)
{
	xmlNodePtr root, node;
	char *tmp;
	GList *iterator;
	/* This is a horrible crackpatch for PR3 used to check for recursive
	   depends... */
	static GList *path = NULL;

	if (droot) {
		g_assert (title != NULL);
		root = xmlNewChild (droot, NULL, title, NULL);
	} else {
		root = xmlNewNode (NULL, "PACKAGE");
	}
	node = xmlNewChild (root, NULL, "NAME", pack->name);
	node = xmlNewChild (root, NULL, "VERSION", pack->version);
	node = xmlNewChild (root, NULL, "MINOR", pack->minor);
	node = xmlNewChild (root, NULL, "ARCH", pack->archtype);
	node = xmlNewChild (root, NULL, "SUMMARY", pack->summary);
	node = xmlNewChild (root, NULL, "DESCRIPTION", pack->description);
	node = xmlNewChild (root, NULL, "STATUS", packagedata_status_enum_to_str (pack->status));
	node = xmlNewChild (root, NULL, "MODSTATUS", packagedata_modstatus_enum_to_str (pack->modify_status));

	tmp = trilobite_get_distribution_name(pack->distribution, FALSE, FALSE);
	node = xmlNewChild (root, NULL, "DISTRIBUTION", tmp);
	g_free (tmp);
	if (pack->distribution.version_major!=-1) {
		tmp = g_strdup_printf ("%d", pack->distribution.version_major);
		xmlSetProp (node, "major", tmp);
		g_free (tmp);
	}
	if (pack->distribution.version_minor!=-1) {
		tmp = g_strdup_printf ("%d", pack->distribution.version_minor);
		xmlSetProp (node, "minor", tmp);
		g_free (tmp);
	}

	tmp = g_strdup_printf ("%d", pack->bytesize);
	node = xmlNewChild (root, NULL, "BYTESIZE", tmp);
	g_free (tmp);

	if (include_provides && pack->provides) {
		tmp = g_strdup ((char*)(pack->provides->data));
		for (iterator = g_list_next (pack->provides); iterator; iterator = g_list_next (iterator)) {
			char *fname = (char*)(iterator->data);
			char *tmp1;
			tmp1 = g_strdup_printf ("%s%c%s", tmp, G_SEARCHPATH_SEPARATOR, fname);
			g_free (tmp);
			tmp = tmp1;			
		}
		node = xmlNewChild (root, NULL, "PROVIDES", tmp);
	}
	
	for (iterator = pack->depends; iterator; iterator = iterator->next) {
		if (g_list_find (path, ((PackageDependency*)iterator->data)->package) == NULL) {
			path = g_list_prepend (path, ((PackageDependency*)iterator->data)->package);
			eazel_install_packagedata_to_xml (((PackageDependency*)iterator->data)->package, 
							  "SOFT_DEPEND", 
							  root, 
							  include_provides);
			path = g_list_remove (path, ((PackageDependency*)iterator->data)->package);
		}
	}
	for (iterator = pack->soft_depends; iterator; iterator = iterator->next) {
		eazel_install_packagedata_to_xml ((PackageData*)iterator->data, 
						  "SOFT_DEPEND", 
						  root, 
						  include_provides);
	}
	for (iterator = pack->hard_depends; iterator; iterator = iterator->next) {
		node = xmlNewChild (root, NULL, "HARD_DEPEND", NULL);
		eazel_install_packagedata_to_xml ((PackageData*)iterator->data, 
						  "HARD_DEPEND", 
						  root,
						  include_provides);
	}
	for (iterator = pack->breaks; iterator; iterator = iterator->next) {
		eazel_install_packagedata_to_xml ((PackageData*)iterator->data, 
						  "BREAKS", 
						  root,
						  include_provides);
	}
	for (iterator = pack->modifies; iterator; iterator = iterator->next) {
		eazel_install_packagedata_to_xml ((PackageData*)iterator->data, 
						  "MODIFIES", 
						  root,
						  include_provides);
	}

	return root;
}

xmlNodePtr
eazel_install_packagelist_to_xml (GList *packages, gboolean include_provides) {
	xmlNodePtr node;
	GList *iterator;

	trilobite_debug ("eazel_install_packagelist_to_xml (%d packages)", g_list_length (packages));
	node = xmlNewNode (NULL, "PACKAGES");
	for (iterator = packages; iterator; iterator = iterator->next) {
		xmlAddChild (node, 
			     eazel_install_packagedata_to_xml ((PackageData*)iterator->data, 
							       NULL, NULL, include_provides)
			);
	}

	return node;	
};

xmlNodePtr
eazel_install_categorydata_to_xml (const CategoryData *category)
{
	xmlNodePtr node;

	node = xmlNewNode (NULL, "CATEGORY");
	xmlSetProp (node, "name", category->name);
	xmlAddChild (node, eazel_install_packagelist_to_xml (category->packages, TRUE));

	return node;
}

static PackageDependency *
osd_parse_provides (PackageData *pack, xmlNodePtr node, GList **feature_list)
{
	xmlNodePtr child;
	GList *list = NULL;
	PackageDependency *dep = NULL;
	char *tmp;
	gboolean got_package = FALSE;

	child = node->xmlChildrenNode;
	while (child) {
		if (g_strcasecmp (child->name, "FILE") == 0) {
			tmp = xmlNodeGetContent (child);
			list = g_list_prepend (list, g_strdup (tmp));
			xmlFree (tmp);
		} else if (g_strcasecmp (child->name, "PACKAGE") == 0) {
			if (got_package) {
				g_warning ("multiple packages in dep list for %s!", pack->name);
			} else {
				dep = packagedependency_new ();
				dep->version = trilobite_xml_get_string (child, "version");
				tmp = trilobite_xml_get_string (child, "sense");
				dep->sense = eazel_softcat_convert_sense_flags (atoi (tmp));
				g_free (tmp);
				got_package = TRUE;
			}
		} else if (g_strcasecmp (child->name, "FEATURE") == 0) {
			tmp = xmlNodeGetContent (child);
			list = g_list_prepend (list, g_strdup (tmp));
			xmlFree (tmp);
		} else {
			trilobite_debug ("Unknown tag %s in xml", child->name);
		}
		child = child->next;
	}

	list = g_list_reverse (list);
	*feature_list = list;
	return dep;
}

static void
osd_parse_dependency (PackageData *pack, xmlNodePtr node)
{
	xmlNodePtr child;
	PackageData *softpack = NULL;
	PackageDependency *dep = NULL;
	GList *features = NULL;

	child = node->xmlChildrenNode;
	while (child) {
		if (g_strcasecmp (child->name, "PROVIDES") == 0) {
			dep = osd_parse_provides (pack, child, &features);
		} else if (g_strcasecmp (child->name, "SOFTPKG") == 0) {
			/* dependent softpkg */
			softpack = osd_parse_softpkg (child);
		} else {
			/* unparsed part of dependency */
		}
		child = child->next;
	}

	if (softpack != NULL) {
		if ((features != NULL) && (softpack->features == NULL)) {
			/* softcat sends separate PROVIDES and SOFTPKG: connect them */
			softpack->features = features;
		}
		if (dep == NULL) {
			dep = packagedependency_new ();
		}
		/* attach this package to a depends struct */
		pack->depends = g_list_prepend (pack->depends, dep);
		dep->package = softpack;
	} else if (features != NULL) {
		g_warning ("stranded PROVIDES block in the dependency info!");
		g_list_foreach (features, (GFunc)g_free, NULL);
		g_list_free (features);
		features = NULL;
	}
}

static void
osd_parse_file_list (PackageData *pack, xmlNodePtr node)
{
	xmlNodePtr child;
	char *tmp;

	child = node->xmlChildrenNode;
	while (child) {
		if (g_strcasecmp (child->name, "FILE") == 0) {
			tmp = xmlNodeGetContent (child);
			pack->provides = g_list_prepend (pack->provides, g_strdup (tmp));
			xmlFree (tmp);
		} else {
			/* bad : thing in file list that isn't a file */
			trilobite_debug ("XML file list contains %s (not FILE)", child->name);
		}
		child = child->next;
	}
	pack->provides = g_list_reverse (pack->provides);
}

static void
osd_parse_feature_list (PackageData *pack, xmlNodePtr node)
{
	xmlNodePtr child;
	char *tmp;

	child = node->xmlChildrenNode;
	while (child) {
		if (g_strcasecmp (child->name, "FEATURE") == 0) {
			tmp = xmlNodeGetContent (child);
			pack->features = g_list_prepend (pack->features, g_strdup (tmp));
			xmlFree (tmp);
		} else {
			trilobite_debug ("XML feature list contains %s (not FILE)", child->name);
		}
		child = child->next;
	}
	pack->features = g_list_reverse (pack->features);
}

static void
osd_parse_implementation (PackageData *pack,
			  xmlNodePtr node)
{
	xmlNodePtr child;

	child = node->xmlChildrenNode;
	while (child) {
		if (g_strcasecmp (child->name, "PROCESSOR")==0) {
			pack->archtype = trilobite_xml_get_string (child, "VALUE");
		} else if (g_strcasecmp (child->name, "OS")==0) {
			char *dtmp = trilobite_xml_get_string (child, "VALUE");
			if (dtmp) {
				pack->distribution.name = trilobite_get_distribution_enum (dtmp,
											   TRUE);
				g_free (dtmp);
			} 
		} else if (g_strcasecmp (child->name, "CODEBASE")==0) {			
			char *tmp;
			pack->remote_url = trilobite_xml_get_string (child, "HREF");
			tmp = trilobite_xml_get_string (child, "SIZE");
			if (tmp) {
				pack->bytesize = atoi (tmp);
				g_free (tmp);
			} else {
				pack->bytesize = 0;
			}
		} else if (g_strcasecmp (child->name, "FILES") == 0) {
			/* oh boy... exhaustive file list */
			osd_parse_file_list (pack, child);
		} else if (g_strcasecmp (child->name, "FEATURES") == 0) {
			/* list of "features" (usually shared libs) in the toplevel package */
			osd_parse_feature_list (pack, child);
		} else if (g_strcasecmp (child->name, "DEPENDENCY")==0) {
			osd_parse_dependency (pack, child);
		} else {
			/* trilobite_debug ("unparsed tag \"%s\" in IMPLEMENTATION", child->name); */
		}
		child = child->next;
	}
}

static PackageData*
osd_parse_softpkg (xmlNodePtr softpkg)
{
	PackageData *result;
	xmlNodePtr child;
	char *tmp;

	result = packagedata_new ();

	result->name = trilobite_xml_get_string (softpkg, "NAME");
	result->version = trilobite_xml_get_string (softpkg, "VERSION");
	result->minor = trilobite_xml_get_string (softpkg, "REVISION");
	result->md5 = trilobite_xml_get_string (softpkg, "MD5");
	
	child = softpkg->xmlChildrenNode;
	while (child) {
		if (g_strcasecmp (child->name, "ABSTRACT")==0) {
			tmp = xmlNodeGetContent (child);
			result->summary = g_strdup (tmp);
			while ((strlen (result->summary) > 0) &&
			       (result->summary[strlen (result->summary)-1] <= ' ')) {
				result->summary[strlen (result->summary)-1] = '\0';
			}
			xmlFree (tmp);
		} else if (g_strcasecmp (child->name, "IMPLEMENTATION")==0) {
			osd_parse_implementation (result, child);
		} else if (g_strcasecmp (child->name, "EAZEL_ID") == 0) {
			result->eazel_id = trilobite_xml_get_string (child, "VALUE");
		} else {
			/* trilobite_debug ("unparsed tag \"%s\" in SOFTPKG", child->name); */
		}
		child = child->next;
	}
	
	return result;
}

static GList*
osd_parse_shared (xmlDocPtr doc)
{
	GList *result;
	xmlNodePtr base, child;

	result = NULL;

	base = doc->root;
	if (base == NULL) {
		g_warning (_("*** The osd xml contains no data! ***\n"));
		return result;
	}

	if (g_strcasecmp (base->name, "PACKAGES") != 0) {
		g_warning (_("*** Bailing from osd parse! ***\n"));
		return result;
	}

	child = base->xmlChildrenNode;
	while (child) {
		if (g_strcasecmp (child->name, "SOFTPKG") == 0) {
			PackageData *pack;
			pack = osd_parse_softpkg (child);
			if (pack) {
				result = g_list_prepend (result, pack);
			} else {
				trilobite_debug ("SOFTPKG parse failed");
			}
		} else {
			trilobite_debug ("child is not a SOFTPKG, but a \"%s\"", child->name);
		}
		child = child->next;
	}

	return result;
}

/* returns FALSE if the XML was all horked */
gboolean
eazel_install_packagelist_parse (GList **list, const char *mem, int size)
{
	xmlDocPtr doc;
	char *ptr, *docptr, *end;
	char *nextnl, *cur;

	*list = NULL;
	if (mem == NULL) {
		return FALSE;
	}

	docptr = g_malloc (size+1);
	memcpy (docptr, mem, size);
	docptr [size] = 0;

	/* libxml is very intolerant of whitespace */
	for (ptr = docptr; (size > 0) && (*ptr <= ' '); ptr++, size--)
		;
	for (end = ptr + size - 1; (size > 0) && (*end <= ' '); *end-- = '\0', size--)
		;

	doc = xmlParseMemory (ptr, size);

	if (doc == NULL) {
		g_warning (_("Could not parse the xml (length %d)"), size);
		for (cur = ptr; (nextnl = strchr (cur, '\n')) != NULL; ) {
			*nextnl = '\0';
			trilobite_debug ("XML: %s", cur);
			cur = nextnl + 1;
		}
		g_free (docptr);
		return FALSE;
	}

	*list = osd_parse_shared (doc);	

	xmlFreeDoc (doc);
	g_free (docptr);
	return TRUE;
}
