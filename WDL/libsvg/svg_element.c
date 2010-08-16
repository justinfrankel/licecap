/* svg_element.c: Data structures for SVG graphics elements

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

svgint_status_t
_svg_element_create (svg_element_t	**element,
		     svg_element_type_t	type,
		     svg_element_t	*parent,
		     svg_t		*doc)
{
    *element = malloc( sizeof (svg_element_t));
    if (*element == NULL)
	return SVG_STATUS_NO_MEMORY;

    return _svg_element_init (*element, type, parent, doc);
}

svg_status_t
_svg_element_init (svg_element_t	*element,
		   svg_element_type_t	type,
		   svg_element_t	*parent,
		   svg_t		*doc)
{
    svg_status_t status;

    element->type = type;
    element->parent = parent;
    element->doc = doc;
    element->id = NULL;

    status = _svg_transform_init (&element->transform);
    if (status)
	return status;

    status = _svg_style_init_empty (&element->style, doc);
    if (status)
	return status;

    switch (type) {
    case SVG_ELEMENT_TYPE_SVG_GROUP:
    case SVG_ELEMENT_TYPE_GROUP:
    case SVG_ELEMENT_TYPE_DEFS:
    case SVG_ELEMENT_TYPE_USE:
    case SVG_ELEMENT_TYPE_SYMBOL:
	status = _svg_group_init (&element->e.group);
	break;
    case SVG_ELEMENT_TYPE_PATH:
	status = _svg_path_init (&element->e.path);
	break;
    case SVG_ELEMENT_TYPE_CIRCLE:
    case SVG_ELEMENT_TYPE_ELLIPSE:
	status = _svg_ellipse_init (&element->e.ellipse);
	break;
    case SVG_ELEMENT_TYPE_LINE:
	status = _svg_line_init (&element->e.line);
	break;
    case SVG_ELEMENT_TYPE_RECT:
	status = _svg_rect_init (&element->e.rect);
	break;
    case SVG_ELEMENT_TYPE_TEXT:
	status = _svg_text_init (&element->e.text);
	break;
    case SVG_ELEMENT_TYPE_IMAGE:
	status = _svg_image_init (&element->e.image);
	break;
    case SVG_ELEMENT_TYPE_GRADIENT:
	status = _svg_gradient_init (&element->e.gradient);
	break;
    case SVG_ELEMENT_TYPE_PATTERN:
	status = _svg_pattern_init (&element->e.pattern, parent, doc);
	break;
    default:
	status = SVGINT_STATUS_UNKNOWN_ELEMENT;
	break;
    }
    if (status)
	return status;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_element_init_copy (svg_element_t	*element,
			svg_element_t	*other)
{
    svg_status_t status;

    element->type   = other->type;
    element->parent = other->parent;
    if (other->id)
	element->id = strdup (other->id);
    else
	element->id = NULL;

    element->transform = other->transform;

    status = _svg_style_init_copy (&element->style, &other->style);
    if (status)
	return status;

    switch (other->type) {
    case SVG_ELEMENT_TYPE_SVG_GROUP:
    case SVG_ELEMENT_TYPE_GROUP:
    case SVG_ELEMENT_TYPE_DEFS:
    case SVG_ELEMENT_TYPE_USE:
    case SVG_ELEMENT_TYPE_SYMBOL:
	status = _svg_group_init_copy (&element->e.group, &other->e.group);
	break;
    case SVG_ELEMENT_TYPE_PATH:
	status = _svg_path_init_copy (&element->e.path, &other->e.path);
	break;
    case SVG_ELEMENT_TYPE_CIRCLE:
    case SVG_ELEMENT_TYPE_ELLIPSE:
	status = _svg_ellipse_init_copy (&element->e.ellipse, &other->e.ellipse);
	break;
    case SVG_ELEMENT_TYPE_LINE:
	status = _svg_line_init_copy (&element->e.line, &other->e.line);
	break;
    case SVG_ELEMENT_TYPE_RECT:
	status = _svg_rect_init_copy (&element->e.rect, &other->e.rect);
	break;
    case SVG_ELEMENT_TYPE_TEXT:
	status = _svg_text_init_copy (&element->e.text, &other->e.text);
	break;
    case SVG_ELEMENT_TYPE_GRADIENT:
	status = _svg_gradient_init_copy (&element->e.gradient, &other->e.gradient);
	break;
    case SVG_ELEMENT_TYPE_PATTERN:
	status = _svg_pattern_init_copy (&element->e.pattern, &other->e.pattern);
	break;
    case SVG_ELEMENT_TYPE_IMAGE:
	status = _svg_image_init_copy (&element->e.image, &other->e.image);
	break;
    default:
	status = SVGINT_STATUS_UNKNOWN_ELEMENT;
	break;
    }
    if (status)
	return status;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_element_deinit (svg_element_t *element)
{
    svg_status_t status;

    status = _svg_transform_deinit (&element->transform);
    if (status)
	return status;

    status = _svg_style_deinit (&element->style);
    if (status)
	return status;

    if (element->id) {
	free (element->id);
	element->id = NULL;
    }

    switch (element->type) {
    case SVG_ELEMENT_TYPE_SVG_GROUP:
    case SVG_ELEMENT_TYPE_GROUP:
    case SVG_ELEMENT_TYPE_DEFS:
    case SVG_ELEMENT_TYPE_USE:
    case SVG_ELEMENT_TYPE_SYMBOL:
	status = _svg_group_deinit (&element->e.group);
	break;
    case SVG_ELEMENT_TYPE_PATH:
	status = _svg_path_deinit (&element->e.path);
	break;
    case SVG_ELEMENT_TYPE_CIRCLE:
    case SVG_ELEMENT_TYPE_ELLIPSE:
    case SVG_ELEMENT_TYPE_LINE:
    case SVG_ELEMENT_TYPE_RECT:
	status = SVG_STATUS_SUCCESS;
	break;
    case SVG_ELEMENT_TYPE_TEXT:
	status = _svg_text_deinit (&element->e.text);
	break;
    case SVG_ELEMENT_TYPE_GRADIENT:
	status = _svg_gradient_deinit (&element->e.gradient);
	break;
    case SVG_ELEMENT_TYPE_PATTERN:
	status = _svg_pattern_deinit (&element->e.pattern);
	break;
    case SVG_ELEMENT_TYPE_IMAGE:
	status = _svg_image_deinit (&element->e.image);
	break;
    default:
	status = SVGINT_STATUS_UNKNOWN_ELEMENT;
	break;
    }
    if (status)
	return status;

    return SVG_STATUS_SUCCESS;
}

svgint_status_t
_svg_element_clone (svg_element_t	**element,
		    svg_element_t	*other)
{
    *element = malloc( sizeof (svg_element_t));
    if (*element == NULL)
	return SVG_STATUS_NO_MEMORY;

    return _svg_element_init_copy (*element, other);
}

svg_status_t
_svg_element_destroy (svg_element_t *element)
{
    svg_status_t status;

    status = _svg_element_deinit (element);

    free (element);

    return status;
}

svg_status_t
svg_element_render (svg_element_t		*element,
		    svg_render_engine_t		*engine,
		    void			*closure)
{
    svg_status_t status, return_status = SVG_STATUS_SUCCESS;
    svg_transform_t transform = element->transform;

    /* if the display property is not activated, we dont have to
       draw this element nor its children, so we can safely return here. */
    status = _svg_style_get_display (&element->style);
    if (status)
	return status;

    if (element->type == SVG_ELEMENT_TYPE_SVG_GROUP
	|| element->type == SVG_ELEMENT_TYPE_GROUP) {

	status = (engine->begin_group) (closure, _svg_style_get_opacity(&element->style));
	if (status)
	    return status;
    } else {
	status = (engine->begin_element) (closure);
	if (status)
	    return status;
    }

    if (element->type == SVG_ELEMENT_TYPE_SVG_GROUP)
    {
	status = (engine->set_viewport_dimension) (closure, &element->e.group.width, &element->e.group.height);
	if (status)
	    return status;
    }

    /* perform extra viewBox transform */
    if ((element->type == SVG_ELEMENT_TYPE_SVG_GROUP ||
	element->type == SVG_ELEMENT_TYPE_GROUP) &&
	element->e.group.view_box.aspect_ratio != SVG_PRESERVE_ASPECT_RATIO_UNKNOWN)
    {
	status = (engine->apply_view_box) (closure, element->e.group.view_box,
					   &element->e.group.width, &element->e.group.height);
    }
    /* TODO : this is probably not the right place to change transform, but
     atm we dont store svg_length_t in group, so... */
    if (element->type == SVG_ELEMENT_TYPE_SVG_GROUP ||
        element->type == SVG_ELEMENT_TYPE_USE)
	_svg_transform_add_translate (&transform, element->e.group.x.value, element->e.group.y.value);

    status = _svg_transform_render (&transform, engine, closure);
    if (status)
	return status;

    status = _svg_style_render (&element->style, engine, closure);
    if (status)
	return status;

    /* If the element doesnt have children, we can check visibility property, otherwise
       the children will have to be processed. */
    if (element->type != SVG_ELEMENT_TYPE_SVG_GROUP &&
	element->type != SVG_ELEMENT_TYPE_GROUP &&
	element->type != SVG_ELEMENT_TYPE_USE)
	return_status = _svg_style_get_visibility (&element->style);

    if (return_status == SVG_STATUS_SUCCESS) {
	switch (element->type) {
	case SVG_ELEMENT_TYPE_SVG_GROUP:
	case SVG_ELEMENT_TYPE_GROUP:
	case SVG_ELEMENT_TYPE_USE:
	    status = _svg_group_render (&element->e.group, engine, closure);
	    break;
	case SVG_ELEMENT_TYPE_PATH:
	    status = _svg_path_render (&element->e.path, engine, closure);
	    break;
	case SVG_ELEMENT_TYPE_CIRCLE:
	    status = _svg_circle_render (&element->e.ellipse, engine, closure);
	    break;
	case SVG_ELEMENT_TYPE_ELLIPSE:
	    status = _svg_ellipse_render (&element->e.ellipse, engine, closure);
	    break;
	case SVG_ELEMENT_TYPE_LINE:
	    status = _svg_line_render (&element->e.line, engine, closure);
	    break;
	case SVG_ELEMENT_TYPE_RECT:
	    status = _svg_rect_render (&element->e.rect, engine, closure);
	    break;
	case SVG_ELEMENT_TYPE_TEXT:
	    status = _svg_text_render (&element->e.text, engine, closure);
	    break;
	case SVG_ELEMENT_TYPE_IMAGE:
	    status = _svg_image_render (&element->e.image, engine, closure);
	    break;
	case SVG_ELEMENT_TYPE_DEFS:
	    break;
	case SVG_ELEMENT_TYPE_GRADIENT:
	    break; /* Gradients are applied as paint, not rendered directly */
	case SVG_ELEMENT_TYPE_PATTERN:
	    break; /* Patterns are applied as paint, not rendered directly */
	case SVG_ELEMENT_TYPE_SYMBOL:
	    status = _svg_symbol_render (element, engine, closure);
	    break;
	default:
	    status = SVGINT_STATUS_UNKNOWN_ELEMENT;
	    break;
	}
	if (status)
	    return_status = status;
    }

    if (element->type == SVG_ELEMENT_TYPE_SVG_GROUP
	|| element->type == SVG_ELEMENT_TYPE_GROUP) {
	
	status = (engine->end_group) (closure, _svg_style_get_opacity(&element->style));
	if (status && !return_status)
	    return_status = status;
    } else {
	status = (engine->end_element) (closure);
	if (status && !return_status)
	    return_status = status;
    }

    return return_status;
}

