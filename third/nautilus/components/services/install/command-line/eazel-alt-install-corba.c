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

/* Return values
   0 = all ok
   !0 = failure

   1 = downloads failed
   2 = dep/install failed
   3 = md5 failure (man-in-the-middle, softcat inconsistency or bad download)
*/

#include <config.h>
#include <gnome.h>
#include <liboaf/liboaf.h>
#include <bonobo.h>
#include <sys/utsname.h>

#include <eazel-package-system-types.h>
#include <eazel-install-corba-types.h>
#include <eazel-install-corba-callback.h>
#include <eazel-install-problem.h>

#include <libtrilobite/libtrilobite.h>
#include <trilobite-eazel-install.h>

#include <unistd.h>

#define PACKAGE_FILE_NAME "package-list.xml"

#define DEFAULT_PROTOCOL PROTOCOL_HTTP
#define DEFAULT_RPMRC "/usr/lib/rpm/rpmrc"
#define DEFAULT_REMOTE_PACKAGE_LIST "/package-list.xml"
#define DEFAULT_REMOTE_RPM_DIR "/RPMS"
#define DEFAULT_LOG_FILE "/tmp/eazel-install/log"

/* This ensure that if the arch is detected as i[3-9]86, the
   requested archtype will be set to i386 */
#define ASSUME_ix86_IS_i386 

#define OAF_ID "OAFIID:trilobite_eazel_install_service:8ff6e815-1992-437c-9771-d932db3b4a17"

/* Popt stuff */
int     arg_dry_run = 0,
	arg_http = 0,
	arg_ftp = 0,
	arg_local = 0,
	arg_debug = 0,
	arg_delay = 0,
	arg_file = 0,
	arg_force = 0,
	arg_upgrade = 0,
	arg_downgrade = 0,
	arg_erase = 0,
	arg_query = 0,
	arg_revert = 0,
	arg_ssl_rename = 0,
	arg_provides = 0,
	arg_verbose = 0,
	arg_id = 0,
	arg_suite = 0,
	arg_ei2 = 0,
	arg_no_pct = 0,
	arg_no_auth = 0,
	arg_silent = 0,
	arg_machine = 0;
char    *arg_server = NULL,
	*arg_cgi = NULL,
	*arg_config_file = NULL,
	*arg_package_list = NULL,
	*arg_username = NULL,
	*arg_root = NULL,
	*arg_batch = NULL,
	*arg_version = NULL;

/* Yeahyeah, but this was initially a test tool,
   so stop whining... */
CORBA_ORB orb;
CORBA_Environment ev;
int cli_result = 0;
GList *cases = NULL;
GList *categories;
gboolean downloaded_files = FALSE;
gboolean auto_cont = FALSE;

static const struct poptOption options[] = {
	{"batch", '\0', POPT_ARG_STRING, &arg_batch, 0, N_("Set the default answer to continue, also default delete to Yes"), NULL},
	{"cgi-path", '\0', POPT_ARG_STRING, &arg_cgi, 0, N_("Specify search cgi"), NULL},
	{"debug", '\0', POPT_ARG_NONE, &arg_debug, 0 , N_("Show debug output"), NULL},
	{"delay", '\0', POPT_ARG_NONE, &arg_delay, 0 , N_("10 sec delay after starting service"), NULL},
	{"downgrade", 'd', POPT_ARG_NONE, &arg_downgrade, 0, N_("Allow downgrades"), NULL},
	{"erase", 'e', POPT_ARG_NONE, &arg_erase, 0, N_("Erase packages"), NULL},
	{"ei2", '\0', POPT_ARG_NONE, &arg_ei2, 0, N_("enable ei2"), NULL},
	{"file",'\0', POPT_ARG_NONE, &arg_file, 0, N_("RPM args are filename"), NULL},
	{"ftp", 'f', POPT_ARG_NONE, &arg_ftp, 0, N_("Use ftp"), NULL},
	{"local", 'l', POPT_ARG_NONE, &arg_local, 0, N_("Use local"), NULL},
	{"http", 'h', POPT_ARG_NONE, &arg_http, 0, N_("Use http"), NULL},
	{"id", 'i', POPT_ARG_NONE, &arg_id, 0, N_("RPM args are Eazel Ids"), NULL},
	{"machine", 'm', POPT_ARG_NONE, &arg_machine, 0, N_("machine readable output"), NULL},
	{"no-percent", '\0', POPT_ARG_NONE, &arg_no_pct, 0, N_("Don't print fancy percent output"), NULL},
	{"no-auth", '\0', POPT_ARG_NONE, &arg_no_auth, 0, N_("don't use eazel auth stuff"), NULL},
	{"packagefile", '\0', POPT_ARG_STRING, &arg_package_list, 0, N_("Specify package file"), NULL},
	{"provides", '\0', POPT_ARG_NONE, &arg_provides, 0, N_("RPM args are needed files"), NULL},
	{"query", 'q', POPT_ARG_NONE, &arg_query, 0, N_("Run Query"), NULL},
	/* Disabled for 1.0 */
	/* {"revert", 'r', POPT_ARG_NONE, &arg_revert, 0, N_("Revert"), NULL}, */
	{"root", '\0', POPT_ARG_STRING, &arg_root, 0, N_("Set root"), NULL},
	{"server", '\0', POPT_ARG_STRING, &arg_server, 0, N_("Specify server"), NULL},
	{"silent", '\0', POPT_ARG_NONE, &arg_silent, 0, N_("Dont print too much, just problems and download"), NULL},
	{"ssl-rename", 's', POPT_ARG_NONE, &arg_ssl_rename, 0, N_("Perform ssl renaming"), NULL},
	{"suite", '\0', POPT_ARG_NONE, &arg_suite, 0, N_("argument is a suite id"), NULL},
	{"test", 't', POPT_ARG_NONE, &arg_dry_run, 0, N_("Test run"), NULL},
	{"username", '\0', POPT_ARG_STRING, &arg_username, 0, N_("Allow username"), NULL},
	{"upgrade", 'u', POPT_ARG_NONE, &arg_upgrade, 0, N_("Allow upgrades"), NULL},
	{"verbose", 'v', POPT_ARG_NONE, &arg_verbose, 0, N_("Verbose output"), NULL},
	{"package-version", 'V', POPT_ARG_STRING, &arg_version, 0, N_("Install a specific package version"), "version"},
	{NULL, '\0', 0, NULL, 0}
};

