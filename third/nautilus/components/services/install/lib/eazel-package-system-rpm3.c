/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* 
 * Copyright (C) 2000 Eazel, Inc
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
 * Authors: Eskil Heyn Olsen <eskil@eazel.com>
 *
 */

/* 
   IMPLEMENTATION NOTE:

   Originally, the rpm3 package system module would open the dbs
   in _new, and close the in _finalize. Addtionally, it would
   close before any rpm spawning and reopen afterwards. This
   close/reopen was needed since rpm needs exclusive lock on
   the db files.

   However, since I now also use the packagesystem object in
   rpmview/packageview, that meant I would keep the db system
   open when installing, thus the install would fail, since
   the view had a open fd on the db's.

   So now, before any operation, I open the db's and close
   afterwards. This sucks pretty much, since eg. during file conflicts
   checking in libeazelinstall, I execute potientially several hundred
   queries in a row - and each query opens/closes the db's. Blech.

*/

#include <config.h>

#ifdef HAVE_RPM

#ifdef HAVE_RPM_30
#define A_DB_FILE "packages.rpm"
#elif HAVE_RPM_40
#define A_DB_FILE "Packages"
#endif

#ifndef A_DB_FILE
#error Unknown DB system
#endif

#include <gnome.h>
#include <locale.h>
#include "eazel-package-system-rpm3-private.h"
#include "eazel-package-system-private.h"
#include <libtrilobite/trilobite-core-utils.h>

#include <rpm/rpmlib.h>
#include <rpm/rpmmacro.h>
#include <rpm/misc.h>

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <locale.h>

#include <libtrilobite/trilobite-root-helper.h>

#define DEFAULT_DB_PATH "/var/lib/rpm"
#define DEFAULT_ROOT "/"

#define USE_PERCENT

EazelPackageSystem* eazel_package_system_implementation (GList*);

/* This is the parent class pointer */
static EazelPackageSystemClass *eazel_package_system_rpm3_parent_class;

/************************************************************
*************************************************************/

#define PERCENTS_PER_RPM_HASH 2

struct RpmMonitorPiggyBag {
	EazelPackageSystemRpm3 *system;
	EazelPackageSystemOperation op;

	unsigned long packages_installed, total_packages;
	unsigned long bytes_installed, total_bytes;

	GList *packages_to_expect;
	GList *packages_seen;

#ifdef USE_PERCENT
	char separator;
	char line[80];
	/* state 1 waiting for package name
                 2 waiting for %%
                 3 reading percentages
	*/
	int state;
	int bytes_read_in_line;
	char *package_name;
#else
	GString *package_name;
#endif
	PackageData *pack;
	double pct;

	GHashTable *name_to_package;

	volatile gboolean subcommand_running;
};

static struct RpmMonitorPiggyBag 
rpmmonitorpiggybag_new (EazelPackageSystemRpm3 *system, 
			EazelPackageSystemOperation op) 
{
	struct RpmMonitorPiggyBag pig;
#ifdef USE_PERCENT
	struct lconv *lc;
#endif

#ifdef USE_PERCENT
	lc = localeconv ();
	pig.separator = *(lc->decimal_point);
	info (system, "decimal separator is '%c'",  pig.separator);
	pig.state = 1;
	pig.bytes_read_in_line = 0;
	pig.line[0] = '\0';
	pig.package_name = NULL;
#else
	pig.pack = NULL;
	pig.package_name = NULL;
#endif
	pig.pct = 0.0;
	pig.system = system;
	pig.op = op;

	pig.packages_seen = NULL;
	pig.packages_installed = 0;	
	pig.bytes_installed = 0;	

	return pig;
}

/* Code to get and set a string field from
   a Header */
void
eazel_package_system_rpm3_get_and_set_string_tag (Header hd,
						  int tag, 
						  char **str)
{
	char *tmp;

	g_assert (str);

	headerGetEntry (hd,
			tag, NULL,
			(void **) &tmp, NULL);
	g_free (*str);
	(*str) = g_strdup (tmp);
}

/* Creates argument list for rpm */
static void
make_rpm_argument_list (EazelPackageSystemRpm3 *system,
			EazelPackageSystemOperation op,
			unsigned long flags,
			const char *dbpath,
			GList *packages,
			GList **args)
{
	GList *iterator;

	for (iterator = packages; iterator; iterator = g_list_next (iterator)) {
		PackageData *pack = (PackageData*)iterator->data;
		if (op == EAZEL_PACKAGE_SYSTEM_OPERATION_INSTALL) {
			(*args) = g_list_prepend (*args, g_strdup (pack->filename));
		} else if (op == EAZEL_PACKAGE_SYSTEM_OPERATION_UNINSTALL) {
			(*args) = g_list_prepend (*args, packagedata_get_name (pack));
		} else {
			g_assert (0);
		}
	}

	if (dbpath) {
		if (op == EAZEL_PACKAGE_SYSTEM_OPERATION_INSTALL &&
		    !(flags & EAZEL_PACKAGE_SYSTEM_OPERATION_DOWNGRADE) &&
		    !(flags & EAZEL_PACKAGE_SYSTEM_OPERATION_UPGRADE)) {
			if (strcmp (dbpath, DEFAULT_DB_PATH)) {
				char *root = g_hash_table_lookup (system->private->db_to_root, dbpath);
				(*args) = g_list_prepend (*args, g_strdup (root));
				(*args) = g_list_prepend (*args, g_strdup ("--prefix"));
			}
		}
		(*args) = g_list_prepend (*args, g_strdup (dbpath));
		(*args) = g_list_prepend (*args, g_strdup ("--dbpath"));		
	}

	if (flags & EAZEL_PACKAGE_SYSTEM_OPERATION_TEST) {
		(*args) = g_list_prepend (*args, g_strdup ("--test"));
	} 

	if (flags & EAZEL_PACKAGE_SYSTEM_OPERATION_FORCE) {
		(*args) = g_list_prepend (*args, g_strdup ("--nodeps"));
		if (op == EAZEL_PACKAGE_SYSTEM_OPERATION_INSTALL) {
			(*args) = g_list_prepend (*args, g_strdup ("--force"));
		}
	}

	/* If the magic epoch ignore is set (and we dont' already have the force flag),
	   set force */
	if (GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (system), "ignore-epochs")) == 1) {
		if (~flags & EAZEL_PACKAGE_SYSTEM_OPERATION_FORCE) {
			(*args) = g_list_prepend (*args, g_strdup ("--force"));
		}
	}

	if (op == EAZEL_PACKAGE_SYSTEM_OPERATION_UNINSTALL) {
		(*args) = g_list_prepend (*args, g_strdup ("-e"));
	} else  {
		if (flags & EAZEL_PACKAGE_SYSTEM_OPERATION_DOWNGRADE) {
			(*args) = g_list_prepend (*args, g_strdup ("--oldpackage"));
		}
		if (flags & EAZEL_PACKAGE_SYSTEM_OPERATION_UPGRADE ||
		    flags & EAZEL_PACKAGE_SYSTEM_OPERATION_DOWNGRADE) {
#ifdef USE_PERCENT
			(*args) = g_list_prepend (*args, g_strdup ("--percent"));
			(*args) = g_list_prepend (*args, g_strdup ("-Uv"));
#else
			(*args) = g_list_prepend (*args, g_strdup ("-Uvh"));
#endif
		} else {
#ifdef USE_PERCENT
			(*args) = g_list_prepend (*args, g_strdup ("--percent"));
			(*args) = g_list_prepend (*args, g_strdup ("-iv"));
#else
			(*args) = g_list_prepend (*args, g_strdup ("-ivh"));
#endif
		}
	}
}

static void
destroy_string_list (GList *list)
{
	g_list_foreach (list, (GFunc)g_free, NULL);
	g_list_free (list);
}

static GHashTable*
rpm_make_names_to_package_hash (GList *packages)
{
	GList *iterator;
	GHashTable *result;

	result = g_hash_table_new (g_str_hash, g_str_equal);

	for (iterator = packages; iterator; iterator = g_list_next (iterator)) {
		char *tmp;		
		PackageData *pack;

		pack = (PackageData*)iterator->data;
		tmp = g_strdup_printf ("%s", pack->name);
		g_hash_table_insert (result,
				     tmp,
				     iterator->data);
	}
	return result;
}

