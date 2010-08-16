/* svg_ascii.c - Like the C library calls, but always ASCII, (ie. not locale-sensitive)
 *
 * (2002-10-08) Lifted by Carl Worth from:
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

/*
 * MT safe
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "svg_ascii.h"

#include <stdlib.h>
#include <locale.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

static const uint16_t svg_ascii_table_data[256] = {
  0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
  0x004, 0x104, 0x104, 0x004, 0x104, 0x104, 0x004, 0x004,
  0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
  0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004, 0x004,
  0x140, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459, 0x459,
  0x459, 0x459, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x0d0, 0x653, 0x653, 0x653, 0x653, 0x653, 0x653, 0x253,
  0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
  0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253, 0x253,
  0x253, 0x253, 0x253, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x0d0,
  0x0d0, 0x473, 0x473, 0x473, 0x473, 0x473, 0x473, 0x073,
  0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
  0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073, 0x073,
  0x073, 0x073, 0x073, 0x0d0, 0x0d0, 0x0d0, 0x0d0, 0x004
  /* the upper 128 are all zeroes */
};

const uint16_t * const svg_ascii_table = svg_ascii_table_data;

/**
 * svg_ascii_strtod:
 * @nptr:    the string to convert to a numeric value.
 * @endptr:  if non-%NULL, it returns the character after
 *           the last character used in the conversion.
 * 
 * Converts a string to a #double value.
 * This function behaves like the standard strtod() function
 * does in the C locale. It does this without actually
 * changing the current locale, since that would not be
 * thread-safe.
 *
 * This function is typically used when reading configuration files or
 * other non-user input that should be locale independent.  To handle
 * input from the user you should normally use the locale-sensitive
 * system strtod() function.
 *
 * If the correct value would cause overflow, plus or minus %HUGE_VAL
 * is returned (according to the sign of the value), and %ERANGE is
 * stored in %errno. If the correct value would cause underflow,
 * zero is returned and %ERANGE is stored in %errno.
 * 
 * This function resets %errno before calling strtod() so that
 * you can reliably detect overflow and underflow.
 *
 * Return value: the #double value.
 **/
double
_svg_ascii_strtod (const char  *nptr,
		   const char **endptr)
{
  char *fail_pos;
  double val;
  struct lconv *locale_data;
  const char *decimal_point;
  int decimal_point_len;
  const char *p, *decimal_point_pos;
  const char *end = NULL; /* Silence gcc */

  if (nptr == NULL)
      return 0;

  fail_pos = NULL;

  locale_data = localeconv ();
  decimal_point = locale_data->decimal_point;
  decimal_point_len = strlen (decimal_point);
  
  decimal_point_pos = NULL;
  if (decimal_point[0] != '.' ||
      decimal_point[1] != 0)
    {
      p = nptr;
      /* Skip leading space */
      while (_svg_ascii_isspace (*p))
	p++;
      
      /* Skip leading optional sign */
      if (*p == '+' || *p == '-')
	p++;
      
      if (p[0] == '0' &&
	  (p[1] == 'x' || p[1] == 'X'))
	{
	  p += 2;
	  /* HEX - find the (optional) decimal point */
	  
	  while (_svg_ascii_isxdigit (*p))
	    p++;
	  
	  if (*p == '.')
	    {
	      decimal_point_pos = p++;
	      
	      while (_svg_ascii_isxdigit (*p))
		p++;
	      
	      if (*p == 'p' || *p == 'P')
		p++;
	      if (*p == '+' || *p == '-')
		p++;
	      while (_svg_ascii_isdigit (*p))
		p++;
	      end = p;
	    }
	}
      else
	{
	  while (_svg_ascii_isdigit (*p))
	    p++;
	  
	  if (*p == '.')
	    {
	      decimal_point_pos = p++;
	      
	      while (_svg_ascii_isdigit (*p))
		p++;
	      
	      if (*p == 'e' || *p == 'E')
		p++;
	      if (*p == '+' || *p == '-')
		p++;
	      while (_svg_ascii_isdigit (*p))
		p++;
	      end = p;
	    }
	}
      /* For the other cases, we need not convert the decimal point */
    }

  /* Set errno to zero, so that we can distinguish zero results
     and underflows */
  errno = 0;
  
  if (decimal_point_pos)
    {
      char *copy, *c;

      /* We need to convert the '.' to the locale specific decimal point */
      copy = malloc (end - nptr + 1 + decimal_point_len);
      
      c = copy;
      memcpy (c, nptr, decimal_point_pos - nptr);
      c += decimal_point_pos - nptr;
      memcpy (c, decimal_point, decimal_point_len);
      c += decimal_point_len;
      memcpy (c, decimal_point_pos + 1, end - (decimal_point_pos + 1));
      c += end - (decimal_point_pos + 1);
      *c = 0;

      val = strtod (copy, &fail_pos);

      if (fail_pos)
	{
	  if (fail_pos > decimal_point_pos)
	    fail_pos = (char *)nptr + (fail_pos - copy) - (decimal_point_len - 1);
	  else
	    fail_pos = (char *)nptr + (fail_pos - copy);
	}
      
      free (copy);
	  
    }
  else
    val = strtod (nptr, &fail_pos);

  if (endptr)
    *endptr = fail_pos;
  
  return val;
}