#define check_ev(s)                                                \
if (ev._major!=CORBA_NO_EXCEPTION) {                               \
	fprintf (stderr, "*** %s: Caught exception %s",            \
                 s, CORBA_exception_id (&ev));                     \
}

static void
set_parameters_from_command_line (GNOME_Trilobite_Eazel_Install service)
{
	if (!arg_debug) {		
		GNOME_Trilobite_Eazel_Install__set_log_file (service, DEFAULT_LOG_FILE, &ev);
		check_ev ("set_log_file");
	} else {
		GNOME_Trilobite_Eazel_Install__set_debug (service, TRUE, &ev);
		check_ev ("set_debug");
	}

	/* We only want 1 protocol type */
	if (arg_http + arg_ftp + arg_local > 1) {
			fprintf (stderr, _("*** You cannot specify more then one protocol type.\n"));
			exit (1);
	}
	if (arg_http) {
		GNOME_Trilobite_Eazel_Install__set_protocol (service, GNOME_Trilobite_Eazel_PROTOCOL_HTTP, &ev);
		check_ev ("set_protocol");
	} else if (arg_ftp) {
		GNOME_Trilobite_Eazel_Install__set_protocol (service, GNOME_Trilobite_Eazel_PROTOCOL_FTP, &ev);
		check_ev ("set_protocol");
	} else if (arg_local) {
		GNOME_Trilobite_Eazel_Install__set_protocol (service, GNOME_Trilobite_Eazel_PROTOCOL_LOCAL, &ev);
		check_ev ("set_protocol");
	} else {
		GNOME_Trilobite_Eazel_Install__set_protocol (service, GNOME_Trilobite_Eazel_PROTOCOL_HTTP, &ev);
		check_ev ("set_protocol");
	}
	if (arg_erase + arg_revert > 1) {
			fprintf (stderr, _("*** Erase and revert ?  Somebody set up us the bomb!\n"));
			exit (1);
	}
	if (arg_upgrade) {
		GNOME_Trilobite_Eazel_Install__set_upgrade (service, TRUE, &ev);
		check_ev ("upgrade");
	}
	if (arg_downgrade) {
		GNOME_Trilobite_Eazel_Install__set_downgrade (service, TRUE, &ev);
		check_ev ("downgrade");
	}
	if (arg_server) {
		char *colon = strchr (arg_server, ':');
		if (colon) {
			char *host;
			int port;

			host = g_new0(char, (colon - arg_server) + 1);
			strncpy (host, arg_server, colon - arg_server);
			colon++;
			port = atoi (colon);
			GNOME_Trilobite_Eazel_Install__set_server (service, host, &ev);
			check_ev ("set_server");
			GNOME_Trilobite_Eazel_Install__set_server_port (service, port, &ev);
			check_ev ("set_port");
			g_free (host);
		} else {
			GNOME_Trilobite_Eazel_Install__set_server (service, arg_server, &ev);
			check_ev ("set_server");
		}
		GNOME_Trilobite_Eazel_Install__set_auth (service, FALSE, &ev);
		check_ev ("set_auth");
	} else if (arg_no_auth==0) {
		char *host, *p;
		int port;

		host = g_strdup (trilobite_get_services_address ());
		if ((p = strchr (host, ':')) != NULL) {
			*p = 0;
			port = atoi (p+1);
		} else {
			port = 80;
		}
		GNOME_Trilobite_Eazel_Install__set_auth (service, TRUE, &ev);
		check_ev ("set_auth");
		GNOME_Trilobite_Eazel_Install__set_server (service, host, &ev);
		check_ev ("set_server");
		GNOME_Trilobite_Eazel_Install__set_server_port (service, port, &ev);
		check_ev ("set_port");
		g_free (host);
	}
	if (arg_username) {
		GNOME_Trilobite_Eazel_Install__set_username (service, arg_username, &ev);
		check_ev ("set_username");
	}
	if (arg_cgi) {
		GNOME_Trilobite_Eazel_Install__set_cgi (service, arg_cgi, &ev);
		check_ev ("set_cgi");
	}

	if (arg_ssl_rename) {
		GNOME_Trilobite_Eazel_Install__set_ssl_rename (service, TRUE, &ev);
		check_ev ("set_ssl_rename");
	}
	if (arg_ei2) {
		GNOME_Trilobite_Eazel_Install__set_ei2 (service, TRUE, &ev);
		check_ev ("set_ei2");
	}
	if (arg_no_auth) {
		GNOME_Trilobite_Eazel_Install__set_auth (service, FALSE, &ev);
		check_ev ("set_auth");
	} 
	if (arg_dry_run) {
		GNOME_Trilobite_Eazel_Install__set_test_mode (service, TRUE, &ev);
	}
	if (arg_force) {
		GNOME_Trilobite_Eazel_Install__set_force (service, TRUE, &ev);
	}
	if (arg_package_list) {
		GNOME_Trilobite_Eazel_Install__set_package_list (service, arg_package_list, &ev);
		check_ev ("packagelist");
	}

	if (arg_root) {
		if (arg_root[0]=='~') {
			char *tmp = g_strdup_printf ("%s/%s", g_get_home_dir (), 
						     arg_root+1);
			free (arg_root);
			arg_root = strdup (tmp);
			g_free (tmp);
		} else if (arg_root[0]!='/' || arg_root[0]=='.') {
			char *tmp = g_strdup_printf ("%s%s%s", g_get_current_dir (), 
						     arg_root[0]=='.' ? "" : "/",
						     arg_root+1);
			free (arg_root);
			arg_root = strdup (tmp);
			g_free (tmp);
		}
		g_message ("DB root = %s", arg_root);
	}
}

