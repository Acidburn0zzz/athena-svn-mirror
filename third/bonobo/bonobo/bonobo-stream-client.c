/**
 * gnome-stream-client.c: Helper routines to access a Bonobo_Stream CORBA object
 *
 * Authors:
 *   Nat Friedman    (nat@nat.org)
 *   Miguel de Icaza (miguel@kernel.org).
 *   Michael Meekss  (michael@helixcode.com)
 *
 * Copyright 1999,2000 Helix Code, Inc.
 */
#include <config.h>

#include <bonobo/Bonobo.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo-stream-client.h>

#define CORBA_BLOCK_SIZE 65536

/**
 * bonobo_stream_client_write:
 * @stream: A CORBA Object reference to a Bonobo_Stream
 * @buffer: the buffer to write
 * @size: number of bytes to write
 * @ev: a CORBA environment to return status information.
 *
 * This is a helper routine to write @size bytes from @buffer to the @stream
 * It will continue to write bytes until a fatal error occurs.
 *
 */
void
bonobo_stream_client_write (const Bonobo_Stream stream,
			    const void *buffer, const size_t size,
			    CORBA_Environment *ev)
{
	Bonobo_Stream_iobuf *buf;
	size_t               pos;
	guint8              *mem = (guint8 *)buffer;

	if (size == 0)
		return;

	g_return_if_fail (ev != NULL);
	
	if (buffer == NULL || stream == CORBA_OBJECT_NIL)
		goto bad_param;

	buf = Bonobo_Stream_iobuf__alloc ();
	if (!buf)
		goto alloc_error;

	for (pos = 0; pos < size;) {
		buf->_buffer = (mem + pos);
		buf->_length = (pos + CORBA_BLOCK_SIZE < size) ?
			CORBA_BLOCK_SIZE : size - pos;
		buf->_maximum = buf->_length;

		Bonobo_Stream_write (stream, buf, ev);
		if (BONOBO_EX (ev)) {
			CORBA_free (buf);
			return;
		}
		pos += buf->_length;
	}

	CORBA_free (buf);
	return;

 alloc_error:
	CORBA_exception_set_system (ev, ex_CORBA_NO_MEMORY,
				    CORBA_COMPLETED_NO);
	return;

 bad_param:
	CORBA_exception_set_system (ev, ex_CORBA_BAD_PARAM,
				    CORBA_COMPLETED_NO);
	return;
}

/**
 * bonobo_stream_client_write_string:
 * @stream: A CORBA object reference to a #Bonobo_Stream.
 * @str: A string.
 * @terminate: Whether or not to write the \0 at the end of the
 * string.
 * @ev: A pointer to a #CORBA_Environment
 *
 * This is a helper routine to write the string in @str to @stream.
 * If @terminate is TRUE, a NULL character will be written out at the
 * end of the string.  This function will not return until the entire
 * string has been written out, unless an exception is raised.  See
 * also bonobo_stream_client_write(). Continues writing until finished
 * or a fatal exception occurs.
 *
 */
void
bonobo_stream_client_write_string (const Bonobo_Stream stream, const char *str,
				   gboolean terminate, CORBA_Environment *ev)
{
	size_t total_length;

	g_return_if_fail (ev != NULL);
	g_return_if_fail (str != NULL);

	total_length = strlen (str) + (terminate ? 1 : 0);

	bonobo_stream_client_write (stream, str, total_length, ev);
}

/**
 * bonobo_stream_client_printf:
 * @stream: A CORBA object reference to a #Bonobo_Stream.
 * @terminate: Whether or not to null-terminate the string when it is
 * written out to the stream.
 * @ev: A CORBA_Environment pointer.
 * @fmt: The printf format string.
 *
 * Processes @fmt and the arguments which follow it to produce a
 * string.  Writes this string out to @stream.  This function will not
 * return until the entire string is written out, unless an exception
 * is raised.  See also bonobo_stream_client_write_string() and
 * bonobo_stream_client_write().
 */
void
bonobo_stream_client_printf (const Bonobo_Stream stream, const gboolean terminate,
			     CORBA_Environment *ev, const char *fmt, ...)
{
	va_list      args;
	char        *str;

	g_return_if_fail (fmt != NULL);

	va_start (args, fmt);
	str = g_strdup_vprintf (fmt, args);
	va_end (args);

	bonobo_stream_client_write_string (stream, str, terminate, ev);

	g_free (str);
}

/**
 * bonobo_stream_client_read_string:
 * @stream: The #Bonobo_Stream from which the string will be read.
 * @str: The string pointer in which the string will be stored.
 * @ev: A pointer to a #CORBA_Environment.
 *
 * Reads a NULL-terminated string from @stream and stores it in a
 * newly-allocated string in @str.
 *
 * Returns: The number of bytes read, or -1 if an error occurs.
 * If an exception occurs, @ev will contain the exception.
 */
CORBA_long
bonobo_stream_client_read_string (const Bonobo_Stream stream, char **str,
				  CORBA_Environment *ev)
{
	Bonobo_Stream_iobuf *buffer;
	GString             *gstr;
	gboolean             all;

	gstr = g_string_sized_new (16);

	for (all = FALSE; !all; ) {

		Bonobo_Stream_read (stream, 1,
				    &buffer, ev);

		if (BONOBO_EX (ev))
			break;

		else if (buffer->_length == 0 ||
			 buffer->_buffer [0] == '\0')
			all = TRUE;
		
		else {
			g_string_append_c (gstr, buffer->_buffer [0]);
			CORBA_free (buffer);
		}
	}

	if (BONOBO_EX (ev)) {
		*str = NULL;
		g_string_free (gstr, TRUE);

		return -1;
	} else {
		CORBA_long l;

		l    = gstr->len;
		*str = gstr->str;
		g_string_free (gstr, FALSE);

		return l;
	}
}
