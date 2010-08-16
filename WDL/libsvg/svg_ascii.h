/* svg_ascii.c - Like the C library calls, but always ASCII, (ie. not locale-sensitive)
 *
 * Grabbed (2002-10-08) by Carl Worth from:
 *
 * GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef SVG_ASCII_H
#define SVG_ASCII_H

#include <stdio.h>
#include "stdint.h"

/* Functions like the ones in <ctype.h> that are not affected by locale. */
typedef enum {
  SVG_ASCII_ALNUM  = 1 << 0,
  SVG_ASCII_ALPHA  = 1 << 1,
  SVG_ASCII_CNTRL  = 1 << 2,
  SVG_ASCII_DIGIT  = 1 << 3,
  SVG_ASCII_GRAPH  = 1 << 4,
  SVG_ASCII_LOWER  = 1 << 5,
  SVG_ASCII_PRINT  = 1 << 6,
  SVG_ASCII_PUNCT  = 1 << 7,
  SVG_ASCII_SPACE  = 1 << 8,
  SVG_ASCII_UPPER  = 1 << 9,
  SVG_ASCII_XDIGIT = 1 << 10
} svg_ascii_type_t;

extern const uint16_t * const svg_ascii_table;

#define _svg_ascii_isalnum(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_ALNUM) != 0)

#define _svg_ascii_isalpha(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_ALPHA) != 0)

#define _svg_ascii_iscntrl(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_CNTRL) != 0)

#define _svg_ascii_isdigit(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_DIGIT) != 0)

#define _svg_ascii_isgraph(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_GRAPH) != 0)

#define _svg_ascii_islower(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_LOWER) != 0)

#define _svg_ascii_isprint(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_PRINT) != 0)

#define _svg_ascii_ispunct(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_PUNCT) != 0)

#define _svg_ascii_isspace(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_SPACE) != 0)

#define _svg_ascii_isupper(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_UPPER) != 0)

#define _svg_ascii_isxdigit(c) \
  ((svg_ascii_table[(unsigned char) (c)] & SVG_ASCII_XDIGIT) != 0)

char
_svg_ascii_tolower (char c);

char
_svg_ascii_toupper (char c);

int
_svg_ascii_digit_value (char c);

int
_svg_ascii_xdigit_value (char c);

double
_svg_ascii_strtod (const char	*nptr,
		   const char **endptr);

int
_svg_ascii_strcasecmp (const char *s1,
		       const char *s2);

int
_svg_ascii_strncasecmp (const char *s1,
			const char *s2,
			size_t      n);

#endif /* SVG_ASCII_H */