/* A GHRFunc to clean
   out the name_to_package hash table 
*/
static gboolean
clear_name_to_package (char *key,
		       PackageData *pack,
		       gpointer unused)
{
	g_free (key);
	return TRUE;
}

static unsigned long
get_total_size_of_packages (const GList *packages)
{
	const GList *iterator;
	unsigned long result = 0;
	for (iterator = packages; iterator; iterator = g_list_next (iterator)) {
		PackageData *pack = (PackageData*)iterator->data;
		result += pack->bytesize;
	}
	return result;
}

static void
eazel_package_system_rpm3_set_mod_status (EazelPackageSystemRpm3 *system,
					  EazelPackageSystemOperation op,
					  PackageData *pack)
{
	if (pack->modify_status == PACKAGE_MOD_UNTOUCHED) {
		switch (op) {
		case EAZEL_PACKAGE_SYSTEM_OPERATION_INSTALL:
			pack->modify_status = PACKAGE_MOD_INSTALLED;
			break;
		case EAZEL_PACKAGE_SYSTEM_OPERATION_UNINSTALL:
			pack->modify_status = PACKAGE_MOD_UNINSTALLED;
			break;
		default:
			break;
		}
	}
}

#ifdef USE_PERCENT
/* This monitors an rpm process pipe and emits
   signals during execution */
/* Ahhh, the joy of not having C++ and not just be able
   to define a function class, but instead I get to carry
   the pig around.... */
static gboolean
monitor_rpm_process_pipe_percent_output (GIOChannel *source,
					 GIOCondition condition,
					 struct RpmMonitorPiggyBag *pig)
{
	char tmp;
	ssize_t bytes_read;

	bytes_read = 0;
	g_io_channel_read (source, &tmp, 1, &bytes_read);

	if (! bytes_read) {
		pig->subcommand_running = FALSE;
		return FALSE;
	}

	if (bytes_read) {
		if (isspace (tmp)) {
			switch (pig->state) {
			case 1:
				if (pig->package_name) {
					g_free (pig->package_name);
				}
				/* Reset */
				pig->pct = 0.0;
				
				pig->package_name = g_strdup (pig->line);
				pig->pack = g_hash_table_lookup (pig->name_to_package, pig->package_name);

				if (pig->pack==NULL) {
					char *dash;
					dash = strrchr (pig->package_name, '-');
					while (dash && pig->pack==NULL) {
						*dash = '\0';
						pig->pack = g_hash_table_lookup (pig->name_to_package, 
										 pig->package_name);
						dash = strrchr (pig->package_name, '-');
					}
				}

				if (pig->pack==NULL) {
					if (pig->package_name && 
					    ((strcmp (pig->package_name, "warning:") == 0) ||
					     (strcmp (pig->package_name, "error:") == 0) ||
					     (strcmp (pig->package_name, "cannot") == 0))) {
						fail (pig->system, "rpm says \"%s\"", pig->package_name);
					} else if (pig->package_name) {
						verbose (pig->system, "lookup \"%s\" failed", 
							 pig->package_name);
					}
				} else {
					unsigned long longs[EAZEL_PACKAGE_SYSTEM_PROGRESS_LONGS];

					info (pig->system, "matched \"%s\"", pig->package_name);

					pig->packages_installed ++;
					
					longs[0] = 0;
					longs[1] = pig->pack->bytesize;
					longs[2] = pig->packages_installed;
					longs[3] = pig->total_packages;
					longs[4] = pig->bytes_installed;
					longs[5] = pig->total_bytes;
					
					eazel_package_system_emit_start (EAZEL_PACKAGE_SYSTEM (pig->system),
									 pig->op,
									 pig->pack);
					eazel_package_system_emit_progress (EAZEL_PACKAGE_SYSTEM (pig->system),
									    pig->op,
									    pig->pack,
									    longs);
					/* switch state */
					pig->state = 2;
				}

				break;

			case 2:
				if (strncmp (pig->line, "%%", 2) == 0) {
					pig->state = 3;
				}
				break;

			case 3: {
				double pct;

				/* Assume we don't go to state 1 */
				pig->state = 2;

				/* Grab the percentage */
				pct = strtod (pig->line, NULL);
				/* fix rounding errors */
				pct = ((int)(pct*1000))/1000.0;

				/* Higher ? */
				if (pct > pig->pct) {
					unsigned long longs[EAZEL_PACKAGE_SYSTEM_PROGRESS_LONGS];
					int amount;

					pig->pct = pct;
					if (pig->pct == 100.0) {
						amount = pig->pack->bytesize;
					} else {
						amount = (int)((pig->pack->bytesize * pig->pct) / 100.0);
					}

					longs[0] = amount;
					longs[1] = pig->pack->bytesize;
					longs[2] = pig->packages_installed;
					longs[3] = pig->total_packages;
					longs[4] = pig->bytes_installed + amount;
					longs[5] = pig->total_bytes;
					
					eazel_package_system_emit_progress (EAZEL_PACKAGE_SYSTEM (pig->system),
									    pig->op,
									    pig->pack, 
									    longs);

					/* Done with package ? */
					if (pig->pct == 100.0) {
						pig->state = 1;
						pig->bytes_installed += pig->pack->bytesize;
						pig->packages_seen = g_list_prepend (pig->packages_seen,
										     pig->pack);

						eazel_package_system_rpm3_set_mod_status (pig->system,
											  pig->op,
											  pig->pack);

						eazel_package_system_emit_end (EAZEL_PACKAGE_SYSTEM (pig->system),
									       pig->op,
									       pig->pack);
						
						g_free (pig->package_name);
						pig->package_name = NULL;
						pig->pack = NULL;
						pig->pct = 0.0;
					}
					
				}
			}
			break;
			default:
				g_assert_not_reached ();
			}
			pig->bytes_read_in_line = 0;
		} else {
			if (pig->bytes_read_in_line > 79) {
				trilobite_debug ("Read more than I expected, resetting");
				pig->bytes_read_in_line = 0;
			} else {
				pig->line[pig->bytes_read_in_line] = tmp;
				pig->bytes_read_in_line++;
				pig->line[pig->bytes_read_in_line] = '\0';
			}
		}
	}

	pig->subcommand_running = TRUE;
	return TRUE;
}
#endif

#ifndef USE_PERCENT
/* This monitors an rpm process pipe and emits
   signals during execution */
static gboolean
monitor_rpm_process_pipe (GIOChannel *source,
			  GIOCondition condition,
			  struct RpmMonitorPiggyBag *pig)
{
	char         tmp;
	ssize_t      bytes_read;
	gboolean result = TRUE;

	g_io_channel_read (source, &tmp, 1, &bytes_read);
	
