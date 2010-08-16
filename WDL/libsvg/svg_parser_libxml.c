/* svg_parser_libxml.c: libxml2 SAX-based parser for SVG documents

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

static xmlEntity *
_svg_parser_sax_get_entity (void		*closure,
			    const xmlChar	*name);

static void
_svg_parser_sax_entity_decl (void		*closure,
			     const xmlChar	*name,
			     int		type,
			     const xmlChar	*publicId,
			     const xmlChar	*systemId,
			     xmlChar		*content);

static void
_svg_parser_sax_warning (void		*closure,
			 const char	*msg, ...);

static void
_svg_parser_sax_error (void		*closure,
		       const char	*msg, ...);

static void
_svg_parser_sax_fatal_error (void	*closure,
			     const char	*msg, ...);

static xmlSAXHandler SVG_PARSER_SAX_HANDLER = {
    NULL,				/* internalSubset */
    NULL,				/* isStandalone */
    NULL,				/* hasInternalSubset */
    NULL,				/* hasExternalSubset */
    NULL,				/* resolveEntity */
    _svg_parser_sax_get_entity,		/* getEntity */
    _svg_parser_sax_entity_decl,	/* entityDecl */
    NULL,				/* notationDecl */
    NULL,				/* attributeDecl */
    NULL,				/* elementDecl */
    NULL,				/* unparsedEntityDecl */
    NULL,				/* setDocumentLocator */
    NULL,				/* startDocument */
    NULL,				/* endDocument */
    _svg_parser_sax_start_element,	/* startElement */
    _svg_parser_sax_end_element,	/* endElement */
    NULL,				/* reference */
    _svg_parser_sax_characters,		/* characters */
    _svg_parser_sax_characters,		/* ignorableWhitespace */
    NULL,				/* processingInstruction */
    NULL,				/* comment */
    _svg_parser_sax_warning,		/* xmlParserWarning */
    _svg_parser_sax_error,		/* xmlParserError */
    _svg_parser_sax_fatal_error,	/* xmlParserFatalError */
    NULL,				/* getParameterEntity */
};

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

    parser->ctxt = xmlCreatePushParserCtxt (&SVG_PARSER_SAX_HANDLER,
					    parser, NULL, 0, NULL);
    if (parser->ctxt == NULL)
	parser->status = SVG_STATUS_NO_MEMORY;

    parser->ctxt->replaceEntities = 1;

    parser->entities = xmlHashCreate (100);
    return parser->status;
}

svg_status_t
_svg_parser_parse_chunk (svg_parser_t *parser, const char *buf, size_t count)
{
    if (parser->status)
	return parser->status;

    if (parser->ctxt == NULL)
	return SVG_STATUS_INVALID_CALL;

    xmlParseChunk (parser->ctxt, buf, count, 0);

    return parser->status;
}

svg_status_t
_svg_parser_end (svg_parser_t *parser)
{
    if (parser->ctxt == NULL)
	return SVG_STATUS_INVALID_CALL;

    if (! parser->ctxt->wellFormed)
	parser->status = SVG_STATUS_PARSE_ERROR;

    xmlFreeParserCtxt (parser->ctxt);
    parser->ctxt = NULL;

    xmlHashFree (parser->entities, (xmlHashDeallocator) xmlFree);
    parser->entities = NULL;

    return parser->status;
}

static xmlEntity *
_svg_parser_sax_get_entity (void		*closure,
			    const xmlChar	*name)
{
    svg_parser_t *parser = closure;

    return xmlHashLookup (parser->entities, name);
}

/* XXX: It's not clear to my why we have to do this. libxml2 has all
 * of this code internally, so we should be able to access a hash of
 * entities automatically rather than maintaining our own. */
static void
_svg_parser_sax_entity_decl (void		*closure,
			     const xmlChar	*name,
			     int		type,
			     const xmlChar	*publicId,
			     const xmlChar	*systemId,
			     xmlChar		*content)
{
    svg_parser_t *parser = closure;
    xmlEntityPtr entity;

    entity = malloc (sizeof(xmlEntity));

    entity->type = XML_ENTITY_DECL;
    entity->name = xmlStrdup (name);
    entity->etype = type;
    if (publicId)
	entity->ExternalID = xmlStrdup (publicId);
    else
	entity->ExternalID = NULL;
    if (systemId)
	entity->SystemID = xmlStrdup (systemId);
    else
	entity->SystemID = NULL;

    if (content) {
	entity->length = xmlStrlen (content);
	entity->content = xmlStrndup (content, entity->length);
    } else {
	entity->length = 0;
	entity->content = NULL;
    }
    entity->URI = NULL;
    entity->orig = NULL;
    entity->owner = 0;
    entity->children = NULL;

    if (xmlHashAddEntry (parser->entities, name, entity)) {
	/* Entity was already defined at another level. */
	free (entity);
    }
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