static void 
eazel_file_conflict_check_signal (EazelInstallCallback *service, 
				  const PackageData *pack,
				  gpointer unused)
{
	if (!arg_silent) {
		if (!arg_machine) {
			printf (_("File conflict checking %s...\n"), pack->name);
		}
	} else {
		printf ("."); fflush (stdout);
	}
}

static void 
eazel_file_uniqueness_check_signal (EazelInstallCallback *service, 
				    const PackageData *pack,
				    gpointer unused)
{
	if (!arg_silent) {
		if (! arg_machine) {
			printf (_("File uniqueness checking %s...\n"), pack->name);
		}
	} else {
		printf ("."); fflush (stdout);
	}
}

static void 
eazel_feature_consistency_check_signal (EazelInstallCallback *service, 
					const PackageData *pack,
					gpointer unused)
{
	if (!arg_silent) {
		if (!arg_machine) {
			printf (_("Feature consistency checking %s...\n"), pack->name);
		}
	} else {
		printf ("."); fflush (stdout);
	}
}

static void 
eazel_download_progress_signal (EazelInstallCallback *service, 
				const PackageData *pack,
				int amount, 
				int total,
				gpointer unused) 
{
	static time_t t;
	static int pct;
	static int old_pct;

	time_t end;
	time_t diff;
	static float ks=0;

	g_assert (pack->name != NULL);

	downloaded_files = TRUE;

	if (arg_machine) {
		return;
	}

	if (amount==0) {
		fprintf (stdout, _("Downloading %s..."), pack->name);
		t = time (NULL);
		old_pct = pct = 0;
	} else if (amount != total ) {
		if (arg_no_pct==0) {
			pct = total != 0 ? (amount * 100) / total : 100;
			if (pct > 5) {
				if (old_pct != pct && pct%5==0) {
					end = time (NULL);
					diff = end - t;
					ks = ((float)amount/1024)/diff;
					old_pct = pct;
				}
				/* I18N note: %s is a package name, %d/%d is bytes/totalbytes,
				   the next %d is a percentage completed, %1.f is a KB/s */
				fprintf (stdout, "\r");
				fprintf (stdout, _("Downloading %s... (%d/%d) = %d%% %.1f KB/s"), 
					 pack->name,
					 amount, total, pct,
					 ks);
				fprintf (stdout, "        \r");

			} else {
				fprintf (stdout, "\r");
				fprintf (stdout, _("Downloading %s... (%d/%d) = %d%%"), 
					 pack->name,
					 amount, total, pct);
			}
		}
	} else if (amount == total && total!=0) {
		if (arg_no_pct==0) {
			fprintf (stdout, "\r");
			fprintf (stdout, _("Downloading %s... (%d/%d) %.1f KB/s Done      \n"),
				 pack->name,
				 amount, total, 
				 ks);
		} else {
			fprintf (stdout, _("Downloading %s... %3.1f KB/s Done\n"), 
				 pack->name, ks);
		}
	}
	fflush (stdout);
}

