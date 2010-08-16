/* svg_parser.c: SAX-based parser for SVG documents

   Copyright © 2000 Eazel, Inc.
   Copyright © 2002 Dom Lachowicz <cinamod@hotmail.com>
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

   Author: Raph Levien <raph@artofcode.com>
*/

#include <stdarg.h>
#include <math.h>
#include <string.h>

#include "svgint.h"

static svg_status_t
_svg_parser_push_state (svg_parser_t		*parser,
			const svg_parser_cb_t	*cb);

static svg_status_t
_svg_parser_pop_state (svg_parser_t *parser);

static svg_status_t
_svg_parser_new_group_element (svg_parser_t *parser,
			       svg_element_t **group_element,
			       svg_element_type_t type);

static svg_status_t
_svg_parser_new_svg_group_element (svg_parser_t *parser, svg_element_t **group_element);

static svg_status_t
_svg_parser_new_leaf_element (svg_parser_t *parser,
			      svg_element_t **child_element,
			      svg_element_type_t type);

static svg_status_t
_svg_parser_parse_anchor (svg_parser_t	*parser,
			  const char	**attributes,
			  svg_element_t	**group_element);

static svg_status_t
_svg_parser_parse_svg (svg_parser_t	*parser,
		       const char	**attributes,
		       svg_element_t	**group_element);

static svg_status_t
_svg_parser_parse_defs (svg_parser_t	*parser,
		       const char	**attributes,
		       svg_element_t	**group_element);

static svg_status_t
_svg_parser_parse_use (svg_parser_t	*parser,
		       const char	**attributes,
		       svg_element_t	**group_element);

static svg_status_t
_svg_parser_parse_symbol (svg_parser_t	*parser,
			  const char	**attributes,
			  svg_element_t	**group_element);

static svg_status_t
_svg_parser_parse_group (svg_parser_t	*parser,
			 const char	**attributes,
			 svg_element_t	**group_element);

static svg_status_t
_svg_parser_parse_path (svg_parser_t	*parser,
			const char	**attributes,
			svg_element_t	**path_element);

static svg_status_t
_svg_parser_parse_line (svg_parser_t	*parser,
			const char	**attributes,
			svg_element_t	**path_element);

static svg_status_t
_svg_parser_parse_rect (svg_parser_t	*parser,
			const char	**attributes,
			svg_element_t	**path_element);

static svg_status_t
_svg_parser_parse_circle (svg_parser_t	*parser,
			  const char	**attributes,
			  svg_element_t	**path_element);

static svg_status_t
_svg_parser_parse_ellipse (svg_parser_t		*parser,
			   const char	**attributes,
			   svg_element_t	**path_element);

static svg_status_t
_svg_parser_parse_polygon (svg_parser_t		*parser,
			   const char	**attributes,
			   svg_element_t	**path_element);

static svg_status_t
_svg_parser_parse_polyline (svg_parser_t	*parser,
			    const char	**attributes,
			    svg_element_t	**path_element);

static svg_status_t
_svg_parser_parse_text (svg_parser_t	*parser,
			const char	**attributes,
			svg_element_t	**text_element);

static svg_status_t
_svg_parser_parse_image (svg_parser_t	*parser,
			 const char	**attributes,
			 svg_element_t	**image_element);

static svg_status_t
_svg_parser_parse_linear_gradient (svg_parser_t	*parser,
				   const char	**attributes,
				   svg_element_t	**path_element);

static svg_status_t
_svg_parser_parse_radial_gradient (svg_parser_t	*parser,
				   const char	**attributes,
				   svg_element_t	**path_element);

static svg_status_t
_svg_parser_parse_gradient_stop (svg_parser_t	*parser,
				 const char	**attributes,
				 svg_element_t	**stop_element);

static svg_status_t
_svg_parser_parse_pattern (svg_parser_t	*parser,
			   const char	**attributes,
			   svg_element_t	**path_element);