	if (bytes_read) {
		/* Percentage output, parse and emit... */
		if (tmp=='#') {
			int amount = 0;
			if (pig->pack == NULL) {
				return TRUE;
			}
			pig->pct += PERCENTS_PER_RPM_HASH;
			if (pig->pct == 100.0) {
				amount = pig->pack->bytesize;
			} else {
				amount = (int)((pig->pack->bytesize * pig->pct)/100.0);
			}
			if (pig->pack && amount) {
				unsigned long longs[EAZEL_PACKAGE_SYSTEM_PROGRESS_LONGS];
				
				longs[0] = amount;
				longs[1] = pig->pack->bytesize;
				longs[2] = pig->packages_installed;
				longs[3] = pig->total_packages;
				longs[4] = pig->bytes_installed + amount;
				longs[5] = pig->total_bytes;

				eazel_package_system_emit_progress (EAZEL_PACKAGE_SYSTEM (pig->system),
								    pig->op,
								    pig->pack, 
								    longs);
			}
			/* By invalidating the pointer here, we
			   only emit with amount==total once and
			   also emit end here */
			if (pig->pct == 100.0) {
				pig->bytes_installed += pig->pack->bytesize;
				pig->packages_seen = g_list_prepend (pig->packages_seen,
								     pig->pack);
				info (pig->system, "seen.size = %d", g_list_length (pig->packages_seen));
				eazel_package_system_emit_end (EAZEL_PACKAGE_SYSTEM (pig->system),
							       pig->op,
							       pig->pack);
				
				pig->pack = NULL;
				pig->pct = 0.0;
				g_string_free (pig->package_name, TRUE);
				pig->package_name = NULL;
			}
		}  else  if (!isspace (tmp)) {		       
			/* Paranoia check */
			if (pig->package_name) {
				g_string_free (pig->package_name, TRUE);
			}

			/* Reset */
			pig->package_name = g_string_new (NULL);
			pig->pack = NULL;
			
                        /* Read untill we hit a space */
			while (bytes_read && !isspace (tmp)) {
				g_string_append_c (pig->package_name, tmp);
				g_io_channel_read (source, &tmp, 1, &bytes_read);
			}
			
			/* It's not a #, and we've read a full word */

			/* first check is this an expected file name ? */
			if (pig->package_name &&
			    pig->package_name->str &&
			    g_list_find_custom (pig->packages_to_expect,
						pig->package_name->str,
						(GCompareFunc)eazel_install_package_name_compare)) {
				if (pig->package_name) {
					pig->pack = g_hash_table_lookup (pig->name_to_package, pig->package_name->str);
				} 
			} else {
				fail (pig->system, "\"%s\" wasn't expected", pig->package_name->str);
			}
					

			if (pig->pack==NULL) {
				if (pig->package_name && 
				    pig->package_name->str &&
				    ((strcmp (pig->package_name->str, "warning:") == 0) ||
				     (strcmp (pig->package_name->str, "error:") == 0) ||
				     (strcmp (pig->package_name->str, "cannot") == 0))) {
					while (tmp != '\n') {
						g_string_append_c (pig->package_name, tmp);
						g_io_channel_read (source, &tmp, 1, &bytes_read);
					}
					fail (pig->system, "rpm says \"%s\"", pig->package_name->str);
				} else if (pig->package_name) {
					verbose (pig->system, "lookup \"%s\" failed", pig->package_name->str);
				}
			} else {
				unsigned long longs[EAZEL_PACKAGE_SYSTEM_PROGRESS_LONGS];
				info (pig->system, "matched \"%s\"", pig->package_name->str);
				pig->pct = 0.0;
				pig->packages_installed ++;
				
				longs[0] = 0;
				longs[1] = pig->pack->bytesize;
				longs[2] = pig->packages_installed;
				longs[3] = pig->total_packages;
				longs[4] = pig->bytes_installed;
				longs[5] = pig->total_bytes;

				eazel_package_system_emit_start (EAZEL_PACKAGE_SYSTEM (pig->system),
								 pig->op,
								 pig->pack);
				eazel_package_system_emit_progress (EAZEL_PACKAGE_SYSTEM (pig->system),
								    pig->op,
								    pig->pack,
								    longs);

			}
		}
	} 

	if (bytes_read == 0) {
		result = FALSE;
	} else {
		result = TRUE;
	}
	
	pig->subcommand_running = result;

	return result;
}
#endif

static void
rpm_create_db (char *dbpath,
	       char *root,
	       EazelPackageSystemRpm3 *system)
{
	addMacro (NULL, "_dbpath", NULL, "/", 0);

	if (strcmp (root, "/")) {
		info (system, "Creating %s", dbpath);
		mkdir (dbpath, 0700);
		rpmdbInit (dbpath, 0644);
	}
}

void
eazel_package_system_rpm3_create_dbs (EazelPackageSystemRpm3 *system,
				      GList *dbpaths)
{	
	GList *iterator;

	g_assert (system);
	g_assert (EAZEL_IS_PACKAGE_SYSTEM_RPM3 (system));
	g_assert (system->private->dbs);

	system->private->dbpaths = dbpaths;
	for (iterator = dbpaths; iterator; iterator = g_list_next (iterator)) {
		char *db = (char*)iterator->data;
		char *root = (char*)(iterator = g_list_next (iterator))->data;

		info (system, "Adding %s as root for %s", root, db);
		g_hash_table_insert (system->private->db_to_root, db, root);
	}

	g_hash_table_foreach (system->private->db_to_root, (GHFunc)rpm_create_db, system);

	info (system, "Read rpmrc file");
	rpmReadConfigFiles ("/usr/lib/rpm/rpmrc", NULL);	
}

/* This is not a safe way of lockchecking, 
   since after the check, a process may still gain
   a lock on a_file.
   This is not a major problem, as the case we're looking
   for is when a process is running from start */
static gboolean
eazel_package_system_rpm3_db_locked (EazelPackageSystemRpm3 *system,
				     char *dbpath)
{
	char *a_file;
	struct flock lock;
	int fd;
	gboolean result = FALSE;

	lock.l_type = F_RDLCK;
	lock.l_start = 0;
	lock.l_whence = 0;
	lock.l_len = 0;

	a_file = g_strdup_printf ("%s/%s", dbpath, A_DB_FILE);
	fd = open (a_file, O_RDONLY);

	/* verbose (system, "lock checking %s", a_file); */

	if (fd == -1) {
		/* fail (system, "Could not get lock info for %s during open phase", a_file); */
	} else {
		if (fcntl (fd, F_SETLK, &lock)) {
			fail (system, "Could not get lock for %s", a_file);
			result = TRUE;
			if (EAZEL_PACKAGE_SYSTEM (system)->err==NULL) {
				EAZEL_PACKAGE_SYSTEM (system)->err = g_new0 (EazelPackageSystemError, 1);
				EAZEL_PACKAGE_SYSTEM (system)->err->e = EazelPackageSystemError_DB_ACCESS;
				EAZEL_PACKAGE_SYSTEM (system)->err->u.db_access.pid = lock.l_pid;
				EAZEL_PACKAGE_SYSTEM (system)->err->u.db_access.path = dbpath;
			}
		} else {
			result = FALSE;
		}
	}
			
	close (fd);
	g_free (a_file);
	
	return result;
}

static gboolean
eazel_package_system_rpm3_dbs_locked (EazelPackageSystemRpm3 *system)
{
	GList *iterator;
	gboolean result = FALSE;
	GList *remove = NULL;
	
	for (iterator = system->private->dbpaths; iterator; iterator = g_list_next (g_list_next (iterator))) {
		char *path;
		char *foo = g_list_next (iterator)->data;

		path = (char*)iterator->data;
		if (eazel_package_system_rpm3_db_locked (system, path)) {
			result = TRUE;
			fail (system, "Removed %s since it's locked", path);
			remove = g_list_prepend (remove, path);
			remove = g_list_prepend (remove, foo);
		}
	}
	
	for (iterator = remove; iterator; iterator = g_list_next (iterator)) {
		system->private->dbpaths = g_list_remove (system->private->dbpaths, iterator->data);
		g_hash_table_remove (system->private->db_to_root, iterator->data);
	}
	g_list_free (remove);
	
	return result;
}

static void
rpm_open_db (char *dbpath,
	     char *root,
	     EazelPackageSystemRpm3 *system)
{
	rpmdb db;

	addMacro(NULL, "_dbpath", NULL, "/", 0);
	if (rpmdbOpen (dbpath, &db, O_RDONLY, 0644)) {
		fail (system, "Opening packages database in %s failed (a)", dbpath);
	} else {			
		if (db) {
			info (system, _("Opened packages database in %s"), dbpath);
			g_hash_table_insert (system->private->dbs,
					     g_strdup (dbpath),
					     db);
		} else {
			fail (system, _("Opening packages database in %s failed"), dbpath);
		}
	}
}

gboolean
eazel_package_system_rpm3_open_dbs (EazelPackageSystemRpm3 *system)
{
	g_assert (system);
	g_assert (EAZEL_IS_PACKAGE_SYSTEM_RPM3 (system));
	g_assert (system->private->dbs);

	if (eazel_package_system_rpm3_dbs_locked (system)) {
		g_warning ("Some db's are locked!");
	}

	if (g_hash_table_size (system->private->db_to_root) == 0) {
		return FALSE;
	}

	g_hash_table_foreach (system->private->db_to_root, 
			      (GHFunc)rpm_open_db,
			      system);

	return TRUE;
}