static void 
eazel_install_progress_signal (EazelInstallCallback *service, 
			       const PackageData *pack,
			       int package_num, int num_packages, 
			       int amount, int total,
			       int total_size_completed, int total_size, 
			       char *title)
{
	char *packname;

	packname = packagedata_get_readable_name (pack);

	if (amount==0) {
		if (!arg_machine) {
			fprintf (stdout, "%s %s", title, packname);
		}
	} else if (amount != total ) {
		if (arg_no_pct==0) {
			if (!arg_machine) {
				if (strcasecmp (title, "Installing")==0) {
					fprintf (stdout, "\r");
					fprintf (stdout, _("Installing %s (%d/%d), (%d/%d)b - (%d/%d) = %d%%"), 
						 packname,
						 package_num, num_packages,
						 total_size_completed, total_size,
						 amount, total,
						 total != 0 ? (amount * 100) / total : 100);
				} else {
					fprintf (stdout, "\r");
					fprintf (stdout, _("Uninstalling %s (%d/%d), (%d/%d)b - (%d/%d) = %d%%"), 
						 packname,
						 package_num, num_packages,
						 total_size_completed, total_size,
						 amount, total,
						 total != 0 ? (amount * 100) / total : 100);

				}
			}
		}
	}
	if (amount == total && total!=0) {
		if (arg_machine) {
			if (title && strcasecmp (title, "Installing")==0) {
				fprintf (stdout, "INSTALLED %s\n", pack->name);				
			} else {
				fprintf (stdout, "UNINSTALLED %s\n", pack->name);				
			}
		} else {
			if (arg_no_pct==0) {
				if (strcasecmp (title, "Installing")==0) {					
					fprintf (stdout, "\r");
					fprintf (stdout, _("Installing %s (%d/%d), (%d/%d)b - (%d/%d) = %d%% Done\n"),
						 packname,
						 package_num, num_packages,
						 total_size_completed, total_size,
						 amount, total, 100);
				} else {
					fprintf (stdout, "\r");
					fprintf (stdout, _("Uninstalling %s (%d/%d), (%d/%d)b - (%d/%d) = %d%% Done\n"),
						 packname,
						 package_num, num_packages,
						 total_size_completed, total_size,
						 amount, total, 100);
					
				}
			} else {
				fprintf (stdout, _("Done\n"));
			}
		}

	}

	fflush (stdout);
	g_free (packname);
}

static void
download_failed (EazelInstallCallback *service, 
		 const PackageData *pack,
		 gpointer unused)
{
	g_assert (pack->name != NULL);

	cli_result = 1;

	if (!arg_machine) {
		fprintf (stdout, _("Download of %s FAILED\n"), pack->name);
	}
}

static void
something_failed (EazelInstallCallback *service,
		  PackageData *pd,
		  EazelInstallProblem *problem,
		  gboolean uninstall)
{
	char *title;
	GList *stuff = NULL;	

	gtk_object_ref (GTK_OBJECT (pd));

	auto_cont = FALSE;
	
	cli_result = 2;

	if (arg_machine) { return; };
	if (arg_silent) { printf ("\n"); }
	if (uninstall) {
		title = g_strdup_printf (_("Package %s failed to uninstall.\n"), pd->name);
	} else {
		title = g_strdup_printf (_("Package %s failed to install.\n"), pd->name);
	}

	fprintf (stdout, "%s\n", title);
	g_free (title);

	if (arg_debug) {
		GList *list = g_list_prepend (NULL, pd);
		char *out = packagedata_dump_tree (list, 2);
		fprintf (stdout, "%s\n", out);
		g_list_free (list);
		g_free (out);
	}

	if (problem) {
		stuff = eazel_install_problem_tree_to_string (problem, pd, uninstall);
		if (stuff) {
			GList *extra_cases = NULL;
			GList *it;
			for (it = stuff; it; it = g_list_next (it)) {
				/* I18N note: \xB7 is a dot */
				fprintf (stdout, _("\t\xB7 Problem : %s\n"), (char*)(it->data));
			}
			eazel_install_problem_tree_to_case (problem, pd, uninstall, &extra_cases);
			if (extra_cases) {
				stuff = eazel_install_problem_cases_to_string (problem, extra_cases);
				if (stuff) {
					GList *it;
					for (it = stuff; it; it = g_list_next (it)) {
						/* I18N note: \xB7 is a dot */
						fprintf (stdout, _("\t\xB7 Action : %s\n"), (char*)(it->data));
					}
				}
			}
			cases = g_list_concat (cases, extra_cases);
		}
	}
	gtk_object_unref (GTK_OBJECT (pd));
}