/**
 * _svg_ascii_tolower:
 * @c: any character.
 * 
 * Convert a character to ASCII lower case.
 *
 * Unlike the standard C library tolower() function, this only
 * recognizes standard ASCII letters and ignores the locale, returning
 * all non-ASCII characters unchanged, even if they are lower case
 * letters in a particular character set. Also unlike the standard
 * library function, this takes and returns a char, not an int, so
 * don't call it on %EOF but no need to worry about casting to #unsigned char
 * before passing a possibly non-ASCII character in.
 * 
 * Return value: the result of converting @c to lower case.
 *               If @c is not an ASCII upper case letter,
 *               @c is returned unchanged.
 **/
char
_svg_ascii_tolower (char c)
{
  return _svg_ascii_isupper (c) ? c - 'A' + 'a' : c;
}

/**
 * _svg_ascii_toupper:
 * @c: any character.
 * 
 * Convert a character to ASCII upper case.
 *
 * Unlike the standard C library toupper() function, this only
 * recognizes standard ASCII letters and ignores the locale, returning
 * all non-ASCII characters unchanged, even if they are upper case
 * letters in a particular character set. Also unlike the standard
 * library function, this takes and returns a char, not an int, so
 * don't call it on %EOF but no need to worry about casting to #unsigned char
 * before passing a possibly non-ASCII character in.
 * 
 * Return value: the result of converting @c to upper case.
 *               If @c is not an ASCII lower case letter,
 *               @c is returned unchanged.
 **/
char
_svg_ascii_toupper (char c)
{
  return _svg_ascii_islower (c) ? c - 'a' + 'A' : c;
}

/**
 * _svg_ascii_digit_value:
 * @c: an ASCII character.
 *
 * Determines the numeric value of a character as a decimal
 * digit. Differs from svg_unichar_digit_value() because it takes
 * a char, so there's no worry about sign extension if characters
 * are signed.
 *
 * Return value: If @c is a decimal digit (according to
 * _svg_ascii_isdigit()), its numeric value. Otherwise, -1.
 **/
int
_svg_ascii_digit_value (char c)
{
  if (_svg_ascii_isdigit (c))
    return c - '0';
  return -1;
}

/**
 * _svg_ascii_xdigit_value:
 * @c: an ASCII character.
 *
 * Determines the numeric value of a character as a hexidecimal
 * digit. Differs from svg_unichar_xdigit_value() because it takes
 * a char, so there's no worry about sign extension if characters
 * are signed.
 *
 * Return value: If @c is a hex digit (according to
 * _svg_ascii_isxdigit()), its numeric value. Otherwise, -1.
 **/
int
_svg_ascii_xdigit_value (char c)
{
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  return _svg_ascii_digit_value (c);
}

/**
 * _svg_ascii_strcasecmp:
 * @s1: string to compare with @s2.
 * @s2: string to compare with @s1.
 * 
 * Compare two strings, ignoring the case of ASCII characters.
 *
 * Unlike the BSD strcasecmp() function, this only recognizes standard
 * ASCII letters and ignores the locale, treating all non-ASCII
 * characters as if they are not letters.
 * 
 * Return value: an integer less than, equal to, or greater than
 *               zero if @s1 is found, respectively, to be less than,
 *               to match, or to be greater than @s2.
 **/
int
_svg_ascii_strcasecmp (const char *s1,
		       const char *s2)
{
  int c1, c2;

  if (s1 == NULL || s2 == NULL)
      return 0;

  while (*s1 && *s2)
    {
      c1 = (int)(unsigned char) _svg_ascii_tolower (*s1);
      c2 = (int)(unsigned char) _svg_ascii_tolower (*s2);
      if (c1 != c2)
	return (c1 - c2);
      s1++; s2++;
    }

  return (((int)(unsigned char) *s1) - ((int)(unsigned char) *s2));
}

/**
 * _svg_ascii_strncasecmp:
 * @s1: string to compare with @s2.
 * @s2: string to compare with @s1.
 * @n:  number of characters to compare.
 * 
 * Compare @s1 and @s2, ignoring the case of ASCII characters and any
 * characters after the first @n in each string.
 *
 * Unlike the BSD strcasecmp() function, this only recognizes standard
 * ASCII letters and ignores the locale, treating all non-ASCII
 * characters as if they are not letters.
 * 
 * Return value: an integer less than, equal to, or greater than zero
 *               if the first @n bytes of @s1 is found, respectively,
 *               to be less than, to match, or to be greater than the
 *               first @n bytes of @s2.
 **/
int
_svg_ascii_strncasecmp (const char *s1,
			const char *s2,
			size_t	   n)
{
  int c1, c2;

  if (s1 == NULL || s2 == NULL)
      return 0;

  while (n && *s1 && *s2)
    {
      n -= 1;
      c1 = (int)(unsigned char) _svg_ascii_tolower (*s1);
      c2 = (int)(unsigned char) _svg_ascii_tolower (*s2);
      if (c1 != c2)
	return (c1 - c2);
      s1++; s2++;
    }

  if (n)
    return (((int) (unsigned char) *s1) - ((int) (unsigned char) *s2));
  else
    return 0;
}
