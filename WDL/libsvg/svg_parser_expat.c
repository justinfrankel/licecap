/* svg_parser_expat.c: Expat SAX-based parser for SVG documents

   Copyright © 2000 Eazel, Inc.
   Copyright © 2002 Dom Lachowicz <cinamod@hotmail.com>
   Copyright © 2002 USC/Information Sciences Institute
   Copyright © 2005 Phil Blundell <pb@handhelds.org>

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
#include "svg_hash.h"

static void
_svg_parser_sax_start_element (void		*closure,
			       const XML_Char	*name,
			       const XML_Char	**atts);

static void
_svg_parser_sax_end_element (void		*closure,
			     const XML_Char	*name);

static void
_svg_parser_sax_characters (void		*closure,
			    const XML_Char	*ch,
			    int			len);

static void
_svg_parser_sax_warning (void		*closure,
			 const char	*msg, ...);

static void
_svg_parser_sax_error (void		*closure,
		       const char	*msg, ...);

static void
_svg_parser_sax_fatal_error (void	*closure,
			     const char	*msg, ...);
svg_status_t
_svg_parser_init (svg_parser_t *parser, svg_t *svg)
{
    parser->svg = svg;
    parser->ctxt = NULL;

    parser->unknown_element_depth = 0;

    parser->state = NULL;

    parser->status = SVG_STATUS_SUCCESS;

    return parser->status;
}

svg_status_t
_svg_parser_deinit (svg_parser_t *parser)
{
    parser->svg = NULL;
    parser->ctxt = NULL;

    parser->status = SVG_STATUS_SUCCESS;

    return parser->status;
}

svg_status_t
_svg_parser_begin (svg_parser_t *parser)
{
    /* Innocent until proven guilty */
    parser->status = SVG_STATUS_SUCCESS;

    if (parser->ctxt)
	parser->status = SVG_STATUS_INVALID_CALL;

    parser->ctxt = XML_ParserCreate (NULL);
    XML_SetUserData (parser->ctxt, parser);

    XML_SetStartElementHandler (parser->ctxt, _svg_parser_sax_start_element);
    XML_SetEndElementHandler (parser->ctxt, _svg_parser_sax_end_element);
    XML_SetCharacterDataHandler (parser->ctxt, _svg_parser_sax_characters);

    if (parser->ctxt == NULL)
	parser->status = SVG_STATUS_NO_MEMORY;

    return parser->status;
}

svg_status_t
_svg_parser_parse_chunk (svg_parser_t *parser, const char *buf, size_t count)
{
    if (parser->status)
	return parser->status;

    if (parser->ctxt == NULL)
	return SVG_STATUS_INVALID_CALL;

    if (XML_Parse (parser->ctxt, buf, count, 0) != XML_STATUS_OK)
	parser->status = SVG_STATUS_PARSE_ERROR;

    return parser->status;
}

svg_status_t
_svg_parser_end (svg_parser_t *parser)
{
    if (parser->ctxt == NULL)
	return SVG_STATUS_INVALID_CALL;

    if (XML_Parse (parser->ctxt, NULL, 0, 1) != XML_STATUS_OK)
	parser->status = SVG_STATUS_PARSE_ERROR;

    XML_ParserFree (parser->ctxt);

    parser->ctxt = NULL;

    return parser->status;
}

static void
_svg_parser_sax_warning (void *closure, const char *msg, ...)
{
    va_list args;
	
    va_start (args, msg);
    vfprintf (stderr, msg, args);
    va_end (args);
}

static void
_svg_parser_sax_error (void *closure, const char *msg, ...)
{
    va_list args;
	
    va_start (args, msg);
    vfprintf (stderr, msg, args);
    va_end (args);
}

static void
_svg_parser_sax_fatal_error (void *closure, const char *msg, ...)
{
    va_list args;
	
    va_start (args, msg);
    vfprintf (stderr, msg, args);
    va_end (args);
}