/*
  This dumps the entire tree for the failed package.
 */
static void
install_failed (EazelInstallCallback *service,
		PackageData *pd,
		EazelInstallProblem *problem)
{
	something_failed (service, pd, problem, FALSE);
}

static void
uninstall_failed (EazelInstallCallback *service,
		  PackageData *pd,		
		  EazelInstallProblem *problem)
{
	something_failed (service, pd, problem, TRUE);
}

static gboolean
eazel_preflight_check_signal (EazelInstallCallback *service, 
			      EazelInstallCallbackOperation op,
			      const GList *packages,
			      int total_bytes,
			      int total_packages,
			      gpointer unused) 
{	
	const GList *iterator;

	if (cases && (total_packages == 0)) {
		if (arg_machine) { 
			return FALSE; 
		}
		fprintf (stdout, _("Cancelling operation\n"));
		return FALSE;
	}

	if (arg_machine) { 
		return TRUE; 
	}

	if (arg_silent) { printf ("\n"); }
	switch (op) {
	case EazelInstallCallbackOperation_INSTALL:
		fprintf (stdout, _("About to install a total of %d packages, %dKB\n"), 
			 total_packages, total_bytes/1024);
		break;
	case EazelInstallCallbackOperation_UNINSTALL:
		fprintf (stdout, _("About to uninstall a total of %d packages, %dKB\n"), 
			 total_packages, total_bytes/1024);
		break;
	case EazelInstallCallbackOperation_REVERT:
		fprintf (stdout, _("About to revert a total of %d packages, %d KB\n"), 
			 total_packages, total_bytes/1024);
		break;
	}
		
	for (iterator = packages; iterator; iterator = iterator->next) {
		PackageData *pack = (PackageData*)iterator->data;
		if (arg_debug) {
			GList *list = g_list_prepend (NULL, pack);
			char *out = packagedata_dump_tree (list, 2);
			fprintf (stdout, "%s", out);
			g_list_free (list);
			g_free (out);
		} else {
			char *name = packagedata_get_readable_name (pack);
			if (pack->depends) {
				/* I18N note: %s is a package name, \xB7 is a dot */
				printf (_("\t\xB7 %s and it's dependencies\n"), name);
			} else {
				/* I18N note: %s is a package name, \xB7 is a dot */
				printf (_("\t\xB7 %s\n"), name);
			}
			g_free (name);
		}
	}

	return TRUE;
}

static gboolean
eazel_save_transaction_signal (EazelInstallCallback *service, 
			       EazelInstallCallbackOperation op,
			       const GList *packages,
			       gpointer unused) 
{	
	char answer[128];
	gboolean result = FALSE;
	
	/* Disabled for 1.0 */
	return result;

	if (arg_machine) { return TRUE; }

	/* I18N note: the (y/n) is translateable. There is later a 1 character
	   string with the context "y" which is the "yes" indicator.
	   If you eg. translate this to Danish : "Forts�t (j/n) " and
	   translated the "y" later to "j", da_DK users can respond with
	   "j" "ja" "JA" etc. */
	printf (_("Save transaction report ? (y/n) "));
	fflush (stdout);
	if (arg_batch) {			
		fprintf (stdout, "%s\n", arg_batch);
		strcpy (answer, arg_batch);
	} else {
		fgets (answer, 10, stdin);
	}
	/* I18N note: y is the letter for the word Yes. This is
	   used in the response for a yes/no questions. Your translation
	   must be 1 character only. */
	if (strncasecmp (answer, _("y"), 1)==0) {
		fflush (stdout);
		result = TRUE;
	}

	return result;
}

static void
dep_check (EazelInstallCallback *service,
	   const PackageData *package,
	   const PackageData *needs_package,
	   gpointer unused) 
{
	char *pack, *needs;
	
	if (arg_machine) { return; }

	pack = packagedata_get_readable_name (package);
	needs = packagedata_get_readable_name (needs_package);
	if (!arg_silent) {
		printf (_("Dependency : %s needs %s\n"), pack, needs);
	} else {
		printf ("."); fflush (stdout);
	}
	g_free (pack);
	g_free (needs);
}