static gboolean
rpm_close_db (char *key, 
	       rpmdb db, 
	       EazelPackageSystemRpm3 *system)
{
	if (db) {
		info (system, _("Closing db for %s (open)"), key);
		rpmdbClose (db);
		db = NULL;
		g_free (key);
	} else {
		fail (system, _("Closing db for %s (not open)"), key);
	}

	return TRUE;
}

gboolean
eazel_package_system_rpm3_close_dbs (EazelPackageSystemRpm3 *system)
{
	/* Close all the db's */
	g_assert (system->private->dbs);
	g_hash_table_foreach_remove (system->private->dbs, 
				     (GHRFunc)rpm_close_db,
				     system);	
	return TRUE;
}

static gboolean
rpm_free_db (char *key, 
	     char *root, 
	     EazelPackageSystemRpm3 *system)
{
	g_free (key);
	g_free (root);
	return TRUE;
}

gboolean
eazel_package_system_rpm3_free_dbs (EazelPackageSystemRpm3 *system)
{
	/* Close all the db's */
	g_assert (system->private->dbs);
	g_hash_table_foreach_remove (system->private->db_to_root, 
				     (GHRFunc)rpm_free_db,
				     system);	
	return TRUE;
}

/************************************************************
 Load Package implemementation
*************************************************************/

static EazelSoftCatSense
rpm_sense_to_softcat_sense (EazelPackageSystemRpm3 *system,
			    int rpm_sense) 
{
	EazelSoftCatSense result = 0;

	if (rpm_sense & RPMSENSE_ANY) {
		result |= EAZEL_SOFTCAT_SENSE_ANY;
	} else {
		if (rpm_sense & RPMSENSE_EQUAL) {
			result |= EAZEL_SOFTCAT_SENSE_EQ;
		}
		if (rpm_sense & RPMSENSE_GREATER) {
			result |= EAZEL_SOFTCAT_SENSE_GT;
		}
		if (rpm_sense & RPMSENSE_LESS) {
			result |= EAZEL_SOFTCAT_SENSE_LT;
		}
	}

	return result;
}

/* This is used for those freaky packages (Acrobat-3.01-2.i386.rpm)
   that both provides libs and requires the same (libagm.so libpfs.so) */
static gboolean
check_require_is_not_a_feature (const char *requires_name, 
				const char **provides_names, 
				int provide_count)
{
	int i;

	if (requires_name == NULL) {
		return TRUE;
	}

	for (i = 0; i < provide_count; i++) {
		if (provides_names[i] && strcmp (requires_name, provides_names[i])==0) {
			return FALSE;
		}
	}
	return TRUE;
}

void 
eazel_package_system_rpm3_packagedata_fill_from_header (EazelPackageSystemRpm3 *system,
							PackageData *pack, 
							Header hd,
							int detail_level)
{
	unsigned long *sizep;
	unsigned long *epochp;

	eazel_package_system_rpm3_get_and_set_string_tag (hd, RPMTAG_NAME, &pack->name);
	eazel_package_system_rpm3_get_and_set_string_tag (hd, RPMTAG_VERSION, &pack->version);
	eazel_package_system_rpm3_get_and_set_string_tag (hd, RPMTAG_RELEASE, &pack->minor);

	eazel_package_system_rpm3_get_and_set_string_tag (hd, RPMTAG_ARCH, &pack->archtype);
	if (~detail_level & PACKAGE_FILL_NO_TEXT) {
		eazel_package_system_rpm3_get_and_set_string_tag (hd, RPMTAG_DESCRIPTION, &pack->description);
		eazel_package_system_rpm3_get_and_set_string_tag (hd, RPMTAG_SUMMARY, &pack->summary);
	}

	headerGetEntry (hd,
			RPMTAG_SIZE, NULL,
			(void **) &sizep, NULL);	

	pack->bytesize = *sizep;

	/* Load the crack that is epoch/serial */
	if (!headerGetEntry (hd, RPMTAG_EPOCH, NULL, (void**)&epochp, NULL)) {
		pack->epoch = 0;
	} else {
		pack->epoch = *epochp;
	}

	pack->packsys_struc = (gpointer)hd;
	
	pack->fillflag = detail_level;

	{
		char **obsoletes = NULL;
		int count = 0;
		int i;
	/* FIXME: bugzilla.eazel.com 6903
	   obsoletes is not a string, it's a stringlist! */
		
		headerGetEntry (hd,			
				RPMTAG_OBSOLETENAME, NULL,
				(void**)&obsoletes, 
				&count);
		
		for (i = 0; i < count; i++) {
			pack->obsoletes = g_list_prepend (pack->obsoletes, g_strdup (obsoletes[i]));
		}
		free (obsoletes);
	}

	/* FIXME: bugzilla.eazel.com 4863 */
	if (~detail_level & PACKAGE_FILL_NO_PROVIDES) {
		char **paths = NULL;
		char **paths_copy = NULL;
		char **names = NULL;
		int *indexes = NULL;
		int count = 0;
		int index = 0;
		int num_paths = 0;
		uint_16 *file_modes;

		g_list_foreach (pack->provides, (GFunc)g_free, NULL);
		g_list_free (pack->provides);
		pack->provides = NULL;

                /* RPM v.3.0.4 and above has RPMTAG_BASENAMES, this will not work
		   with any version below 3.0.4 */

		headerGetEntry (hd,			
				RPMTAG_DIRINDEXES, NULL,
				(void**)&indexes, NULL);
		headerGetEntry (hd,			
				RPMTAG_DIRNAMES, NULL,
				(void**)&paths, &num_paths);
		headerGetEntry (hd,			
				RPMTAG_BASENAMES, NULL,
				(void**)&names, &count);
		headerGetEntry (hd,			
				RPMTAG_FILEMODES, NULL,
				(void**)&file_modes, NULL);

		/* Copy all paths and shave off last /.
		   This is needed to remove the dir entries from 
		   the packagedata's provides list. */
		paths_copy = g_new0 (char*, num_paths);
		for (index=0; index<num_paths; index++) {
			paths_copy[index] = g_strdup (paths[index]);
			paths_copy[index][strlen (paths_copy[index]) - 1] = 0;
		}

		/* Now loop through all the basenames */
		for (index=0; index<count; index++) {
			char *fullname = NULL;
			if (paths) {
				fullname = g_strdup_printf ("%s/%s", paths_copy[indexes[index]], names[index]);
			} else {
				fullname = g_strdup (names[index]);
			}
#if 0
			fprintf (stderr, "file_modes[%s] = 0%o %s\n", 
				 fullname, file_modes[index],
				 (file_modes[index] & 040000) ? "DIR" : "file" );
#endif
			
			if (detail_level & PACKAGE_FILL_NO_DIRS_IN_PROVIDES) {
				if (file_modes[index] & 040000) {
					g_free (fullname);
					fullname = NULL;
				}
			}
			if (fullname) {
#if 0
				fprintf (stderr, "%s provides %s\n", pack->name, fullname);
#endif
				pack->provides = g_list_prepend (pack->provides, fullname);
			}
		}
		pack->provides = g_list_reverse (pack->provides);
		for (index=0; index<num_paths; index++) {
			g_free (paths_copy[index]);
		}
		g_free (paths_copy);
		free ((void*)paths);
		free ((void*)names);
	}


	if (~detail_level & PACKAGE_FILL_NO_DEPENDENCIES) {		
		const char **requires_name, **requires_version, **provides_names;
		int *requires_flag;
		int count, provide_count;
		int index;

		headerGetEntry (hd,
				RPMTAG_PROVIDENAME, NULL,
				(void**)&provides_names,
				&provide_count);
		headerGetEntry (hd,
				RPMTAG_REQUIRENAME, NULL,
				(void**)&requires_name,
				&count);
		headerGetEntry (hd,
				RPMTAG_REQUIREVERSION, NULL,
				(void**)&requires_version,
				NULL);
		headerGetEntry (hd,
				RPMTAG_REQUIREFLAGS, NULL,
				(void**)&requires_flag,
				NULL);

		for (index = 0; index < count; index++) {
			PackageData *package = packagedata_new ();
			PackageDependency *pack_dep = packagedependency_new ();

			/* If it's a lib*.so* or a /yadayada, but not ld-linux.so
			   or rpmlib( add to provides */
			if ((strncmp (requires_name[index], "lib", 3)==0 && 
			     strstr (requires_name[index], ".so")) ||
			    (*requires_name[index]=='/')) {
				if (check_require_is_not_a_feature (requires_name[index], 
								    provides_names, 
								    provide_count) == TRUE) {
					package->features = g_list_prepend (package->features, 
									    g_strdup (requires_name[index]));
				}
			} else if ((strncmp (requires_name[index], "ld-linux.so", 11) == 0) ||
				   (strncmp (requires_name[index], "rpmlib(", 7) == 0)) { 
				/* foo */
			} else {
				/* Otherwise, add as a package name */
				package->name = g_strdup (requires_name[index]);
				/* and set the version if not empty */
				pack_dep->version = *requires_version[index]=='\0' ? 
					NULL : g_strdup (requires_version[index]);
			}
			/* If anything set, add dep */
			if (package->name || package->features) {
				pack_dep->sense = rpm_sense_to_softcat_sense (system,
									      requires_flag[index]);
				package->archtype = trilobite_get_distribution_arch ();
				pack_dep->package = package;
				pack->depends = g_list_prepend (pack->depends, pack_dep);
			} else {
				packagedependency_destroy (pack_dep);
				gtk_object_unref (GTK_OBJECT (package));
			}
		}
		free ((void*)provides_names);
		free ((void*)requires_name);
		free ((void*)requires_version);

	}

	if (~detail_level & PACKAGE_FILL_NO_FEATURES) {		
		const char **provides_name;
		int count;
		int index;

		headerGetEntry (hd,
				RPMTAG_PROVIDENAME, NULL,
				(void**)&provides_name,
				&count);

		for (index = 0; index < count; index++) {
			pack->features = g_list_prepend (pack->features, 
							 g_strdup (provides_name[index]));
		}

		free ((void*)provides_name);

	
	}

}

