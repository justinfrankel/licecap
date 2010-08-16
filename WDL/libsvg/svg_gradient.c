/* svg_gradient.c: Data structures for SVG gradients
 
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
  
   Author: Steven Kramer
*/

#include "svgint.h"

#include <string.h>

svg_status_t
_svg_gradient_init (svg_gradient_t *gradient)
{
    int i;
    svg_transform_t transform;

    _svg_gradient_set_type (gradient, SVG_GRADIENT_LINEAR);

    gradient->units = SVG_GRADIENT_UNITS_BBOX;
    gradient->spread = SVG_GRADIENT_SPREAD_PAD;

    _svg_transform_init (&transform);
    for (i = 0; i < 6 ; i++) {
	gradient->transform [i] = transform.m[i/2][i%2];
    }

    gradient->stops = NULL;
    gradient->num_stops = 0;
    gradient->stops_size = 0;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_gradient_init_copy (svg_gradient_t *gradient,
			 svg_gradient_t *other)
{
    *gradient = *other;
    
    gradient->stops = malloc (gradient->stops_size * sizeof (svg_gradient_stop_t));
    if (gradient->stops == NULL)
	return SVG_STATUS_NO_MEMORY;
    memcpy (gradient->stops, other->stops, gradient->num_stops * sizeof (svg_gradient_stop_t));

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_gradient_deinit (svg_gradient_t *gradient)
{
    if (gradient->stops) {
	free (gradient->stops);
	gradient->stops = NULL;
    }
    gradient->stops_size = 0;
    gradient->num_stops = 0;
	
    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_gradient_set_type (svg_gradient_t *gradient,
			svg_gradient_type_t type)
{
    gradient->type = type;

    /* XXX: Should check what these defaults should really be. */

    if (gradient->type == SVG_GRADIENT_LINEAR) {
	_svg_length_init_unit (&gradient->u.linear.x1, 0, SVG_LENGTH_UNIT_PCT, SVG_LENGTH_ORIENTATION_HORIZONTAL);
	_svg_length_init_unit (&gradient->u.linear.y1, 0, SVG_LENGTH_UNIT_PCT, SVG_LENGTH_ORIENTATION_VERTICAL);
	_svg_length_init_unit (&gradient->u.linear.x2, 100, SVG_LENGTH_UNIT_PCT, SVG_LENGTH_ORIENTATION_HORIZONTAL);
	_svg_length_init_unit (&gradient->u.linear.y2, 0, SVG_LENGTH_UNIT_PCT, SVG_LENGTH_ORIENTATION_VERTICAL);
    } else {
	_svg_length_init_unit (&gradient->u.radial.cx, 50, SVG_LENGTH_UNIT_PCT, SVG_LENGTH_ORIENTATION_HORIZONTAL);
	_svg_length_init_unit (&gradient->u.radial.cy, 50, SVG_LENGTH_UNIT_PCT, SVG_LENGTH_ORIENTATION_VERTICAL);
	_svg_length_init_unit (&gradient->u.radial.fx, 50, SVG_LENGTH_UNIT_PCT, SVG_LENGTH_ORIENTATION_HORIZONTAL);
	_svg_length_init_unit (&gradient->u.radial.fy, 50, SVG_LENGTH_UNIT_PCT, SVG_LENGTH_ORIENTATION_VERTICAL);
	_svg_length_init_unit (&gradient->u.radial.r, 50, SVG_LENGTH_UNIT_PCT, SVG_LENGTH_ORIENTATION_HORIZONTAL);
    }

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_gradient_add_stop (svg_gradient_t *gradient,
			double		offset,
			svg_color_t	*color,
			double		opacity)
{
    svg_gradient_stop_t *new_stops, *stop;

    if (gradient->num_stops >= gradient->stops_size) {
	int old_size = gradient->stops_size;
	if (gradient->stops_size)
	    gradient->stops_size *= 2;
	else
	    gradient->stops_size = 2; /* Any useful gradient has at least 2 */
	new_stops = realloc (gradient->stops,
			     gradient->stops_size * sizeof (svg_gradient_stop_t));
	if (new_stops == NULL) {
	    gradient->stops_size = old_size;
	    return SVG_STATUS_NO_MEMORY;
	}
	gradient->stops = new_stops;
    }

    stop = &gradient->stops[gradient->num_stops++];
    stop->offset = offset;
    stop->color = *color;
    stop->opacity = opacity;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_gradient_apply_attributes (svg_gradient_t	*gradient,
				svg_t		*svg,
				const char	**attributes)
{
    svgint_status_t status;
    const char *href;
    int i;
    svg_transform_t transform;
    const char *str;
    svg_gradient_t* prototype = 0;

    /* SPK: still an incomplete set of attributes */
    _svg_attribute_get_string (attributes, "xlink:href", &href, 0);
    if (href) {
    	svg_element_t *ref = NULL;
	_svg_fetch_element_by_id (svg, href + 1, &ref);
	
	if (ref && ref->type == SVG_ELEMENT_TYPE_GRADIENT) {
	    svg_gradient_t save_gradient = *gradient;
	    
	    prototype = &ref->e.gradient;
	    _svg_gradient_init_copy (gradient, prototype);
	    
	    if (gradient->type != save_gradient.type) {
		gradient->type = save_gradient.type;
		gradient->u = save_gradient.u;
	    }
	}
    }

    status = _svg_attribute_get_string (attributes, "gradientUnits", &str, "objectBoundingBox");
    if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND && prototype) {
	gradient->units = prototype->units;
    } else {
	if (strcmp (str, "userSpaceOnUse") == 0)
	    gradient->units = SVG_GRADIENT_UNITS_USER;
	else if (strcmp (str, "objectBoundingBox") == 0)
	    gradient->units = SVG_GRADIENT_UNITS_BBOX;
	else
	    return SVG_STATUS_INVALID_VALUE;
    }    
    
    status = _svg_attribute_get_string (attributes, "gradientTransform", &str, 0);
    if (str) {
	_svg_transform_init (&transform);
	_svg_transform_parse_str (&transform, str);
	for (i = 0 ; i < 6 ; i++) {
	    gradient->transform [i] = transform.m[i/2][i%2];
	}
    } else if (prototype) {
	for (i = 0 ; i < 6 ; ++i)
	    gradient->transform[i] = prototype->transform[i];
    }

    status = _svg_attribute_get_string (attributes, "spreadMethod",
					&str, "pad");
    if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND && prototype) {
	gradient->spread = prototype->spread;
    } else {
	if (strcmp (str, "pad") == 0)
	    gradient->spread = SVG_GRADIENT_SPREAD_PAD;
	else if (strcmp (str, "reflect") == 0)
	    gradient->spread = SVG_GRADIENT_SPREAD_REFLECT;
	else if (strcmp (str, "repeat") == 0)
	    gradient->spread = SVG_GRADIENT_SPREAD_REPEAT;
	else
	    return SVG_STATUS_INVALID_VALUE;
    }

    if (prototype && prototype->type != gradient->type)
	prototype = NULL;

    if (gradient->type == SVG_GRADIENT_LINEAR) {
	status = _svg_attribute_get_length (attributes, "x1", &gradient->u.linear.x1, "0%");
	if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND && prototype)
	    gradient->u.linear.x1 = prototype->u.linear.x1;
	status = _svg_attribute_get_length (attributes, "y1", &gradient->u.linear.y1, "0%");
	if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND && prototype)
	    gradient->u.linear.y1 = prototype->u.linear.y1;
	status = _svg_attribute_get_length (attributes, "x2", &gradient->u.linear.x2, "100%");
	if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND && prototype)
	    gradient->u.linear.x2 = prototype->u.linear.x2;
	status = _svg_attribute_get_length (attributes, "y2", &gradient->u.linear.y2, "0%");
	if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND && prototype)
	    gradient->u.linear.y2 = prototype->u.linear.y2;
    } else {
	status = _svg_attribute_get_length (attributes, "cx", &gradient->u.radial.cx, "50%");
	if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND && prototype)
	    gradient->u.radial.cx = prototype->u.radial.cx;
	status = _svg_attribute_get_length (attributes, "cy", &gradient->u.radial.cy, "50%");
	if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND && prototype)
	    gradient->u.radial.cy = prototype->u.radial.cy;
	status = _svg_attribute_get_length (attributes, "r", &gradient->u.radial.r, "50%");
	if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND && prototype)
	    gradient->u.radial.r = prototype->u.radial.r;

	/* fx and fy default to cx and cy */
	status = _svg_attribute_get_length (attributes, "fx", &gradient->u.radial.fx, "50%");
	if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND)
	    gradient->u.radial.fx = gradient->u.radial.cx;

	status = _svg_attribute_get_length (attributes, "fy", &gradient->u.radial.fy, "50%");
	if (status == SVGINT_STATUS_ATTRIBUTE_NOT_FOUND)
	    gradient->u.radial.fy = gradient->u.radial.cy;
    }

    return SVG_STATUS_SUCCESS;
}