static void
md5_check_failed (EazelInstallCallback *service,
		  const PackageData *package,
		  const char *actual_md5,
		  gpointer unused) 
{
	cli_result = 3;
	if (arg_machine) { return; }
	if (arg_silent) { printf ("\n"); }
	/* I18N note: %s is a package name */
	fprintf (stdout, _("Package %s failed md5 check!\n"), package->name);
	/* I18N note: %s is a 32 bytes hex numbers, \xB7 is a dot */
	fprintf (stdout, _("\t\xB7 server MD5 checksum is %s\n"), package->md5);
	/* I18N note: %s is a 32 bytes hex numbers, \xB7 is a dot */
	fprintf (stdout, _("\t\xB7 actual MD5 checksum is %s\n"), actual_md5);
}

static PackageData*
create_package (char *name) 
{
	struct utsname buf;
	PackageData *pack;

	uname (&buf);
	pack = packagedata_new ();
	if (arg_file) {
		/* If file starts with /, use filename,
		   if it starts with ~, insert g_get_home_dir,
		   otherwise, insert g_get_current_dir */
		pack->filename = 
			name[0]=='/' ? 
			g_strdup (name) : 
			name[0]=='~' ?
			g_strdup_printf ("%s/%s", g_get_home_dir (), name+1) :
			g_strdup_printf ("%s/%s", g_get_current_dir (), name);
	} else if (arg_provides) {
		pack->provides = g_list_prepend (pack->provides, g_strdup (name));
	} else if (arg_id) {
		pack->eazel_id = g_strdup (name);
	} else if (arg_suite) {
		pack->suite_id = g_strdup (name);
	} else {
		pack->name = g_strdup (name);
	}
	if (arg_version) {
		pack->version = g_strdup (arg_version);
	}
	pack->archtype = g_strdup (buf.machine);
#ifdef ASSUME_ix86_IS_i386
	if (strlen (pack->archtype)==4 && pack->archtype[0]=='i' &&
	    pack->archtype[1]>='3' && pack->archtype[1]<='9' &&
	    pack->archtype[2]=='8' && pack->archtype[3]=='6') {
		g_free (pack->archtype);
		pack->archtype = g_strdup ("i386");
	}
#endif
	pack->distribution = trilobite_get_distribution ();
	pack->toplevel = TRUE;
	
	return pack;
}

static gboolean
delete_files (EazelInstallCallback *service, EazelInstallProblem *problem)
{
	char answer[128];
	gboolean ask_delete = TRUE;
	gboolean result = TRUE;
	
	if (arg_machine) { return TRUE; }

	if ((auto_cont && cases) || cases ) {
		gboolean cont = FALSE;

		if (auto_cont == FALSE) {
			/* I18N note: the (y/n) is translateable. There is later a 1 character
			   string with the context "y" which is the "yes" indicator.
			   If you eg. translate this to Danish : "Forts�t (j/n " and
			   translated the "y" later to "j", da_DK users can respond with
			   "j" "ja" "JA" etc. */
			printf (_("Continue? (y/n) "));
			fflush (stdout);
			if (arg_batch) {			
				fprintf (stdout, "%s\n", arg_batch);
				strcpy (answer, arg_batch);
			} else {
				fgets (answer, 10, stdin);
			}
			/* I18N note: y is the letter for the word Yes. This is
			   used in the response for a yes/no questions. Your translation
			   must be 1 character only. */
			if (strncasecmp (answer, _("y"), 1)==0) {
				fflush (stdout);
				cont = TRUE;
			}

		} else {
			cont = TRUE;
		}
		if (cont) {
			auto_cont = TRUE;
			eazel_install_problem_handle_cases (problem, 
							    service, 
							    &cases, 
							    &categories, 
							    NULL,
							    arg_root);
			ask_delete = FALSE;
			result = FALSE;
		} else {
			eazel_install_problem_case_list_destroy (cases);
			cases = NULL;
		}
	} 

	if (downloaded_files && !arg_query && !arg_erase && !arg_file && ask_delete) {
		/* I18N note: the (y/n) is translateable. There is later a 1 character
		   string with the context "y" which is the "yes" indicator.
		   If you eg. translate this to Danish : "Forts�t (j/n " and
		   translated the "y" later to "j", da_DK users can respond with
		   "j" "ja" "JA" etc. */
		printf (_("Should I delete the RPM files? (y/n) "));
		fflush (stdout);
		if (arg_batch) {			
			fprintf (stdout, "yes\n");
			strcpy (answer, "yes");
		} else {
			fgets (answer, 10, stdin);
		}
		
		/* I18N note: y is the letter for the word Yes. This is
		   used in the response for a yes/no questions. Your translation
		   must be 1 character only. */
		if (strncasecmp (answer, _("y"), 1)==0) {
			fflush (stdout);
			eazel_install_callback_delete_files (service, &ev);			
		} 
	}

	return result;
}

