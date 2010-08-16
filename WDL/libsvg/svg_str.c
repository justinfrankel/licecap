/* svg_path.c: Data structures for SVG paths
 
   Copyright © 2002 USC/Information Sciences Institute
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
  
   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
  
   Author: Carl Worth <cworth@isi.edu>
*/

#include "svgint.h"

void
_svg_str_skip_space (const char **str)
{
    const char *s = *str;
    while (_svg_ascii_isspace (*s))
	s++;
    *str = s;
}

void
_svg_str_skip_char (const char **str, char c)
{
    const char *s = *str;
    while (*s == c)
	s++;
    *str = s;
}

void
_svg_str_skip_space_or_char (const char **str, char c)
{
    const char *s = *str;
    while (_svg_ascii_isspace (*s) || *s == c)
	s++;
    *str = s;
}

svgint_status_t
_svg_str_parse_csv_doubles (const char *str, double *value, int num_values, const char **end)
{
    int i;
    double val;
    const char *fail_pos = str;
    svg_status_t status = SVG_STATUS_SUCCESS;

    for (i=0; i < num_values; i++) {
	_svg_str_skip_space_or_char (&str, ',');
	if (*str == '\0') {
	    fail_pos = str;
	    status = SVGINT_STATUS_ARGS_EXHAUSTED;
	    break;
	}

	val = _svg_ascii_strtod (str, &fail_pos);
	if (fail_pos == str) {
	    status = SVGINT_STATUS_ARGS_EXHAUSTED;
	    break;
	}
	str = fail_pos;

	value[i] = val;
    }

    if (end)
	*end = fail_pos;
    return status;
}

svgint_status_t
_svg_str_parse_all_csv_doubles (const char *str, double **value, int *num_values, const char **end)
{
    svgint_status_t status;
    int size = 0;
    *num_values = 0;
    *value = NULL;

    while (1) {
	if (*num_values >= size) {
	    while (*num_values >= size)
		size = size ? size * 2 : 5;
	    *value = realloc(*value, size * sizeof(double));
	    if (*value == NULL)
		status = SVG_STATUS_NO_MEMORY;
	}

	status = _svg_str_parse_csv_doubles (str, *value + *num_values, 1, end);
	if (status)
	    break;

	(*num_values)++;
	str = *end;
    }

    if (status == SVGINT_STATUS_ARGS_EXHAUSTED)
	status = SVG_STATUS_SUCCESS;
    
    return status;
}