svg_status_t
_svg_element_get_nearest_viewport (svg_element_t *element, svg_element_t **viewport)
{
    svg_element_t *elem = element;
    *viewport = NULL;

    while (elem && !*viewport)
    {
	if (elem->type == SVG_ELEMENT_TYPE_SVG_GROUP)
	    *viewport = elem;
	elem = elem->parent;
    }
    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_element_parse_aspect_ratio (const char *aspect_ratio_str,
				 svg_view_box_t *view_box)
{
    const char *start, *end;
    if (strlen (aspect_ratio_str) < 8)
	return SVG_STATUS_SUCCESS;

    if (strncmp(aspect_ratio_str, "xMinYMin", 8) == 0)
	view_box->aspect_ratio = SVG_PRESERVE_ASPECT_RATIO_XMINYMIN;
    else if (strncmp (aspect_ratio_str, "xMidYMin", 8) == 0)
	view_box->aspect_ratio = SVG_PRESERVE_ASPECT_RATIO_XMIDYMIN;
    else if (strncmp (aspect_ratio_str, "xMaxYMin", 8) == 0)
	view_box->aspect_ratio = SVG_PRESERVE_ASPECT_RATIO_XMAXYMIN;
    else if (strncmp (aspect_ratio_str, "xMinYMid", 8) == 0)
	view_box->aspect_ratio = SVG_PRESERVE_ASPECT_RATIO_XMINYMID;
    else if (strncmp (aspect_ratio_str, "xMidYMid", 8) == 0)
	view_box->aspect_ratio = SVG_PRESERVE_ASPECT_RATIO_XMIDYMID;
    else if (strncmp (aspect_ratio_str, "xMaxYMid", 8) == 0)
	view_box->aspect_ratio = SVG_PRESERVE_ASPECT_RATIO_XMAXYMID;
    else if (strncmp (aspect_ratio_str, "xMinYMax", 8) == 0)
	view_box->aspect_ratio = SVG_PRESERVE_ASPECT_RATIO_XMINYMAX;
    else if (strncmp (aspect_ratio_str, "xMidYMax", 8) == 0)
	view_box->aspect_ratio = SVG_PRESERVE_ASPECT_RATIO_XMIDYMAX;
    else if (strncmp (aspect_ratio_str, "xMaxYMax", 8) == 0)
	view_box->aspect_ratio = SVG_PRESERVE_ASPECT_RATIO_XMAXYMAX;
    else
	view_box->aspect_ratio = SVG_PRESERVE_ASPECT_RATIO_NONE;

    start = aspect_ratio_str + 8;
    end = aspect_ratio_str + strlen (aspect_ratio_str) + 1;

	_svg_str_skip_space (&start);

    if (strcmp (start, "meet") == 0)
	view_box->meet_or_slice = SVG_MEET_OR_SLICE_MEET;
    else if (strcmp (start, "slice") == 0)
	view_box->meet_or_slice = SVG_MEET_OR_SLICE_SLICE;
    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_element_parse_view_box (const char	*view_box_str,
			     double	*x,
			     double	*y,
			     double	*width,
			     double	*height)
{
    const char *s;
    const char *end;

    s = view_box_str;
    *x = _svg_ascii_strtod (s, &end);
    if (end == s)
	return SVG_STATUS_PARSE_ERROR;

    s = end;
    _svg_str_skip_space_or_char (&s, ',');
    *y = _svg_ascii_strtod (s, &end);
    if (end == s)
	return SVG_STATUS_PARSE_ERROR;

    s = end;
    _svg_str_skip_space_or_char (&s, ',');
    *width = _svg_ascii_strtod (s, &end);
    if (end == s)
	return SVG_STATUS_PARSE_ERROR;

    s = end;
    _svg_str_skip_space_or_char (&s, ',');
    *height = _svg_ascii_strtod (s, &end);
    if (end == s)
	return SVG_STATUS_PARSE_ERROR;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_element_apply_attributes (svg_element_t	*element,
			       const char	**attributes)
{
    svg_status_t status;
    const char *id;

    status = _svg_transform_apply_attributes (&element->transform, attributes);
    if (status)
	return status;

    status = _svg_style_apply_attributes (&element->style, attributes);
    if (status)
	return status;

    _svg_attribute_get_string (attributes, "id", &id, NULL);
    if (id)
	element->id = strdup (id);

    switch (element->type) {
    case SVG_ELEMENT_TYPE_SVG_GROUP:
	status = _svg_group_apply_svg_attributes (&element->e.group, attributes);
	if (status)
	    return status;
	/* fall-through */
    case SVG_ELEMENT_TYPE_GROUP:
	status = _svg_group_apply_group_attributes (&element->e.group, attributes);
	break;
    case SVG_ELEMENT_TYPE_SYMBOL:
	status = _svg_group_apply_svg_attributes (&element->e.group, attributes);
	break;
    case SVG_ELEMENT_TYPE_USE:
	status = _svg_group_apply_use_attributes (element, attributes);
	break;
    case SVG_ELEMENT_TYPE_PATH:
	status = _svg_path_apply_attributes (&element->e.path, attributes);
	break;
    case SVG_ELEMENT_TYPE_RECT:
    case SVG_ELEMENT_TYPE_CIRCLE:
    case SVG_ELEMENT_TYPE_ELLIPSE:
    case SVG_ELEMENT_TYPE_LINE:
	break;
    case SVG_ELEMENT_TYPE_TEXT:
	status = _svg_text_apply_attributes (&element->e.text, attributes);
	break;
    case SVG_ELEMENT_TYPE_IMAGE:
	status = _svg_image_apply_attributes (&element->e.image, attributes);
	break;
    case SVG_ELEMENT_TYPE_GRADIENT:
	status = _svg_gradient_apply_attributes (&element->e.gradient,
						 element->doc, attributes);
	break;
    case SVG_ELEMENT_TYPE_PATTERN:
	status = _svg_pattern_apply_attributes (&element->e.pattern, attributes);
	break;
    default:
	status = SVGINT_STATUS_UNKNOWN_ELEMENT;
	break;
    }

    if (status)
	return status;

    return SVG_STATUS_SUCCESS;
}

svg_pattern_t *
svg_element_pattern (svg_element_t *element)
{
    if (element->type != SVG_ELEMENT_TYPE_PATTERN)
	return NULL;

    return &element->e.pattern;
}