static gboolean 
rpm_packagedata_fill_from_file (EazelPackageSystemRpm3 *system,
				PackageData *pack, 
				const char *filename, 
				int detail_level)
{
	static FD_t fd;
	Header hd;
	int rpm_result;

	/* Set filename field */
	if (pack->filename != filename) {
		g_free (pack->filename);
		pack->filename = g_strdup (filename);
	}

	/* FIXME: Would be better to call a package_data_ function to do this. */
	if (pack->packsys_struc) {
		headerFree ((Header) pack->packsys_struc);
		pack->packsys_struc = NULL;
	}

	/* Open rpm */
	fd = fdOpen (filename, O_RDONLY, 0);

	if (fd == NULL) {
		g_warning (_("Cannot open %s"), filename);
		pack->status = PACKAGE_CANNOT_OPEN;
		return FALSE;
	}

	/* Get Header block */
	rpm_result = rpmReadPackageHeader (fd, &hd, &pack->source_package, NULL, NULL);
	if (rpm_result == 0) {
		eazel_package_system_rpm3_packagedata_fill_from_header (system, pack, hd, detail_level);
		pack->status = PACKAGE_UNKNOWN_STATUS;
	}

	fdClose (fd);

	return (rpm_result == 0) ? TRUE : FALSE;
}

static PackageData* 
rpm_packagedata_new_from_file (EazelPackageSystemRpm3 *system,
			       const char *file, 
			       int detail_level)
{
	PackageData *pack;

	pack = packagedata_new ();

	if (rpm_packagedata_fill_from_file (system, pack, file, detail_level)==FALSE) {
		trilobite_debug ("RPM3 unable to fill from file '%s'", file);
		gtk_object_unref (GTK_OBJECT (pack));
		pack = NULL;
	}

	return pack;
}

PackageData*
eazel_package_system_rpm3_load_package (EazelPackageSystemRpm3 *system,
					PackageData *in_package,
					const char *filename,
					int detail_level)
{
	PackageData *result = NULL;

	if (in_package) {
		result = in_package;
		if (rpm_packagedata_fill_from_file (system, result, filename, detail_level)==FALSE) {
			trilobite_debug ("RPM3 unable to fill from file '%s'", filename);
			result = NULL;
		}
	} else {
		result = rpm_packagedata_new_from_file (system, filename, detail_level);
	}

	return result;
}

/************************************************************
 Query implemementation
*************************************************************/

rpmdb
eazel_package_system_rpm3_get_db (EazelPackageSystemRpm3 *system,
				  const char *dbpath)
{
	rpmdb db;

	db = g_hash_table_lookup (system->private->dbs, dbpath);
	if (!db) {
		fail (system, "query could not access db in %s", dbpath);
		return NULL;
	}
	return db;
}

#ifdef HAVE_RPM_30
static void
eazel_package_system_rpm3_query_impl (EazelPackageSystemRpm3 *system,
				      const char *dbpath,
				      const char* key,
				      EazelPackageSystemQueryEnum flag,
				      int detail_level,
				      GList **result)
{
	rpmdb db = eazel_package_system_rpm3_get_db (system, dbpath);
	int rc = 1;
	dbiIndexSet matches;

	if (!db) {
		return;
	}

	switch (flag) {
	case EAZEL_PACKAGE_SYSTEM_QUERY_OWNS:		
		info (system, "query (in %s) OWNS %s", dbpath, key);
		rc = rpmdbFindByFile (db, key, &matches);
		break;
	case EAZEL_PACKAGE_SYSTEM_QUERY_PROVIDES:		
		info (system, "query (in %s) PROVIDES %s", dbpath, key);
		rc = rpmdbFindByProvides (db, key, &matches);
		break;
	case EAZEL_PACKAGE_SYSTEM_QUERY_MATCHES:
		info (system, "query (in %s) MATCHES %s", dbpath, key);
		rc = rpmdbFindPackage (db, key, &matches);
		break;
	case EAZEL_PACKAGE_SYSTEM_QUERY_REQUIRES:
		info (system, "query (in %s) REQUIRES %s", dbpath, key);
		rc = rpmdbFindByRequiredBy (db, key, &matches);
		break;
	default:
		g_warning ("Unknown query flag %d", flag);
		g_assert_not_reached ();
	}
	       
	if (rc == 0) {
		unsigned int i;		

		info (system, "%d hits", dbiIndexSetCount (matches));
		for (i = 0; i < dbiIndexSetCount (matches); i++) {
			unsigned int offset;
			Header hd;
			PackageData *pack;
			
			offset = dbiIndexRecordOffset (matches, i);
			hd = rpmdbGetRecord (db, offset);
			pack = packagedata_new ();
			eazel_package_system_rpm3_packagedata_fill_from_header (system, 
										pack, 
										hd, 
										detail_level);
			g_free (pack->install_root);
			pack->install_root = g_strdup (dbpath);
			if (g_list_find_custom (*result, 
						pack, 
						(GCompareFunc)eazel_install_package_compare)!=NULL) {
				info (system, "%s already in set", pack->name);
				gtk_object_unref (GTK_OBJECT (pack));
			} else {
				(*result) = g_list_prepend (*result, pack);
			}
		}
		dbiFreeIndexRecord (matches);
	} else {
		info (system, "0 hits");
	}
}

static void
eazel_package_system_rpm3_query_substr (EazelPackageSystemRpm3 *system,
					const char *dbpath,
					const char *key,
					int detail_level,
					GList **result)
{
	int offset;
	rpmdb db = eazel_package_system_rpm3_get_db (system, dbpath);
 
	if (!db) {
		return;
	}

	for (offset = rpmdbFirstRecNum (db); offset; offset = rpmdbNextRecNum (db, offset)) {
		Header hd;
		char *name = NULL;
		
		hd = rpmdbGetRecord (db, offset);

		eazel_package_system_rpm3_get_and_set_string_tag (hd, RPMTAG_NAME, &name);

		/* If key occurs in name, create package and add to result */
		if (strstr (name, key)) {
			PackageData *pack = packagedata_new ();			
			eazel_package_system_rpm3_packagedata_fill_from_header (system, 
										pack, 
										hd, 
										detail_level);
			(*result) = g_list_prepend (*result, pack);
		} else {
			headerFree (hd);
		}
		g_free (name);
	}
	
}

