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
 * Authors: J Shane Culpepper
 */

#ifndef NAUTILUS_SUMMARY_VIEW_PRIVATE_H
#define NAUTILUS_SUMMARY_VIEW_PRIVATE_H


#include <gnome.h>

#define DEFAULT_SUMMARY_BACKGROUND_COLOR_SPEC	"rgb:FFFF/FFFF/FFFF"
#define DEFAULT_SUMMARY_BACKGROUND_COLOR_RGB	NAUTILUS_RGB_COLOR_WHITE
#define DEFAULT_SUMMARY_TEXT_COLOR_RGB		NAUTILUS_RGB_COLOR_BLACK

#define URL_REDIRECT_TABLE_HOME			"eazel-services://anonymous/services/urls"
#define URL_REDIRECT_TABLE_HOME_2		"eazel-services:/services/urls"
#define SUMMARY_CONFIG_XML			"eazel-services://anonymous/services"
#define SUMMARY_CONFIG_XML_2			"eazel-services:/services"

#define SUMMARY_TERMS_OF_USE_URI		"eazel-services://anonymous/aboutus/terms_of_use"
#define SUMMARY_PRIVACY_STATEMENT_URI		"eazel-services://anonymous/aboutus/privacy"
#define SUMMARY_CHANGE_PWD_FORM			"eazel-services://anonymous/account/login/lost_pwd_form"

#define SUMMARY_XML_KEY				"eazel_summary_xml"
#define URL_REDIRECT_TABLE			"eazel_url_table_xml"
#define REGISTER_KEY				"eazel_service_register"
#define PREFERENCES_KEY				"eazel_service_account_maintenance"

#define GOTO_BUTTON_LABEL			_("Go There!")
#define SOFTCAT_GOTO_BUTTON_LABEL		_("More Info!")
#define INSTALL_GOTO_BUTTON_LABEL		_("Install Me!")

#define MAX_IMAGE_WIDTH				50
#define MAX_IMAGE_HEIGHT			50

#define FOOTER_REGISTER_OR_PREFERENCES		0
#define FOOTER_LOGIN_OR_LOGOUT			1
#define FOOTER_TERMS_OF_USER			2
#define FOOTER_PRIVACY_STATEMENT		3


enum {
	LOGIN_DIALOG_NAME_ROW,
	LOGIN_DIALOG_PASSWORD_ROW,
	LOGIN_DIALOG_ROW_COUNT
};

enum {
	LOGIN_DIALOG_REGISTER_BUTTON_INDEX,
	LOGIN_DIALOG_OK_BUTTON_INDEX,
	LOGIN_DIALOG_CANCEL_BUTTON
};

typedef struct _ServicesButtonCallbackData ServicesButtonCallbackData;

typedef enum {
	Pending_None,
	Pending_Login,
} SummaryPendingOperationType;

typedef enum {
	initial,
	retry
} SummaryLoginAttemptType;


struct _ServicesButtonCallbackData {
	NautilusView    *nautilus_view;
	char            *uri;
};

/* A NautilusContentView's private information. */
struct _NautilusSummaryViewDetails {
	char				*uri;
	NautilusView			*nautilus_view;
	SummaryData			*xml_data;

	/* Parent form and title */
	GtkWidget			*form;
	GtkWidget			*form_title;

	/* Login State */
	char				*user_name;
	volatile gboolean		logged_in;
	GtkWidget			*caption_table;
	SummaryLoginAttemptType		current_attempt;
	int				attempt_number;

	/* EazelProxy -- for logging in/logging out */
	EazelProxy_UserControl		user_control;
	SummaryPendingOperationType	pending_operation;
	EazelProxy_AuthnCallback	authn_callback;

	/* Services control panel */
	int				current_service_row;
	GtkWidget			*services_row;
	GtkWidget			*services_icon_container;
	GtkWidget			*services_icon_widget;
	char				*services_icon_name;
	GtkWidget			*services_description_header_widget;
	char				*services_description_header;
	GtkWidget			*services_description_body_widget;
	char				*services_description_body;
	GtkWidget			*services_button_container;
	GtkWidget			*services_goto_button;
	GtkWidget			*services_goto_label_widget;
	char				*services_goto_label;
	char				*services_redirects[500];
	gboolean			services_button_enabled;
	GtkWidget			*services_notebook;

	/* Login Frame Widgets */
	GnomeDialog			*login_dialog;
	GtkWidget			*username_label;
	GtkWidget			*password_label;
	GtkWidget			*username_entry;
	GtkWidget			*password_entry;
	/* Buttons available if user is not logged in */
	GtkWidget			*login_button;
	GtkWidget			*login_label;
	GtkWidget			*register_button;
	GtkWidget			*register_label;
	/* Buttons available if user is logged in */
	GtkWidget			*preferences_button;
	GtkWidget			*preferences_label;
	GtkWidget			*logout_button;
	GtkWidget			*logout_label;

	/* Eazel news panel */
	int				current_news_row;
	gboolean			news_has_data;
	GtkWidget			*service_news_row;
	GtkWidget			*news_icon_container;
	GtkWidget			*news_icon_widget;
	char				*news_icon_name;
	GtkWidget			*news_date_widget;
	char				*news_date;
	GtkWidget			*news_description_header_widget;
	char				*news_description_header;
	GtkWidget			*news_description_body_widget;
	char				*news_description_body;

	/* Update control panel */
	int				current_update_row;
	gboolean			updates_has_data;
	GtkWidget			*updates_row;
	GtkWidget			*update_icon_container;
	GtkWidget			*update_icon_widget;
	char				*update_icon_name;
	GtkWidget			*update_description_header_widget;
	char				*update_description_header;
	GtkWidget			*update_description_body_widget;
	char				*update_description_body;
	GtkWidget			*update_description_version_widget;
	char				*update_description_version;
	GtkWidget			*update_button_container;
	GtkWidget			*update_goto_button;
	GtkWidget			*update_goto_label_widget;
	char				*update_goto_label;
	char				*update_redirects[500];
	GtkWidget			*update_softcat_goto_button;
	GtkWidget			*update_softcat_goto_label_widget;
	char				*update_softcat_goto_label;
	char				*update_softcat_redirects[500];
	GtkWidget			*updates_notebook;

};


#endif /* NAUTILUS_SUMMARY_VIEW_PRIVATE_H */