static void
done (EazelInstallCallback *service,
      gboolean result,
      EazelInstallProblem *problem)
{
	if (cli_result == 0) {
		cli_result = result ? 0 : 1;
	}

	if (!arg_machine) {
		if (result) {
			fprintf (stderr, _("Operation ok\n"));
		} else {
			fprintf (stderr, _("Operation failed\n"));
		}
	}

	if (delete_files (service, problem)) {
		trilobite_main_quit ();
	}
}

static char *
get_password_dude (TrilobiteRootClient *root_client, const char *prompt, void *user_data)
{
	char * real_prompt;
	char * passwd;

	real_prompt = g_strdup_printf ("%s: ", prompt);
	passwd = getpass (real_prompt);
	g_free (real_prompt);

	return g_strdup (passwd);
}

static TrilobiteRootClient *
set_root_client (BonoboObjectClient *service)
{
	TrilobiteRootClient *root_client;

	if (bonobo_object_client_has_interface (service, "IDL:Trilobite/PasswordQuery:1.0", &ev)) {
		root_client = trilobite_root_client_new ();
		gtk_signal_connect (GTK_OBJECT (root_client), "need_password", GTK_SIGNAL_FUNC (get_password_dude),
				    NULL);

		if (! trilobite_root_client_attach (root_client, service)) {
			fprintf (stderr, "*** unable to attach root client to Trilobite/PasswordQuery!");
		}

		return root_client;
	} else {
		fprintf (stderr, "*** Object does not support IDL:Trilobite/PasswordQuery:1.0");
		return NULL;
	}
}