static void
eazel_package_system_rpm3_query_foreach (char *dbpath,
					 rpmdb db,
					 struct RpmQueryPiggyBag *pig)
{
	info (pig->system, "eazel_package_system_rpm3_query_foreach");
	switch (pig->flag) {
	case EAZEL_PACKAGE_SYSTEM_QUERY_OWNS:		
	case EAZEL_PACKAGE_SYSTEM_QUERY_PROVIDES:		
	case EAZEL_PACKAGE_SYSTEM_QUERY_MATCHES:
		eazel_package_system_rpm3_query_impl (pig->system,
						      dbpath,
						      pig->key,
						      pig->flag,
						      pig->detail_level,
						      pig->result);
		break;
	case EAZEL_PACKAGE_SYSTEM_QUERY_REQUIRES:
		eazel_package_system_rpm3_query_requires (pig->system,
							  dbpath,
							  pig->key,
							  pig->detail_level,
							  pig->result);
		break;
	case EAZEL_PACKAGE_SYSTEM_QUERY_REQUIRES_FEATURE:
		eazel_package_system_rpm3_query_requires_feature (pig->system,
								  dbpath,
								  pig->key,
								  pig->detail_level,
								  pig->result);
		break;
	case EAZEL_PACKAGE_SYSTEM_QUERY_SUBSTR:
		eazel_package_system_rpm3_query_substr (pig->system,
							dbpath,
							pig->key,
							pig->detail_level,
							pig->result);
		break;
	default:
		g_warning ("Unknown query flag %d", pig->flag);
		g_assert_not_reached ();
	}
}
#endif /* HAVE_RPM_30 */

void
eazel_package_system_rpm3_query_requires (EazelPackageSystemRpm3 *system,
					  const char *dbpath,
					  const gpointer *key,
					  int detail_level,
					  GList **result)
{
	const PackageData *pack = (PackageData*)key;

	if (pack->name) {
		(EAZEL_PACKAGE_SYSTEM_RPM3_CLASS (GTK_OBJECT (system)->klass)->query_impl) (EAZEL_PACKAGE_SYSTEM (system),
											    dbpath,
											    pack->name,
											    EAZEL_PACKAGE_SYSTEM_QUERY_REQUIRES,
											    detail_level,
											    result);
	}
	if (pack->provides) {
		GList *iterator;
		/* FIXME: ideally, this could use package->features instead, that would
		   be safer then doing the strstr check. But for now, I just check if
		   fkey is "lib.*\.so.*", or "/bin/.*" or "/sbin/.*" */
		for (iterator = pack->provides; iterator; iterator = g_list_next (iterator)) {
			const char *fkey = (const char*)iterator->data;
			if ((strncmp (g_basename (fkey), "lib", 3)==0 && strstr (fkey, ".so")) ||
			    strncmp (fkey, "/bin/", 5)==0 ||
			    strncmp (fkey, "/sbin/", 6)==0) {
				(EAZEL_PACKAGE_SYSTEM_RPM3_CLASS (GTK_OBJECT (system)->klass)->query_impl) (EAZEL_PACKAGE_SYSTEM (system),
													    dbpath,
													    g_basename (fkey),
													    EAZEL_PACKAGE_SYSTEM_QUERY_REQUIRES,
													    detail_level,
													    result);
			}
		}
		info (system, "result set size is now %d", g_list_length (*result));
	}
}

void
eazel_package_system_rpm3_query_requires_feature (EazelPackageSystemRpm3 *system,
						  const char *dbpath,
						  const gpointer *key,
						  int detail_level,
						  GList **result)
{
	(EAZEL_PACKAGE_SYSTEM_RPM3_CLASS (GTK_OBJECT (system)->klass)->query_impl) (EAZEL_PACKAGE_SYSTEM (system),
										    dbpath,
										    (gpointer)key,
										    EAZEL_PACKAGE_SYSTEM_QUERY_REQUIRES,
										    detail_level,
										    result);
}

GList*               
eazel_package_system_rpm3_query (EazelPackageSystemRpm3 *system,
				 const char *dbpath,
				 const gpointer key,
				 EazelPackageSystemQueryEnum flag,
				 int detail_level)
{
	GList *result = NULL;
	struct RpmQueryPiggyBag pig;

	info (system, "eazel_package_system_rpm3_query (dbpath=\"%s\", key=%p, flag=%d, detail=%d)", 
	      dbpath, key, flag, detail_level);
	
	pig.system = system;
	pig.key = key;
	pig.flag = flag;
	pig.detail_level = detail_level;
	pig.result = &result;
	
	if (!eazel_package_system_rpm3_open_dbs (system)) {
		return NULL;
	}

	if (dbpath==NULL) {
		g_hash_table_foreach (system->private->dbs, 
				      (GHFunc)(EAZEL_PACKAGE_SYSTEM_RPM3_CLASS (GTK_OBJECT (system)->klass)->query_foreach),
				      &pig);
	} else {
		(EAZEL_PACKAGE_SYSTEM_RPM3_CLASS (GTK_OBJECT (system)->klass)->query_foreach) (dbpath, NULL, &pig);
	}
	eazel_package_system_rpm3_close_dbs (system);

	return result;
}

/************************************************************
 Install implemementation
*************************************************************/

static void
display_arguments (EazelPackageSystemRpm3 *system,
		   GList *args) 
{
	char *str, *tmp;
	GList *iterator;

	str = g_strdup ("rpm");
	for (iterator = args; iterator; iterator = g_list_next (iterator)) {
		tmp = g_strdup_printf ("%s %s", str, (char*)iterator->data);
		g_free (str);
		str = tmp;
		/* Since there is a max length on g_message output ... */
		if (strlen (str) > 600) {
			fail (system, "%s", str);
			g_free (str);
			str = g_strdup ("");
		}
	}
	fail (system, "%s", str);
}

static void
monitor_subcommand_pipe (EazelPackageSystemRpm3 *system,
			 int fd, 
			 GIOFunc monitor_func,
			 struct RpmMonitorPiggyBag *pig)
{
	GIOChannel *channel;

	pig->subcommand_running = TRUE;
	channel = g_io_channel_unix_new (fd);

	info (system, "beginning monitor on %d", fd);
	g_io_add_watch_full (channel, 10, G_IO_IN | G_IO_ERR | G_IO_NVAL | G_IO_HUP, 
			     monitor_func, 
			     pig, NULL);

	while (pig->subcommand_running) {
#if 0
		/* this is evil and it still doesn't work, so foo. */
		while (gdk_events_pending ()) {
			gtk_main_do_event (gdk_event_get ());
		}
#endif
		gtk_main_iteration ();
	}
	info (system, "ending monitor on %d", fd);
}

static void
eazel_package_system_rpm3_set_state (EazelPackageSystemRpm3 *system,
				     GList *packages,
				     PackageSystemStatus status) {
	GList *iterator;
	for (iterator = packages; iterator; iterator = g_list_next (iterator)) {
		PackageData *pack = PACKAGEDATA (iterator->data);
		pack->status = status;
	}
}


/* returns TRUE on success */
static gboolean
manual_rpm_command (GList *args, int *fd)
{
	char **argv;
	int i, errfd, child_pid;
	GList *iterator;
	gboolean result;

	/* Create argv list */
	argv = g_new0 (char*, g_list_length (args) + 2);
	argv[0] = g_strdup ("rpm");
	i = 1;
	for (iterator = args; iterator; iterator = g_list_next (iterator)) {
		argv[i] = g_strdup (iterator->data);
		i++;
	}
	argv[i] = NULL;

	if (access ("/bin/rpm", R_OK|X_OK)!=0) {
		g_warning ("/bin/rpm missing or not executable for uid");
		result = FALSE;
		goto out;
	} 
	/* start /bin/rpm... */
	if ((child_pid = trilobite_pexec ("/bin/rpm", argv, NULL, fd, &errfd)) == 0) {
		g_warning ("Could not start rpm");
		result = FALSE;
	} else {
		trilobite_debug ("/bin/rpm running (pid %d, stdout %d, stderr %d)", child_pid, *fd, errfd);
		//close (errfd);
		result = TRUE;
	}

out:	
	for (i = 0; argv[i]; i++) {
		g_free (argv[i]);
	}
	g_free (argv);
	return result;
}

