/* svg.h: Public interface for libsvg
 
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
_svg_attribute_get_double (const char	**attributes,
			   const char	*name,
			   double	*value,
			   double	default_value)
{
    int i;

    *value = default_value;

    if (attributes == NULL)
	return SVGINT_STATUS_ATTRIBUTE_NOT_FOUND;

    for (i=0; attributes[i]; i += 2) {
	if (strcmp (attributes[i], name) == 0) {
	    *value = _svg_ascii_strtod (attributes[i+1], NULL);
	    return SVG_STATUS_SUCCESS;
	}
    }

    return SVGINT_STATUS_ATTRIBUTE_NOT_FOUND;
}

svgint_status_t
_svg_attribute_get_string (const char	**attributes,
			   const char	*name,
			   const char	**value,
			   const char	*default_value)
{
    int i;

    *value = default_value;

    if (attributes == NULL)
	return SVGINT_STATUS_ATTRIBUTE_NOT_FOUND;

    for (i=0; attributes[i]; i += 2) {
	if (strcmp (attributes[i], name) == 0) {
	    *value = attributes[i+1];
	    return SVG_STATUS_SUCCESS;
	}
    }

    return SVGINT_STATUS_ATTRIBUTE_NOT_FOUND;
}

svgint_status_t
_svg_attribute_get_length (const char	**attributes,
			   const char	*name,
			   svg_length_t	*value,
			   const char	*default_value)
{
    int i;

    _svg_length_init_from_str (value, default_value);

    if (attributes == NULL)
	return SVGINT_STATUS_ATTRIBUTE_NOT_FOUND;

    for (i=0; attributes[i]; i += 2) {
	if (strcmp (attributes[i], name) == 0) {
	    _svg_length_init_from_str (value, attributes[i+1]);
	    return SVG_STATUS_SUCCESS;
	}
    }
    
    return SVGINT_STATUS_ATTRIBUTE_NOT_FOUND;
}