int main(int argc, char *argv[]) {
	poptContext ctxt;
	GList *packages;
	char *str;
	GList *strs;
	EazelInstallCallback *cb;		
	EazelInstallProblem *problem = NULL;

	CORBA_exception_init (&ev);

	strs = NULL;

	/* Seems that bonobo_main doens't like
	   not having gnome_init called, dies in a
	   X call, yech */

#ifdef ENABLE_NLS /* sadly we need this ifdef because otherwise the following get empty statement warnings */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);
#endif

#if 0
	gnomelib_register_popt_table (oaf_popt_options, oaf_get_popt_table_name ());
	orb = oaf_init (argc, argv);
	gnome_init_with_popt_table ("Eazel Install", "1.0", argc, argv, options, 0, &ctxt);
	if (!bonobo_init (NULL, NULL, NULL)) {
		g_error ("Could not init bonobo");
	}
#else
	trilobite_init ("Eazel Install", "1.0", NULL, options, argc, argv);
	ctxt = trilobite_get_popt_context ();
#endif

	packages = NULL;
	categories = NULL;
	/* If there are more args, get them and parse them as packages */
	while ((str = poptGetArg (ctxt)) != NULL) {
		packages = g_list_prepend (packages, create_package (str));
		strs = g_list_prepend (strs, g_strdup (str));
	}
	if (packages) {
		CategoryData *category;
		category = categorydata_new ();
		category->name = g_strdup ("files from commandline");
		category->packages = packages;
		categories = g_list_prepend (NULL, category);		
	}

	bonobo_activate ();

	cb = eazel_install_callback_new ();
	problem = eazel_install_problem_new (); 
	gtk_object_ref (GTK_OBJECT (problem));

	if (arg_delay) {
		sleep (10);
	}

	set_parameters_from_command_line (eazel_install_callback_corba_objref (cb));
	set_root_client (eazel_install_callback_bonobo (cb));
	
	/* Set up signal connections */
	gtk_signal_connect (GTK_OBJECT (cb), "file_conflict_check", 
			    GTK_SIGNAL_FUNC (eazel_file_conflict_check_signal), 
			    NULL);
	gtk_signal_connect (GTK_OBJECT (cb), "file_uniqueness_check", 
			    GTK_SIGNAL_FUNC (eazel_file_uniqueness_check_signal), 
			    NULL);
	gtk_signal_connect (GTK_OBJECT (cb), "feature_consistency_check", 
			    GTK_SIGNAL_FUNC (eazel_feature_consistency_check_signal), 
			    NULL);

	gtk_signal_connect (GTK_OBJECT (cb), "download_progress", 
			    GTK_SIGNAL_FUNC (eazel_download_progress_signal), 
			    NULL);
	gtk_signal_connect (GTK_OBJECT (cb), "preflight_check", 
			    GTK_SIGNAL_FUNC (eazel_preflight_check_signal), 
			    NULL);
	gtk_signal_connect (GTK_OBJECT (cb), "save_transaction", 
			    GTK_SIGNAL_FUNC (eazel_save_transaction_signal), 
			    NULL);
	gtk_signal_connect (GTK_OBJECT (cb), "install_progress", 
			    GTK_SIGNAL_FUNC (eazel_install_progress_signal), 
			    _("Installing"));
	gtk_signal_connect (GTK_OBJECT (cb), "md5_check_failed", 
			    GTK_SIGNAL_FUNC (md5_check_failed), 
			    "");
	gtk_signal_connect (GTK_OBJECT (cb), "install_failed", 
			    GTK_SIGNAL_FUNC (install_failed), 
			    problem);
	gtk_signal_connect (GTK_OBJECT (cb), "uninstall_progress", 
			    GTK_SIGNAL_FUNC (eazel_install_progress_signal), 
			    _("Uninstalling"));
	gtk_signal_connect (GTK_OBJECT (cb), "uninstall_failed", 
			    GTK_SIGNAL_FUNC (uninstall_failed), 
			    problem);
	gtk_signal_connect (GTK_OBJECT (cb), "download_failed", 
			    GTK_SIGNAL_FUNC (download_failed), 
			    NULL);
	gtk_signal_connect (GTK_OBJECT (cb), "dependency_check", 
			    GTK_SIGNAL_FUNC (dep_check), 
			    NULL);
	gtk_signal_connect (GTK_OBJECT (cb), "done", 
			    GTK_SIGNAL_FUNC (done), 
			    problem);

	if (arg_erase + arg_query + arg_revert > 1) {
		g_error (_("Only one operation at a time please."));
	}

	if (arg_erase) {
		if (categories == NULL) {
			fprintf (stderr, _("%s: --help for usage\n"), argv[0]);
			cli_result = 1;
		} else {
			eazel_install_callback_uninstall_packages (cb, categories, arg_root, &ev);
		}
	} else if (arg_query) {
		GList *iterator;
		if (strs == NULL) {
			fprintf (stderr, _("%s: --help for usage\n"), argv[0]);
			cli_result = 1;
		} else for (iterator = strs; iterator; iterator = iterator->next) {
			GList *matched_packages;
			GList *match_it;
			matched_packages = eazel_install_callback_simple_query (cb, 
										(char*)iterator->data, 
										NULL, 
										&ev);
			for (match_it = matched_packages; match_it; match_it = match_it->next) {
				PackageData *p;
				p = (PackageData*)match_it->data;
				if (arg_verbose) {
					char *tmp;
					GList *provide_iterator;
					tmp = trilobite_get_distribution_name (p->distribution, TRUE, FALSE);
					fprintf (stdout, _("Name         : %s\n"), p->name?p->name:"?"); 
					fprintf (stdout, _("Version      : %s\n"), p->version?p->version:"?");
					fprintf (stdout, _("Minor        : %s\n"), p->minor?p->minor:"?");
					
					fprintf (stdout, _("Size         : %d\n"), p->bytesize);
					fprintf (stdout, _("Arch         : %s\n"), p->archtype?p->archtype:"?");
					fprintf (stdout, _("Distribution : %s\n"), tmp?tmp:"?");
					fprintf (stdout, _("Description  : %s\n"), 
						 p->description?p->description:"?");
					fprintf (stdout, _("Install root : %s\n"), 
						 p->install_root?p->install_root:"?");
					if (p->provides) {
						fprintf (stdout, _("Provides     : \n"));
						for (provide_iterator = p->provides; provide_iterator; 
						     provide_iterator = g_list_next (provide_iterator)) {
							fprintf (stdout, "\t%s\n", 
								 (char*)provide_iterator->data);
						}
					}
				} else {
					fprintf (stdout, "%s %s %50.50s\n", p->name, p->version, p->description);
				}
			}
			cli_result = 0;
		}
	} else if (arg_revert) {
		GList *iterator;
		if (strs == NULL) {
			fprintf (stderr, _("%s: --help for usage\n"), argv[0]);
			cli_result = 1;
		} else for (iterator = strs; iterator; iterator = iterator->next) {
			eazel_install_callback_revert_transaction (cb, (char*)iterator->data, arg_root, &ev);
		}
	} else {
		if (categories == NULL) {
			fprintf (stderr, _("%s: --help for usage\n"), argv[0]);
			cli_result = 1;
		} else {
			eazel_install_callback_install_packages (cb, categories, arg_root, &ev);
		}
	}
	
	if (!cli_result && !arg_query) {
		trilobite_main ();
	}

	categorydata_list_destroy (categories);

	eazel_install_callback_unref (GTK_OBJECT (cb));
	gtk_object_unref (GTK_OBJECT (problem));
	/* Corba cleanup */
	CORBA_exception_free (&ev);

	if (arg_debug) {
		printf (_("exit code %d\n"), cli_result);
	}
       
	return cli_result;
};