static void 
eazel_package_system_rpm3_execute (EazelPackageSystemRpm3 *system,
				   struct RpmMonitorPiggyBag *pig,
				   GList *args)
{
	TrilobiteRootHelper *root_helper;
	int fd;
	gboolean go = TRUE;

	display_arguments (system, args);

	root_helper = gtk_object_get_data (GTK_OBJECT (system), "trilobite-root-helper");
	if (root_helper) {
		TrilobiteRootHelperStatus root_helper_stat;

		root_helper_stat = trilobite_root_helper_start (root_helper);
		if (root_helper_stat != TRILOBITE_ROOT_HELPER_SUCCESS) {
			g_warning ("Error in starting trilobite_root_helper");			
			go = FALSE;
		} else if (trilobite_root_helper_run (root_helper, 
						      TRILOBITE_ROOT_HELPER_RUN_RPM, args, &fd) != 
			   TRILOBITE_ROOT_HELPER_SUCCESS) {
			g_warning ("Error in running trilobite_root_helper");
			trilobite_root_helper_destroy (GTK_OBJECT (root_helper));
			go = FALSE;
		}
	} else {
		/* start /bin/rpm manually -- we're in bootstrap installer mode */
		go = manual_rpm_command (args, &fd);
	}
	if (go) {
#ifdef USE_PERCENT
		monitor_subcommand_pipe (system, fd, (GIOFunc)monitor_rpm_process_pipe_percent_output, pig);
#else
		monitor_subcommand_pipe (system, fd, (GIOFunc)monitor_rpm_process_pipe, pig);
#endif
	} else {
		eazel_package_system_rpm3_set_state (system, pig->packages_to_expect, PACKAGE_CANCELLED);
		/* FIXME: fail all the packages in pig */
	}
}

/* If any package in "packages" does not occur in "seen",
   emit failed signal */
static void
check_if_all_packages_seen (EazelPackageSystemRpm3 *system, 
			    const char *dbpath,
			    EazelPackageSystemOperation op,
			    int flags,
			    GList *packages,
			    GList *seen)
{
	GList *iterator;
	
	/* HACK: that fixes bugzilla.eazel.com 4914 */		  
	eazel_package_system_rpm3_open_dbs (system);

	for (iterator = packages; iterator; iterator = g_list_next (iterator)) {
		PackageData *pack = (PackageData*)iterator->data;

		if (flags & EAZEL_PACKAGE_SYSTEM_OPERATION_TEST) {
				eazel_package_system_emit_start (EAZEL_PACKAGE_SYSTEM (system),
								 op,
								 pack);				
				eazel_package_system_rpm3_set_mod_status (system,
									  op,
									  pack);
				eazel_package_system_emit_end (EAZEL_PACKAGE_SYSTEM (system),
							       op,
							       pack);
				continue;
		} 
		
		/* HACK: that fixes bugzilla.eazel.com 4914 */		  
		if (op==EAZEL_PACKAGE_SYSTEM_OPERATION_UNINSTALL) {
			if (eazel_package_system_is_installed (EAZEL_PACKAGE_SYSTEM (system),
							       dbpath,
							       pack->name,
							       pack->version,
							       pack->minor,
							       EAZEL_SOFTCAT_SENSE_EQ)) {
				fail (system, "%s is still installed", pack->name);
				eazel_package_system_emit_failed (EAZEL_PACKAGE_SYSTEM (system), op, pack);
			} else {
				eazel_package_system_emit_start (EAZEL_PACKAGE_SYSTEM (system),
								 op,
								 pack);				
				eazel_package_system_rpm3_set_mod_status (system,
									  op,
									  pack);
				eazel_package_system_emit_end (EAZEL_PACKAGE_SYSTEM (system),
							       op,
							       pack);
			}
		} else {
			if (!g_list_find_custom (seen, 
						 pack,
						 (GCompareFunc)eazel_install_package_compare)) {
				fail (system, "did not see %s", pack->name);
					eazel_package_system_emit_failed (EAZEL_PACKAGE_SYSTEM (system), 
									  op, pack);
			}
		}
	}

	/* HACK: that fixes bugzilla.eazel.com 4914 */		  
	eazel_package_system_rpm3_close_dbs (system);
}
			    

static void 
eazel_package_system_rpm3_install_uninstall (EazelPackageSystemRpm3 *system, 
					     EazelPackageSystemOperation op,
					     const char *dbpath,
					     GList* packages,
					     unsigned long flags)
{
	struct RpmMonitorPiggyBag pig = rpmmonitorpiggybag_new (system, op);
	GList *args = NULL;

	pig.system = system;
	pig.op = op;

	pig.total_packages = g_list_length (packages);
	pig.total_bytes = get_total_size_of_packages (packages);
	pig.packages_to_expect = packages;
	pig.name_to_package = rpm_make_names_to_package_hash (packages);

	make_rpm_argument_list (system, op, flags, dbpath, packages, &args);
	eazel_package_system_rpm3_execute (system, &pig, args);
	destroy_string_list (args);

	check_if_all_packages_seen (system, dbpath, op, flags, packages, pig.packages_seen);
	g_list_free (pig.packages_seen);

	g_hash_table_foreach_remove (pig.name_to_package, (GHRFunc)clear_name_to_package, NULL);
	g_hash_table_destroy (pig.name_to_package);
}

void                 
eazel_package_system_rpm3_install (EazelPackageSystemRpm3 *system, 
				   const char *dbpath,
				   GList* packages,
				   unsigned long flags)
{
	info (system, "eazel_package_system_rpm3_install");

	eazel_package_system_rpm3_install_uninstall (system, 
						     EAZEL_PACKAGE_SYSTEM_OPERATION_INSTALL,
						     dbpath,
						     packages,
						     flags);
}

/************************************************************
 Uninstall implemementation
*************************************************************/

void                 
eazel_package_system_rpm3_uninstall (EazelPackageSystemRpm3 *system, 
				     const char *dbpath,
				     GList* packages,
				     unsigned long flags)
{
	info (system, "eazel_package_system_rpm3_uninstall");
	eazel_package_system_rpm3_install_uninstall (system, 
						     EAZEL_PACKAGE_SYSTEM_OPERATION_UNINSTALL,
						     dbpath,
						     packages,
						     flags);
}

/************************************************************
 Verify implemementation
*************************************************************/

static gboolean
eazel_package_system_rpm3_verify_impl (EazelPackageSystemRpm3 *system, 
				       const char *root,
				       PackageData *package,
				       unsigned long *info,
				       gboolean *cont)
{
	unsigned int i;
	int v_result;
	unsigned long infoblock[EAZEL_PACKAGE_SYSTEM_PROGRESS_LONGS];
	gboolean result;
	char *p_name = packagedata_get_readable_name (package);

	g_assert (package->packsys_struc);

	infoblock[0] = 0;
	infoblock[1] = g_list_length (package->provides);
	infoblock[2] = info[0];
	infoblock[3] = info[1];
	infoblock[4] = info[2];
	infoblock[5] = info[3];

	(*cont) = eazel_package_system_emit_start (EAZEL_PACKAGE_SYSTEM (system), 
						   EAZEL_PACKAGE_SYSTEM_OPERATION_VERIFY,
						   package);
	/* abort if signal returns false */
	if (*cont == FALSE) {
		g_free (p_name);
		return FALSE;
	}

	result = TRUE;

	for (i = 0; i < g_list_length (package->provides); i++) {
		int res;
		/* next file... */
		infoblock [0]++;
		infoblock [4]++;
		
		info (system, "checking file %d/%d \"%s\" from \"%s\"", 
		      infoblock[0], g_list_length (package->provides),
		      (char*)((g_list_nth (package->provides, i))->data),
		      p_name);

		(*cont) = eazel_package_system_emit_progress (EAZEL_PACKAGE_SYSTEM (system), 
							      EAZEL_PACKAGE_SYSTEM_OPERATION_VERIFY,
							      package, 
							      infoblock);		/* abort if signal returns false */
		if (*cont == FALSE) {
			result = FALSE;
			break;
		}
		res = rpmVerifyFile ("", (Header)package->packsys_struc, i, &v_result, RPMVERIFY_NONE);
		if (v_result!=0) {
			fail (system, "file %d (%s) failed", i,
			      (char*)((g_list_nth (package->provides, i))->data));
			(*cont) = eazel_package_system_emit_failed (EAZEL_PACKAGE_SYSTEM (system), 
								    EAZEL_PACKAGE_SYSTEM_OPERATION_VERIFY,
								    package);
			
			result = FALSE;

			/* abort if signal returns false */
			if (*cont == FALSE) {
				break;
			}
		} 
		
	}
	/* Update the total-amount-completed counter */
	info[2] = infoblock[4];

	if (*cont) {
		(*cont) = eazel_package_system_emit_end (EAZEL_PACKAGE_SYSTEM (system), 
							 EAZEL_PACKAGE_SYSTEM_OPERATION_VERIFY,
							 package);
		/* no need to check, called will abort if *cont == FALSE */
	}
	g_free (p_name);
	return result;
}

