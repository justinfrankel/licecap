/* svg_color.c: Data structures for SVG colors
 
   Copyright (C) 2000 Eazel, Inc.
  
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

#include <string.h>
#include <math.h>

#include "svgint.h"

static int
_svg_color_cmp (const void * a, const void * b);

static unsigned int
_svg_color_get_hex_digit (const char *str);

static unsigned int
_svg_color_get_two_hex_digits (const char *str);

static svg_status_t
_svg_color_parse_component (const char **str, unsigned int *component);

typedef struct svg_color_map {
    const char *name;
    svg_color_t color;
} svg_color_map_t;

/* pack 3 [0,255] ints into one 32 bit one */
#define PACK_RGB(r,g,b) (((r) << 16) | ((g) << 8) | (b))

static const svg_color_map_t SVG_COLOR_MAP[] = {
    { "aliceblue",            { 0, PACK_RGB (240,248,255) }},
    { "antiquewhite",         { 0, PACK_RGB (250,235,215) }},
    { "aqua",                 { 0, PACK_RGB (  0,255,255) }},
    { "aquamarine",           { 0, PACK_RGB (127,255,212) }},
    { "azure",                { 0, PACK_RGB (240,255,255) }},
    { "beige",                { 0, PACK_RGB (245,245,220) }},
    { "bisque",               { 0, PACK_RGB (255,228,196) }},
    { "black",                { 0, PACK_RGB (  0,  0,  0) }},
    { "blanchedalmond",       { 0, PACK_RGB (255,235,205) }},
    { "blue",                 { 0, PACK_RGB (  0,  0,255) }},
    { "blueviolet",           { 0, PACK_RGB (138, 43,226) }},
    { "brown",                { 0, PACK_RGB (165, 42, 42) }},
    { "burlywood",            { 0, PACK_RGB (222,184,135) }},
    { "cadetblue",            { 0, PACK_RGB ( 95,158,160) }},
    { "chartreuse",           { 0, PACK_RGB (127,255,  0) }},
    { "chocolate",            { 0, PACK_RGB (210,105, 30) }},
    { "coral",                { 0, PACK_RGB (255,127, 80) }},
    { "cornflowerblue",       { 0, PACK_RGB (100,149,237) }},
    { "cornsilk",             { 0, PACK_RGB (255,248,220) }},
    { "crimson",              { 0, PACK_RGB (220, 20, 60) }},
    { "cyan",                 { 0, PACK_RGB (  0,255,255) }},
    { "darkblue",             { 0, PACK_RGB (  0,  0,139) }},
    { "darkcyan",             { 0, PACK_RGB (  0,139,139) }},
    { "darkgoldenrod",        { 0, PACK_RGB (184,132, 11) }},
    { "darkgray",             { 0, PACK_RGB (169,169,168) }},
    { "darkgreen",            { 0, PACK_RGB (  0,100,  0) }},
    { "darkgrey",             { 0, PACK_RGB (169,169,169) }},
    { "darkkhaki",            { 0, PACK_RGB (189,183,107) }},
    { "darkmagenta",          { 0, PACK_RGB (139,  0,139) }},
    { "darkolivegreen",       { 0, PACK_RGB ( 85,107, 47) }},
    { "darkorange",           { 0, PACK_RGB (255,140,  0) }},
    { "darkorchid",           { 0, PACK_RGB (153, 50,204) }},
    { "darkred",              { 0, PACK_RGB (139,  0,  0) }},
    { "darksalmon",           { 0, PACK_RGB (233,150,122) }},
    { "darkseagreen",         { 0, PACK_RGB (143,188,143) }},
    { "darkslateblue",        { 0, PACK_RGB ( 72, 61,139) }},
    { "darkslategray",        { 0, PACK_RGB ( 47, 79, 79) }},
    { "darkslategrey",        { 0, PACK_RGB ( 47, 79, 79) }},
    { "darkturquoise",        { 0, PACK_RGB (  0,206,209) }},
    { "darkviolet",           { 0, PACK_RGB (148,  0,211) }},
    { "deeppink",             { 0, PACK_RGB (255, 20,147) }},
    { "deepskyblue",          { 0, PACK_RGB (  0,191,255) }},
    { "dimgray",              { 0, PACK_RGB (105,105,105) }},
    { "dimgrey",              { 0, PACK_RGB (105,105,105) }},
    { "dodgerblue",           { 0, PACK_RGB ( 30,144,255) }},
    { "firebrick",            { 0, PACK_RGB (178, 34, 34) }},
    { "floralwhite" ,         { 0, PACK_RGB (255,255,240) }},
    { "forestgreen",          { 0, PACK_RGB ( 34,139, 34) }},
    { "fuchsia",              { 0, PACK_RGB (255,  0,255) }},
    { "gainsboro",            { 0, PACK_RGB (220,220,220) }},
    { "ghostwhite",           { 0, PACK_RGB (248,248,255) }},
    { "gold",                 { 0, PACK_RGB (215,215,  0) }},
    { "goldenrod",            { 0, PACK_RGB (218,165, 32) }},
    { "gray",                 { 0, PACK_RGB (128,128,128) }},
    { "grey",                 { 0, PACK_RGB (128,128,128) }},
    { "green",                { 0, PACK_RGB (  0,128,  0) }},
    { "greenyellow",          { 0, PACK_RGB (173,255, 47) }},
    { "honeydew",             { 0, PACK_RGB (240,255,240) }},
    { "hotpink",              { 0, PACK_RGB (255,105,180) }},
    { "indianred",            { 0, PACK_RGB (205, 92, 92) }},
    { "indigo",               { 0, PACK_RGB ( 75,  0,130) }},
    { "ivory",                { 0, PACK_RGB (255,255,240) }},
    { "khaki",                { 0, PACK_RGB (240,230,140) }},
    { "lavender",             { 0, PACK_RGB (230,230,250) }},
    { "lavenderblush",        { 0, PACK_RGB (255,240,245) }},
    { "lawngreen",            { 0, PACK_RGB (124,252,  0) }},
    { "lemonchiffon",         { 0, PACK_RGB (255,250,205) }},
    { "lightblue",            { 0, PACK_RGB (173,216,230) }},
    { "lightcoral",           { 0, PACK_RGB (240,128,128) }},
    { "lightcyan",            { 0, PACK_RGB (224,255,255) }},
    { "lightgoldenrodyellow", { 0, PACK_RGB (250,250,210) }},
    { "lightgray",            { 0, PACK_RGB (211,211,211) }},
    { "lightgreen",           { 0, PACK_RGB (144,238,144) }},
    { "lightgrey",            { 0, PACK_RGB (211,211,211) }},
    { "lightpink",            { 0, PACK_RGB (255,182,193) }},
    { "lightsalmon",          { 0, PACK_RGB (255,160,122) }},
    { "lightseagreen",        { 0, PACK_RGB ( 32,178,170) }},
    { "lightskyblue",         { 0, PACK_RGB (135,206,250) }},
    { "lightslategray",       { 0, PACK_RGB (119,136,153) }},
    { "lightslategrey",       { 0, PACK_RGB (119,136,153) }},
    { "lightsteelblue",       { 0, PACK_RGB (176,196,222) }},
    { "lightyellow",          { 0, PACK_RGB (255,255,224) }},
    { "lime",                 { 0, PACK_RGB (  0,255,  0) }},
    { "limegreen",            { 0, PACK_RGB ( 50,205, 50) }},
    { "linen",                { 0, PACK_RGB (250,240,230) }},
    { "magenta",              { 0, PACK_RGB (255,  0,255) }},
    { "maroon",               { 0, PACK_RGB (128,  0,  0) }},
    { "mediumaquamarine",     { 0, PACK_RGB (102,205,170) }},
    { "mediumblue",           { 0, PACK_RGB (  0,  0,205) }},
    { "mediumorchid",         { 0, PACK_RGB (186, 85,211) }},
    { "mediumpurple",         { 0, PACK_RGB (147,112,219) }},
    { "mediumseagreen",       { 0, PACK_RGB ( 60,179,113) }},
    { "mediumslateblue",      { 0, PACK_RGB (123,104,238) }},
    { "mediumspringgreen",    { 0, PACK_RGB (  0,250,154) }},
    { "mediumturquoise",      { 0, PACK_RGB ( 72,209,204) }},
    { "mediumvioletred",      { 0, PACK_RGB (199, 21,133) }},
    { "mediumnightblue",      { 0, PACK_RGB ( 25, 25,112) }},
    { "mintcream",            { 0, PACK_RGB (245,255,250) }},
    { "mintyrose",            { 0, PACK_RGB (255,228,225) }},
    { "moccasin",             { 0, PACK_RGB (255,228,181) }},
    { "navajowhite",          { 0, PACK_RGB (255,222,173) }},
    { "navy",                 { 0, PACK_RGB (  0,  0,128) }},
    { "oldlace",              { 0, PACK_RGB (253,245,230) }},
    { "olive",                { 0, PACK_RGB (128,128,  0) }},
    { "olivedrab",            { 0, PACK_RGB (107,142, 35) }},
    { "orange",               { 0, PACK_RGB (255,165,  0) }},
    { "orangered",            { 0, PACK_RGB (255, 69,  0) }},
    { "orchid",               { 0, PACK_RGB (218,112,214) }},
    { "palegoldenrod",        { 0, PACK_RGB (238,232,170) }},
    { "palegreen",            { 0, PACK_RGB (152,251,152) }},
    { "paleturquoise",        { 0, PACK_RGB (175,238,238) }},
    { "palevioletred",        { 0, PACK_RGB (219,112,147) }},
    { "papayawhip",           { 0, PACK_RGB (255,239,213) }},
    { "peachpuff",            { 0, PACK_RGB (255,218,185) }},
    { "peru",                 { 0, PACK_RGB (205,133, 63) }},
    { "pink",                 { 0, PACK_RGB (255,192,203) }},
    { "plum",                 { 0, PACK_RGB (221,160,203) }},
    { "powderblue",           { 0, PACK_RGB (176,224,230) }},
    { "purple",               { 0, PACK_RGB (128,  0,128) }},
    { "red",                  { 0, PACK_RGB (255,  0,  0) }},
    { "rosybrown",            { 0, PACK_RGB (188,143,143) }},
    { "royalblue",            { 0, PACK_RGB ( 65,105,225) }},
    { "saddlebrown",          { 0, PACK_RGB (139, 69, 19) }},
    { "salmon",               { 0, PACK_RGB (250,128,114) }},
    { "sandybrown",           { 0, PACK_RGB (244,164, 96) }},
    { "seagreen",             { 0, PACK_RGB ( 46,139, 87) }},
    { "seashell",             { 0, PACK_RGB (255,245,238) }},
    { "sienna",               { 0, PACK_RGB (160, 82, 45) }},
    { "silver",               { 0, PACK_RGB (192,192,192) }},
    { "skyblue",              { 0, PACK_RGB (135,206,235) }},
    { "slateblue",            { 0, PACK_RGB (106, 90,205) }},
    { "slategray",            { 0, PACK_RGB (112,128,144) }},
    { "slategrey",            { 0, PACK_RGB (112,128,114) }},
    { "snow",                 { 0, PACK_RGB (255,255,250) }},
    { "springgreen",          { 0, PACK_RGB (  0,255,127) }},
    { "steelblue",            { 0, PACK_RGB ( 70,130,180) }},
    { "tan",                  { 0, PACK_RGB (210,180,140) }},
    { "teal",                 { 0, PACK_RGB (  0,128,128) }},
    { "thistle",              { 0, PACK_RGB (216,191,216) }},
    { "tomato",               { 0, PACK_RGB (255, 99, 71) }},
    { "turquoise",            { 0, PACK_RGB ( 64,224,208) }},
    { "violet",               { 0, PACK_RGB (238,130,238) }},
    { "wheat",                { 0, PACK_RGB (245,222,179) }},
    { "white",                { 0, PACK_RGB (255,255,255) }},
    { "whitesmoke",           { 0, PACK_RGB (245,245,245) }},
    { "yellow",               { 0, PACK_RGB (255,255,  0) }},
    { "yellowgreen",          { 0, PACK_RGB (154,205, 50) }}
};
#undef PACK_RGB

