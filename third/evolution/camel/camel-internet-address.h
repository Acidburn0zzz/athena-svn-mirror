/*
 *  Copyright (C) 2000 Ximian Inc.
 *
 *  Authors: Michael Zucchi <notzed@ximian.com>
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
 */

#ifndef _CAMEL_INTERNET_ADDRESS_H
#define _CAMEL_INTERNET_ADDRESS_H

#include <camel/camel-address.h>

#define CAMEL_INTERNET_ADDRESS(obj)         CAMEL_CHECK_CAST (obj, camel_internet_address_get_type (), CamelInternetAddress)
#define CAMEL_INTERNET_ADDRESS_CLASS(klass) CAMEL_CHECK_CLASS_CAST (klass, camel_internet_address_get_type (), CamelInternetAddressClass)
#define CAMEL_IS_INTERNET_ADDRESS(obj)      CAMEL_CHECK_TYPE (obj, camel_internet_address_get_type ())

typedef struct _CamelInternetAddressClass CamelInternetAddressClass;

struct _CamelInternetAddress {
	CamelAddress parent;

	struct _CamelInternetAddressPrivate *priv;
};

struct _CamelInternetAddressClass {
	CamelAddressClass parent_class;
};

guint			camel_internet_address_get_type	(void);
CamelInternetAddress   *camel_internet_address_new	(void);

int			camel_internet_address_add	(CamelInternetAddress *, const char *, const char *);
gboolean		camel_internet_address_get	(const CamelInternetAddress *, int, const char **, const char **);

int			camel_internet_address_find_name(CamelInternetAddress *, const char *, const char **);
int			camel_internet_address_find_address(CamelInternetAddress *, const char *, const char **);

/* utility functions, for network/display formatting */
char *			camel_internet_address_encode_address(int *len, const char *name, const char *addr);
char *			camel_internet_address_format_address(const char *real, const char *addr);

#endif /* ! _CAMEL_INTERNET_ADDRESS_H */