static svg_status_t
_svg_parser_parse_text_characters (svg_parser_t		*parser,
				   const char	*ch,
				   int			len);

typedef struct svg_parser_map {
    char		*name;
    svg_parser_cb_t	cb;
} svg_parser_map_t;

static const svg_parser_map_t SVG_PARSER_MAP[] = {
    {"a", 		{_svg_parser_parse_anchor,		NULL }},
    {"svg",		{_svg_parser_parse_svg,			NULL }},
    {"g",		{_svg_parser_parse_group,		NULL }},
    {"path",		{_svg_parser_parse_path,		NULL }},
    {"line",		{_svg_parser_parse_line,		NULL }},
    {"rect",		{_svg_parser_parse_rect,		NULL }},
    {"circle",		{_svg_parser_parse_circle,		NULL }},
    {"ellipse",		{_svg_parser_parse_ellipse,		NULL }},
    {"defs",		{_svg_parser_parse_defs,		NULL }},
    {"use",		{_svg_parser_parse_use,			NULL }},
    {"symbol",		{_svg_parser_parse_symbol,		NULL }},
    {"polygon",		{_svg_parser_parse_polygon,		NULL }},
    {"polyline",	{_svg_parser_parse_polyline,		NULL }},
    {"text",		{_svg_parser_parse_text,
			 _svg_parser_parse_text_characters }},
    {"image",		{_svg_parser_parse_image,		NULL }},
    {"linearGradient",	{_svg_parser_parse_linear_gradient,	NULL }},
    {"radialGradient",	{_svg_parser_parse_radial_gradient,	NULL }},
    {"stop",		{_svg_parser_parse_gradient_stop,	NULL }},
    {"pattern",		{_svg_parser_parse_pattern,		NULL }},
};

void
_svg_parser_sax_start_element (void		*closure,
			       const xmlChar	*name_unsigned,
			       const xmlChar	**attributes_unsigned)
{
    unsigned int i;
    svg_parser_t *parser = closure;
    const svg_parser_cb_t *cb;
    svg_element_t *element;
    const char *name = (const char *) name_unsigned;
    const char **attributes = (const char **) attributes_unsigned;

    if (parser->unknown_element_depth) {
	parser->unknown_element_depth++;
	return;
    }

    cb = NULL;
    for (i=0; i < SVG_ARRAY_SIZE (SVG_PARSER_MAP); i++) {
	if (strcmp (SVG_PARSER_MAP[i].name, name) == 0) {
	    cb = &SVG_PARSER_MAP[i].cb;
	    break;
	}
    }

    if (cb == NULL) {
	parser->unknown_element_depth++;
	return;
    }

    parser->status = _svg_parser_push_state (parser, cb);
    if (parser->status)
	return;

    parser->status = (cb->parse_element) (parser, attributes, &element);
    if (parser->status) {
	if (parser->status == SVGINT_STATUS_UNKNOWN_ELEMENT)
	    parser->status = SVG_STATUS_SUCCESS;
	return;
    }

    parser->status = _svg_element_apply_attributes (element, attributes);
    if (parser->status)
	return;

    if (element->id)
	_svg_store_element_by_id (parser->svg, element);

    return;
}

void
_svg_parser_sax_end_element (void		*closure,
			     const xmlChar	*name)
{
    svg_parser_t *parser = closure;

    if (parser->unknown_element_depth) {
	parser->unknown_element_depth--;
	return;
    }

    parser->status = _svg_parser_pop_state (parser);
    if (parser->status)
	return;

    return;
}

