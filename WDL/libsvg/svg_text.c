/* svg_text.c: Data structures for SVG text elements
 
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
_svg_text_init (svg_text_t *text)
{
    text->chars = NULL;
    text->len = 0;
    _svg_length_init_unit (&text->x, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_HORIZONTAL);
    _svg_length_init_unit (&text->y, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_VERTICAL);

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_text_init_copy (svg_text_t *text,
		     svg_text_t *other)
{
    text->x = other->x;
    text->y = other->y;

    text->len = other->len;
    if (text->len) {
	text->chars = malloc (text->len + 1);
	if (text->chars == NULL)
	    return SVG_STATUS_NO_MEMORY;
	memcpy (text->chars, other->chars, text->len);
	text->chars[text->len] = '\0';
    } else {
	text->chars = NULL;
    }

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_text_deinit (svg_text_t *text)
{
    free (text->chars);
    text->len = 0;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_text_append_chars (svg_text_t	*text,
			const char	*chars,
			int		len)
{
    char *new_chars;

    text->len += len;

    new_chars = realloc (text->chars, text->len + 1);
    if (new_chars == NULL) {
	text->len -= len;
	return SVG_STATUS_NO_MEMORY;
    }

    if (text->chars == NULL)
	new_chars[0] = '\0';
    text->chars = new_chars;
    strncat (text->chars, chars, len);

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_text_render (svg_text_t		*text,
		  svg_render_engine_t	*engine,
		  void			*closure)
{
    return (engine->render_text) (closure,
				  &text->x, &text->y,
				  text->chars);
}

svg_status_t
_svg_text_apply_attributes (svg_text_t		*text,
			    const char		**attributes)
{
    _svg_attribute_get_length (attributes, "x", &text->x, "0");
    _svg_attribute_get_length (attributes, "y", &text->y, "0");

    /* XXX: What else goes here? */

    return SVG_STATUS_SUCCESS;
}
