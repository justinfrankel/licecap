/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: svg_parser_tinyxml.c (tinyxml support for libsvg)
  See lice.h for license and other information
*/

#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <wchar.h>

extern "C" {
  
#include "svgint.h"
#include "svg_hash.h"

};

#include "tinyxml/tinyxml.h"
#include "../wdlstring.h"

typedef char XML_Char;

class XmlStuff
{
public:
  TiXmlDocument xd;
  WDL_String str;
};

/*static void
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

/*static void
_svg_parser_sax_warning (void		*closure,
			 const char	*msg, ...);

static void
_svg_parser_sax_error (void		*closure,
		       const char	*msg, ...);

static void
_svg_parser_sax_fatal_error (void	*closure,
			     const char	*msg, ...);*/

extern "C" {

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
  parser->status = SVG_STATUS_SUCCESS;

  if (parser->ctxt)
    parser->status = SVG_STATUS_INVALID_CALL;

  parser->ctxt = (int)new XmlStuff;
    /*parser->ctxt = XML_ParserCreate (NULL);
    XML_SetUserData (parser->ctxt, parser);

    XML_SetStartElementHandler (parser->ctxt, _svg_parser_sax_start_element);
    XML_SetEndElementHandler (parser->ctxt, _svg_parser_sax_end_element);
    XML_SetCharacterDataHandler (parser->ctxt, _svg_parser_sax_characters);*/

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

  XmlStuff *xs = (XmlStuff *)parser->ctxt;
  xs->str.Append(buf, count);

  return parser->status;
}

static void attributeAdd(WDL_HeapBuf *hp, char *an, char *av)
{
  int sc = sizeof(char *); //hopefully 64-bit compatible...
  int pl = hp->GetSize();
  hp->Resize(pl+sc*2);
  char **p = (char **)hp->Get();
  pl /= sc;
  p[pl] = (char *)an;
  p[pl+1] = (char *)av;
}

static void processElements(svg_parser_t *parser, TiXmlElement *elem)
{
  for(; elem; elem = elem->NextSiblingElement())
  {
    const char *v = elem->Value();
    const char *t = elem->GetText();

    //fill up attributes table
    WDL_HeapBuf attributes;
    TiXmlAttribute *attr;
    for(attr = elem->FirstAttribute(); attr; attr = attr->Next())
    {
      const char *an = attr->Name();
      const char *av = attr->Value();
      attributeAdd(&attributes, (char *)an, (char *)av);
    }
    attributeAdd(&attributes, NULL, NULL);

    _svg_parser_sax_start_element(parser, v, (const char **)attributes.Get());

    if(t) _svg_parser_sax_characters(parser, t, strlen(t));

    TiXmlElement *child = elem->FirstChildElement();
    if(child) processElements(parser, child);

    _svg_parser_sax_end_element(parser, v);
  }
}

svg_status_t
_svg_parser_end (svg_parser_t *parser)
{
  if (parser->ctxt == NULL)
  	return SVG_STATUS_INVALID_CALL;

  XmlStuff *xs = (XmlStuff *)parser->ctxt;
  xs->xd.Parse(xs->str.Get());

  processElements(parser, xs->xd.FirstChildElement());

  delete xs;
  parser->ctxt = NULL;

  return parser->status;
}

};
