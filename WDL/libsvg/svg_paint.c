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

#include <string.h>

#include "svgint.h"

svg_status_t
_svg_paint_init (svg_paint_t *paint, svg_t *svg, const char *str)
{
    svg_status_t status;

    if (strcmp (str, "none") == 0) {
	paint->type = SVG_PAINT_TYPE_NONE;
	return SVG_STATUS_SUCCESS;
    }

    /* Paint parser courtesy of Steven Kramer */
    if (strncmp (str, "url(#", 5) == 0 && strchr (str, ')')) {
    	svg_element_t	*element = NULL;
    	const char	*end = strchr (str, ')');
    	int 		length = end - (str + 5);
    	char		*id = malloc (length+1);
    	if (!id)
	    return SVG_STATUS_NO_MEMORY;

    	strncpy (id, str + 5, length);
    	id[length] = '\0';

    	_svg_fetch_element_by_id (svg, id, &element);
    	free (id);

	if (element == NULL)
	    return SVG_STATUS_PARSE_ERROR;

	switch (element->type) {
	case SVG_ELEMENT_TYPE_GRADIENT:
	    paint->type = SVG_PAINT_TYPE_GRADIENT;
	    paint->p.gradient = &element->e.gradient;
	    break;
	case SVG_ELEMENT_TYPE_PATTERN:
	    paint->type = SVG_PAINT_TYPE_PATTERN;
	    paint->p.pattern_element = element;
	    break;
	default:
	    return SVG_STATUS_PARSE_ERROR;
	}

	return SVG_STATUS_SUCCESS;
    }

    status = _svg_color_init_from_str (&paint->p.color, str);
    if (status)
	return status;
    paint->type = SVG_PAINT_TYPE_COLOR;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_paint_deinit (svg_paint_t *paint)
{
    switch (paint->type) {
    case SVG_PAINT_TYPE_NONE:
	return SVG_STATUS_SUCCESS;
    case SVG_PAINT_TYPE_COLOR:
	return _svg_color_deinit (&paint->p.color);
    case SVG_PAINT_TYPE_GRADIENT:
    case SVG_PAINT_TYPE_PATTERN:
	/* XXX: If we don't free the gradient/pattern paint here, do
           we anywhere? Do we need to reference count them? */
	return SVG_STATUS_SUCCESS;
    }

    return SVG_STATUS_SUCCESS;
}