static unsigned long
get_num_of_files_in_packages (GList *packages)
{
	GList *iterator;
	unsigned long result = 0;
	for (iterator = packages; iterator; iterator = g_list_next (iterator)) {
		PackageData *pack = (PackageData*)iterator->data;
		result += g_list_length (pack->provides);
	}
	return result;
}

gboolean
eazel_package_system_rpm3_verify (EazelPackageSystemRpm3 *system, 
				  const char *dbpath,
				  GList* packages)
{
	GList *iterator;
	char *root = ""; /* FIXME: fill this using dbpath */
	unsigned long info[4];
	gboolean cont = TRUE;
	gboolean result = TRUE;

	info[0] = 0;
	info[1] = g_list_length (packages);
	info[2] = 0; /* updated by eazel_package_system_rpm3_verify_impl */
	info[3] = get_num_of_files_in_packages (packages);

	info (system, "eazel_package_system_rpm3_verify");

	if (!eazel_package_system_rpm3_open_dbs (system)) {
		return FALSE;
	}
	for (iterator = packages; iterator; iterator = g_list_next (iterator)) {
		PackageData *pack = (PackageData*)iterator->data;
		info[0] ++;
		if (eazel_package_system_rpm3_verify_impl (system, root, pack, info, &cont) == FALSE) {
			result = FALSE;
		}
		if (cont == FALSE) {
			break;
		}
	}
	eazel_package_system_rpm3_close_dbs (system);
	return result;
}

/************************************************************
 Version compare implementation							    
*************************************************************/

int
eazel_package_system_rpm3_compare_version (EazelPackageSystem *system,
					   const char *a,
					   const char *b)
{
	int result;
	result = rpmvercmp (a, b);
	/* Special bandaid for M18 <> 0.7 mozilla versions */
	if (isdigit (*a) && *b=='M') {
		if (result < 0) { result = abs (result); }
	}
	return result;
}

/************************************
 Database mtime implementation
************************************/
time_t
eazel_package_system_rpm3_database_mtime (EazelPackageSystemRpm3 *system)
{
	struct stat st;

	if (stat (DEFAULT_DB_PATH "/" A_DB_FILE, &st) == 0) {
		return st.st_mtime;
	} else {
		return (time_t)0;
	}


}

/*****************************************
  GTK+ object stuff
*****************************************/

static void
eazel_package_system_rpm3_finalize (GtkObject *object)
{
	EazelPackageSystemRpm3 *system;

	g_return_if_fail (object != NULL);
	g_return_if_fail (EAZEL_PACKAGE_SYSTEM_RPM3 (object));

	system = EAZEL_PACKAGE_SYSTEM_RPM3 (object);

	eazel_package_system_rpm3_free_dbs (system);
	g_hash_table_destroy (system->private->dbs);

	if (GTK_OBJECT_CLASS (eazel_package_system_rpm3_parent_class)->finalize) {
		GTK_OBJECT_CLASS (eazel_package_system_rpm3_parent_class)->finalize (object);
	}
}

static void
eazel_package_system_rpm3_class_initialize (EazelPackageSystemRpm3Class *klass) 
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass*)klass;
	object_class->finalize = eazel_package_system_rpm3_finalize;
	
	eazel_package_system_rpm3_parent_class = gtk_type_class (eazel_package_system_get_type ());
#ifdef HAVE_RPM_30
	klass->query_foreach = (EazelPackageSystemRpmQueryForeachFunc)eazel_package_system_rpm3_query_foreach;
	klass->query_impl = (EazelPackageSystemRpmQueryImplFunc)eazel_package_system_rpm3_query_impl;
#else
	klass->query_foreach = NULL;
	klass->query_impl = NULL;
#endif /* HAVE_RPM_30 */
}

static void
eazel_package_system_rpm3_initialize (EazelPackageSystemRpm3 *system) 
{
	g_assert (system != NULL);
	g_assert (EAZEL_IS_PACKAGE_SYSTEM_RPM3 (system));
	
	system->private = g_new0 (EazelPackageSystemRpm3Private, 1);
	system->private->dbs = g_hash_table_new (g_str_hash, g_str_equal);
	system->private->db_to_root = g_hash_table_new (g_str_hash, g_str_equal);
}

GtkType
eazel_package_system_rpm3_get_type() {
	static GtkType system_type = 0;

	/* First time it's called ? */
	if (!system_type)
	{
		static const GtkTypeInfo system_info =
		{
			"EazelPackageSystemRpm3",
			sizeof (EazelPackageSystemRpm3),
			sizeof (EazelPackageSystemRpm3Class),
			(GtkClassInitFunc) eazel_package_system_rpm3_class_initialize,
			(GtkObjectInitFunc) eazel_package_system_rpm3_initialize,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL,
		};

		system_type = gtk_type_unique (eazel_package_system_get_type (), &system_info);
	}

	return system_type;
}

EazelPackageSystemRpm3 *
eazel_package_system_rpm3_new (GList *dbpaths) 
{
	EazelPackageSystemRpm3 *system;

	g_return_val_if_fail (dbpaths, NULL);

	system = EAZEL_PACKAGE_SYSTEM_RPM3 (gtk_object_new (TYPE_EAZEL_PACKAGE_SYSTEM_RPM3, NULL));

	gtk_object_ref (GTK_OBJECT (system));
	gtk_object_sink (GTK_OBJECT (system));

	eazel_package_system_rpm3_create_dbs (system, dbpaths);

	return system;
}

#ifdef HAVE_RPM_30
EazelPackageSystem*
eazel_package_system_implementation (GList *dbpaths)
{
	EazelPackageSystem *result;
	GList *tdbpaths = dbpaths;

	g_message ("Eazel Package System - rpm3");
	
	tdbpaths = g_list_prepend (tdbpaths, g_strdup (DEFAULT_ROOT));
	tdbpaths = g_list_prepend (tdbpaths, g_strdup (DEFAULT_DB_PATH));
	result = EAZEL_PACKAGE_SYSTEM (eazel_package_system_rpm3_new (tdbpaths));
	
	result->private->load_package = 
		(EazelPackageSytemLoadPackageFunc)eazel_package_system_rpm3_load_package;
	result->private->query = (EazelPackageSytemQueryFunc)eazel_package_system_rpm3_query;
	result->private->install = (EazelPackageSytemInstallFunc)eazel_package_system_rpm3_install;
	result->private->uninstall = (EazelPackageSytemUninstallFunc)eazel_package_system_rpm3_uninstall;
	result->private->verify = (EazelPackageSytemVerifyFunc)eazel_package_system_rpm3_verify;
	result->private->compare_version = 
		(EazelPackageSystemCompareVersionFunc)eazel_package_system_rpm3_compare_version;
	result->private->database_mtime = 
		(EazelPackageSystemDatabaseMtimeFunc)eazel_package_system_rpm3_database_mtime;

	return result;
}
#endif /* HAVE_RPM_30 */

#endif /* HAVE_RPM */
