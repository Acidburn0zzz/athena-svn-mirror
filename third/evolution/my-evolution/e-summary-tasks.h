/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* e-summary-tasks.h
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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
 * Author: Iain Holmes
 */

#ifndef __E_SUMMARY_TASKS_H__
#define __E_SUMMARY_TASKS_H__

#include "e-summary-type.h"

typedef struct _ESummaryTasks ESummaryTasks;

const char *e_summary_tasks_get_html (ESummary *summary);
void e_summary_tasks_init (ESummary *summary);
void e_summary_tasks_reconfigure (ESummary *summary);
void e_summary_tasks_free (ESummary *summary);

#endif