void
_svg_parser_sax_characters (void		*closure,
			    const xmlChar	*ch_unsigned,
			    int			len)
{
    int i;
    svg_parser_t *parser = closure;
    const char *src, *ch = (const char *) ch_unsigned;
    char *ch_copy, *dst;
    int space;

    /* XXX: This is the correct default behavior, but we're supposed
     * to honor xml:space="preserve" if present, (which just means to
     * not do this replacement).
     */
    ch_copy = malloc (len);
    if (ch_copy == NULL)
	return;

    dst = ch_copy;
    space = 0;
    for (src=ch, i=0; i < len; i++, src++) {
	if (*src == '\n')
	    continue;
	if (*src == '\t' || *src == ' ') {
	    if (space)
		continue;
	    *dst = ' ';
	    space = 1;
	} else {
	    *dst = *src;
	    space = 0;
	}
	dst++;
    }

    if (parser->state->cb->parse_characters) {
	parser->status = (parser->state->cb->parse_characters) (parser, ch_copy, dst - ch_copy);
	if (parser->status)
	    return;
    }

    free (ch_copy);
    ch_copy = NULL;

    return;
}

static svg_status_t
_svg_parser_push_state (svg_parser_t		*parser,
			const svg_parser_cb_t	*cb)
{
    svg_parser_state_t *state;

    state = malloc (sizeof (svg_parser_state_t));
    if (state == NULL)
	return SVG_STATUS_NO_MEMORY;

    if (parser->state) {
	*state = *parser->state;
    } else {
	state->group_element = NULL;
	state->text = NULL;
    }

    state->cb = cb;

    state->next = parser->state;
    parser->state = state;

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_parser_pop_state (svg_parser_t *parser)
{
    svg_parser_state_t *old;

    if (parser->state == NULL)
	return SVG_STATUS_SUCCESS;

    old = parser->state;
    parser->state = parser->state->next;
    free(old);

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_parser_new_svg_group_element (svg_parser_t *parser, svg_element_t **group_element)
{
    svg_status_t status;
    svg_element_t *parent;

    parent = parser->state->group_element;

    status = _svg_element_create (group_element,
				  SVG_ELEMENT_TYPE_SVG_GROUP,
				  parent,
				  parser->svg);
    if (status)
	    return status;

    if (parent) {
	status = _svg_group_add_element (&parent->e.group, *group_element);
    } else {
	_svg_style_init_defaults (&(*group_element)->style, parser->svg);
	parser->svg->group_element = *group_element;
    }

    parser->state->group_element = *group_element;

    return status;
}

static svg_status_t
_svg_parser_new_group_element (svg_parser_t *parser,
			       svg_element_t **group_element,
			       svg_element_type_t type)
{
    svg_status_t status;

    status = _svg_parser_new_leaf_element (parser, group_element, type);
    if (status)
	return status;

    /* The only thing that distinguishes a group from a leaf is that
       the group becomes the new parent for future elements. */
    parser->state->group_element = *group_element;

    return status;
}

static svg_status_t
_svg_parser_new_leaf_element (svg_parser_t *parser,
			      svg_element_t **child_element,
			      svg_element_type_t type)
{
    svg_status_t status;

    status = _svg_element_create (child_element, type,
				  parser->state->group_element,
				  parser->svg);
    if (status)
	return status;

    status = _svg_group_add_element (&parser->state->group_element->e.group,
				     *child_element);
    if (status)
	return status;

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_parser_parse_anchor (svg_parser_t	*parser,
			  const char	**attributes,
			  svg_element_t	**group_element)
{
    /* XXX: Currently ignoring all anchor elements */
    return SVGINT_STATUS_UNKNOWN_ELEMENT;
}

static svg_status_t
_svg_parser_parse_svg (svg_parser_t	*parser,
		       const char	**attributes,
		       svg_element_t	**group_element)
{
    return _svg_parser_new_svg_group_element (parser, group_element);
}

static svg_status_t
_svg_parser_parse_group (svg_parser_t	*parser,
			 const char	**attributes,
			 svg_element_t	**group_element)
{
    return _svg_parser_new_group_element (parser, group_element, SVG_ELEMENT_TYPE_GROUP);
}

static svg_status_t
_svg_parser_parse_defs (svg_parser_t	*parser,
			const char	**attributes,
			svg_element_t	**group_element)
{
    return _svg_parser_new_group_element (parser, group_element, SVG_ELEMENT_TYPE_DEFS);
}

static svg_status_t
_svg_parser_parse_use (svg_parser_t	*parser,
			const char	**attributes,
			svg_element_t	**group_element)
{
    return _svg_parser_new_group_element (parser, group_element, SVG_ELEMENT_TYPE_USE);
}

static svg_status_t
_svg_parser_parse_symbol (svg_parser_t	*parser,
			  const char	**attributes,
			  svg_element_t	**group_element)
{
    return _svg_parser_new_group_element (parser, group_element, SVG_ELEMENT_TYPE_SYMBOL);
}

static svg_status_t
_svg_parser_parse_path (svg_parser_t	*parser,
			const char	**attributes,
			svg_element_t	**path_element)
{
    return _svg_parser_new_leaf_element (parser,
					 path_element,
					 SVG_ELEMENT_TYPE_PATH);
}

static svg_status_t
_svg_parser_parse_line (svg_parser_t	*parser,
			const char	**attributes,
			svg_element_t	**path_element)
{
    svg_status_t status;

    status = _svg_parser_new_leaf_element (parser, path_element, SVG_ELEMENT_TYPE_LINE);
    if (status)
	return status;

    _svg_attribute_get_length (attributes, "x1", &((*path_element)->e.line.x1), "0");
    _svg_attribute_get_length (attributes, "y1", &((*path_element)->e.line.y1), "0");
    _svg_attribute_get_length (attributes, "x2", &((*path_element)->e.line.x2), "0");
    _svg_attribute_get_length (attributes, "y2", &((*path_element)->e.line.y2), "0");

    return SVG_STATUS_SUCCESS;
}


static svg_status_t
_svg_parser_parse_rect (svg_parser_t	*parser,
			const char	**attributes,
			svg_element_t	**path_element)
{
    svg_status_t status;
    int has_rx = 0, has_ry = 0;

    status = _svg_parser_new_leaf_element (parser, path_element, SVG_ELEMENT_TYPE_RECT);
    if (status)
	return SVG_STATUS_PARSE_ERROR;

    _svg_attribute_get_length (attributes, "x", &((*path_element)->e.rect.x), "0");
    _svg_attribute_get_length (attributes, "y", &((*path_element)->e.rect.y), "0");
    _svg_attribute_get_length (attributes, "width", &((*path_element)->e.rect.width), "0");
    _svg_attribute_get_length (attributes, "height", &((*path_element)->e.rect.height), "0");
    status = _svg_attribute_get_length (attributes, "rx", &((*path_element)->e.rect.rx), "0");
    if (status == SVG_STATUS_SUCCESS)
	has_rx = 1;
    status = _svg_attribute_get_length (attributes, "ry", &((*path_element)->e.rect.ry), "0");
    if (status == SVG_STATUS_SUCCESS)
	has_ry = 1;

    if (has_rx || has_ry) {
 	if (! has_rx)
	    (*path_element)->e.rect.rx = (*path_element)->e.rect.ry;
 	if (! has_ry)
	    (*path_element)->e.rect.ry = (*path_element)->e.rect.rx;
	}
    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_parser_parse_circle (svg_parser_t	*parser,
			  const char	**attributes,
			  svg_element_t	**path_element)
{
    svg_status_t status;

    status = _svg_parser_new_leaf_element (parser, path_element, SVG_ELEMENT_TYPE_CIRCLE);
    if (status)
	return SVG_STATUS_PARSE_ERROR;

    _svg_attribute_get_length (attributes, "cx", &((*path_element)->e.ellipse.cx), "0");
    _svg_attribute_get_length (attributes, "cy", &((*path_element)->e.ellipse.cy), "0");
    _svg_attribute_get_length (attributes, "r", &((*path_element)->e.ellipse.rx), "100%");
    _svg_attribute_get_length (attributes, "r", &((*path_element)->e.ellipse.ry), "100%");
    if ((*path_element)->e.ellipse.rx.value < 0)
	return SVG_STATUS_PARSE_ERROR;

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_parser_parse_ellipse (svg_parser_t		*parser,
			   const char	**attributes,
			   svg_element_t	**path_element)
{
    svg_status_t status;

    status = _svg_parser_new_leaf_element (parser, path_element, SVG_ELEMENT_TYPE_ELLIPSE);
    if (status)
	return status;

    _svg_attribute_get_length (attributes, "cx", &((*path_element)->e.ellipse.cx), "0");
    _svg_attribute_get_length (attributes, "cy", &((*path_element)->e.ellipse.cy), "0");
    _svg_attribute_get_length (attributes, "rx", &((*path_element)->e.ellipse.rx), "100%");
    _svg_attribute_get_length (attributes, "ry", &((*path_element)->e.ellipse.ry), "100%");
    if ((*path_element)->e.ellipse.rx.value < 0 || (*path_element)->e.ellipse.ry.value < 0)
	return SVG_STATUS_PARSE_ERROR;

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_parser_parse_polygon (svg_parser_t		*parser,
			   const char	**attributes,
			   svg_element_t	**path_element)
{
    svg_status_t status;
    svg_path_t *path;

    status = _svg_parser_parse_polyline (parser,
					 attributes,
					 path_element);
    if (status)
	return status;

    path = &(*path_element)->e.path;

    status = _svg_path_close_path (path);
    if (status)
	return status;

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_parser_parse_polyline (svg_parser_t	*parser,
			    const char	**attributes,
			    svg_element_t	**path_element)
{
    svg_status_t status;
    const char *points;
    const char *p, *next;
    svg_path_t *path;
    double pt[2];
    int first;

    _svg_attribute_get_string (attributes, "points", &points, NULL);

    if (points == NULL)
	return SVG_STATUS_PARSE_ERROR;

    status = _svg_parser_new_leaf_element (parser,
					   path_element,
					   SVG_ELEMENT_TYPE_PATH);
    if (status)
	return status;
    path = &(*path_element)->e.path;

    first = 1;
    p = points;
    while (*p) {
	status = _svg_str_parse_csv_doubles (p, pt, 2, &next);
	if (status)
	    return SVG_STATUS_PARSE_ERROR;

	if (first) {
	    _svg_path_move_to (path, pt[0], pt[1]);
	    first = 0;
	} else {
	    _svg_path_line_to (path, pt[0], pt[1]);
	}

	p = next;
	_svg_str_skip_space (&p);
    }

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_parser_parse_text (svg_parser_t	*parser,
			const char	**attributes,
			svg_element_t	**text_element)
{
    svg_status_t status;

    status = _svg_parser_new_leaf_element (parser,
					   text_element,
					   SVG_ELEMENT_TYPE_TEXT);
    if (status)
	return status;

    parser->state->text = &(*text_element)->e.text;

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_parser_parse_image (svg_parser_t	*parser,
			 const char	**attributes,
			 svg_element_t	**image_element)
{
    return _svg_parser_new_leaf_element (parser,
					 image_element,
					 SVG_ELEMENT_TYPE_IMAGE);
}

/* Gradient parsing code by Steven Kramer */

static svg_status_t
_svg_parser_parse_linear_gradient (svg_parser_t	*parser,
				   const char	**attributes,
				   svg_element_t	**gradient_element)
{
    svg_status_t status;

    /* XXX: This is really bogus. The gradient element is a subtype of
       svg_element_t that is distinct from the svg_group_t
       subtype. So, if any gradient happens to have a child element
       other than a gradient stop (which has hacked parsing to avoid
       the problem), then the svg_element union will be totally
       scrambled. This must be fixed. */
    status = _svg_parser_new_group_element (parser,
					    gradient_element,
					    SVG_ELEMENT_TYPE_GRADIENT);
    if (status)
	return status;
    
    _svg_gradient_set_type (&(*gradient_element)->e.gradient,
			    SVG_GRADIENT_LINEAR);

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_parser_parse_radial_gradient (svg_parser_t		*parser,
				   const char	**attributes,
				   svg_element_t	**gradient_element)
{
    svg_status_t status;

    status = _svg_parser_new_group_element (parser,
					    gradient_element,
					    SVG_ELEMENT_TYPE_GRADIENT);
    if (status)
	return status;

    _svg_gradient_set_type (&(*gradient_element)->e.gradient,
			    SVG_GRADIENT_RADIAL);

    return SVG_STATUS_SUCCESS;
}

/* XXX: This function is a mess --- it's doing its own style/attribute
   handling rather than leaving that to the existing
   framework. Instead this should be shifted to the standard
   mechanism, whereby we have a new svg_gradient_stop element, etc.

   If we'd like to, we can collapse the gradient's child stop elements
   into an array when the gradient is done being parsed.  */
static svg_status_t
_svg_parser_parse_gradient_stop (svg_parser_t	*parser,
				 const char	**attributes,
				 svg_element_t	**gradient_element)
{
    svg_style_t style;
    svg_gradient_t* gradient;
    double offset;
    double opacity;
    svg_color_t color;
    const char* color_str;
    svg_element_t *group_element;

    gradient_element = NULL;

    if (parser->state->group_element == NULL
	|| parser->state->group_element->type != SVG_ELEMENT_TYPE_GRADIENT)
	return SVG_STATUS_PARSE_ERROR;

    group_element = parser->state->group_element;
    gradient = &group_element->e.gradient;

    /* XXX: This ad-hoc style parsing breaks inheritance I believe. */
    _svg_style_init_empty (&style, parser->svg);
    style.flags = SVG_STYLE_FLAG_NONE;
    _svg_style_apply_attributes (&style, attributes);
    color = style.color;
    opacity = style.opacity;

    _svg_attribute_get_double (attributes, "offset", &offset, 0);
    _svg_attribute_get_double (attributes, "stop-opacity", &opacity, opacity);
    if (_svg_attribute_get_string (attributes, "stop-color", &color_str, "#000000") == SVG_STATUS_SUCCESS)
	_svg_color_init_from_str (&color, color_str);
    if (color.is_current_color)
	color = group_element->style.color;

    /* XXX: Rather than directly storing the stop in the gradient
       here, it would be cleaner to just have the stop be a standard
       child element. */
    _svg_gradient_add_stop (gradient, offset, &color, opacity);
    
    /* XXX: Obviously, this is totally bogus and needs to change. */
    /* not quite unknown, just don't store the element and stop applying attributes */
    return SVGINT_STATUS_UNKNOWN_ELEMENT;
}

static svg_status_t
_svg_parser_parse_pattern (svg_parser_t		*parser,
			   const char	**attributes,
			   svg_element_t	**pattern_element)
{
    svg_status_t status;

    status = _svg_parser_new_leaf_element (parser,
					   pattern_element,
					   SVG_ELEMENT_TYPE_PATTERN);

    /* XXX: This is a bit messy. The pattern has child elements, but
       is not of SVG_ELEMENT_TYPE_GROUP, but rather contains one. We
       need to set that containend group as the parent for future
       components. */
    parser->state->group_element = (*pattern_element)->e.pattern.group_element;

    return SVG_STATUS_SUCCESS;
}


static svg_status_t
_svg_parser_parse_text_characters (svg_parser_t		*parser,
				   const char	*ch,
				   int			len)
{
    svg_status_t status;
    svg_text_t *text;

    text = parser->state->text;
    if (text == NULL)
	return SVG_STATUS_PARSE_ERROR;

    status = _svg_text_append_chars (text, ch, len);
    if (status)
	return status;

    return SVG_STATUS_SUCCESS;
}