svg_status_t
_svg_color_init_rgb (svg_color_t *color,
		     unsigned int r,
		     unsigned int g,
		     unsigned int b)
{
    color->rgb = (((r) << 16) | ((g) << 8) | (b));

    return SVG_STATUS_SUCCESS;
}

/* compare function callback for bsearch */
static int
_svg_color_cmp (const void * a, const void * b)
{
    const char * needle = a;
    const svg_color_map_t *haystack = b;
    
    return _svg_ascii_strcasecmp (needle, haystack->name);
}

static unsigned int
_svg_color_get_hex_digit (const char *str)
{
    if (*str >= '0' && *str <= '9')
	return *str - '0';
    else if (*str >= 'A' && *str <= 'F')
	return *str - 'A' + 10;
    else if (*str >= 'a' && *str <= 'f')
	return *str - 'a' + 10;
    else
	return 0;
}

static unsigned int
_svg_color_get_two_hex_digits (const char *str)
{
    int i;
    unsigned int hex, ret;

    ret = 0;
    for (i = 0; i < 2 && str[i]; i++) {
	ret <<= 4;
	hex = _svg_color_get_hex_digit (str+i);
	ret += hex;
    }

    return ret;
}

static svg_status_t
_svg_color_parse_component (const char **str, unsigned int *component)
{
    const char *s, *end;
    double c;

    s = *str;

    c = _svg_ascii_strtod (s, &end);
    if (end == s)
	return SVG_STATUS_PARSE_ERROR;
    s = end;

    _svg_str_skip_space (&s);

    if (*s == '%') {
	c *= 2.55;
	s++;
    }

    _svg_str_skip_space (&s);

    if (c > 255)
	c = 255;
    else if (c < 0)
	c = 0;

    *component = (unsigned int) c;
    *str = s;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_color_init_from_str (svg_color_t *color, const char *str)
{
    unsigned int r=0, g=0, b=0;
    svg_status_t status;
    svg_color_map_t *map;

    /* XXX: Need to check SVG spec. for this error case */
    if (str == NULL || str[0] == '\0')
	return _svg_color_init_rgb (color, 0, 0, 0);

    if (strcmp (str, "currentColor") == 0) {
	_svg_color_init_rgb (color, 0, 0, 0);
	color->is_current_color = 1;
	return SVG_STATUS_SUCCESS;
    }

    color->is_current_color = 0;

    if (str[0] == '#') {
	str++;
	if (strlen(str) >= 6) {
	    r = _svg_color_get_two_hex_digits (str);
	    str += 2;
	    g = _svg_color_get_two_hex_digits (str);
	    str += 2;
	    b = _svg_color_get_two_hex_digits (str);
	} else if (strlen(str) >= 3) {
	    r = _svg_color_get_hex_digit (str);
	    r += (r << 4);
	    str++;
	    g = _svg_color_get_hex_digit (str);
	    g += (g << 4);
	    str++;
	    b = _svg_color_get_hex_digit (str);
	    b += (b << 4);
	}

	return _svg_color_init_rgb (color, r, g, b);
    }

    _svg_str_skip_space (&str);
    if (strncmp (str, "rgb", 3) == 0) {
	str += 3;

	_svg_str_skip_space (&str);
	_svg_str_skip_char (&str, '(');
	status = _svg_color_parse_component (&str, &r);
	if (status)
	    return status;
	_svg_str_skip_char (&str, ',');
	status = _svg_color_parse_component (&str, &g);
	if (status)
	    return status;
	_svg_str_skip_char (&str, ',');
	status = _svg_color_parse_component (&str, &b);
	if (status)
	    return status;
	_svg_str_skip_char (&str, ')');

	return _svg_color_init_rgb (color, r, g, b);
    }

    map = bsearch (str, SVG_COLOR_MAP,
		   SVG_ARRAY_SIZE(SVG_COLOR_MAP),
		   sizeof (svg_color_map_t),
		   _svg_color_cmp);
    
    /* default to black on failed lookup */
    if (map == NULL)
	return _svg_color_init_rgb (color, 0, 0, 0);

    *color = map->color;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_color_deinit (svg_color_t *color)
{
    return SVG_STATUS_SUCCESS;
}

unsigned int
svg_color_get_red (const svg_color_t *color)
{
    return (color->rgb & 0xff0000) >> 16;
}

unsigned int
svg_color_get_green (const svg_color_t *color)
{
    return (color->rgb & 0xff00) >> 8;
}

unsigned int
svg_color_get_blue (const svg_color_t *color)
{
    return (color->rgb & 0xff);
}
