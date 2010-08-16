/* 
   svg_transform.c: Data structure for SVG transformation matrix.
 
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

#include <math.h>
#include <string.h>

#include "svgint.h"

static svg_status_t
_svg_transform_multiply (svg_transform_t	*result,
			 const svg_transform_t	*t1,
			 const svg_transform_t	*t2);

static svg_transform_t SVG_TRANSFORM_IDENTITY = {
    {
	{1, 0},
	{0, 1},
	{0, 0}
    }
};

svg_status_t
_svg_transform_init (svg_transform_t *transform)
{
    *transform = SVG_TRANSFORM_IDENTITY;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_transform_init_matrix (svg_transform_t *transform,
			    double a, double b,
			    double c, double d,
			    double e, double f)
{
    transform->m[0][0] = a; transform->m[0][1] = b;
    transform->m[1][0] = c; transform->m[1][1] = d;
    transform->m[2][0] = e; transform->m[2][1] = f;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_transform_init_translate (svg_transform_t *transform,
			       double tx, double ty)
{
    return _svg_transform_init_matrix (transform,
				       1,  0,
				       0,  1,
				       tx, ty);
}

svg_status_t
_svg_transform_init_scale (svg_transform_t *transform,
			   double sx, double sy)
{
    return _svg_transform_init_matrix (transform,
				       sx, 0,
				       0, sy,
				       0,  0);
}

/* silly SVG spec. uses degrees */
svg_status_t
_svg_transform_init_rotate (svg_transform_t *transform,
			    double angle_degrees)
{
    double rad = M_PI * angle_degrees / 180.0;
    return _svg_transform_init_matrix (transform,
				       cos(rad),  sin(rad),
				       -sin(rad), cos(rad),
				       0,         0);
}

/* silly SVG spec. uses degrees */
svg_status_t
_svg_transform_init_skew_x (svg_transform_t *transform,
			    double angle_degrees)
{
    double rad = M_PI * angle_degrees / 180.0;
    return _svg_transform_init_matrix (transform,
				       1,        0,
				       tan(rad), 1,
				       0,        0);
}

/* silly SVG spec. uses degrees */
svg_status_t
_svg_transform_init_skew_y (svg_transform_t *transform,
			    double angle_degrees)
{
    double rad = M_PI * angle_degrees / 180.0;
    return _svg_transform_init_matrix (transform,
				       1, tan(rad),
				       0,        1,
				       0,        0);
}

svg_status_t
_svg_transform_deinit (svg_transform_t *transform)
{
    return _svg_transform_init (transform);
}

svg_status_t
_svg_transform_add_translate (svg_transform_t *transform,
			      double tx, double ty)
{
    svg_transform_t translate;

    _svg_transform_init_translate (&translate, tx, ty);
    return _svg_transform_multiply_into_right (&translate, transform);
}

svg_status_t
_svg_transform_add_scale_uniform (svg_transform_t *transform, double s)
{
    return _svg_transform_add_scale (transform, s, s);
}

svg_status_t
_svg_transform_add_scale (svg_transform_t *transform,
			  double sx, double sy)
{
    svg_transform_t scale;

    _svg_transform_init_scale (&scale, sx, sy);
    return _svg_transform_multiply_into_right (&scale, transform);
}

/* silly SVG spec. uses degrees */
svg_status_t
_svg_transform_add_rotate (svg_transform_t *transform,
			   double angle_degrees)
{
    svg_transform_t rotate;

    _svg_transform_init_rotate (&rotate, angle_degrees);
    return _svg_transform_multiply_into_right (&rotate, transform);
}

/* silly SVG spec. uses degrees */
svg_status_t
_svg_transform_add_skew_x (svg_transform_t *transform,
			   double angle_degrees)
{
    svg_transform_t skew;

    _svg_transform_init_skew_x (&skew, angle_degrees);
    return _svg_transform_multiply_into_right (&skew, transform);
}

/* silly SVG spec. uses degrees */
svg_status_t
_svg_transform_add_skew_y (svg_transform_t *transform,
			   double angle_degrees)
{
    svg_transform_t skew;

    _svg_transform_init_skew_y (&skew, angle_degrees);
    return _svg_transform_multiply_into_right (&skew, transform);
}


svg_status_t
_svg_transform_multiply_into_left (svg_transform_t *t1, const svg_transform_t *t2)
{
    svg_status_t status;
    svg_transform_t t1_new;

    status = _svg_transform_multiply (&t1_new, t1, t2);

    *t1 = t1_new;

    return status;
}

svg_status_t
_svg_transform_multiply_into_right (const svg_transform_t *t1, svg_transform_t *t2)
{
    svg_status_t status;
    svg_transform_t t2_new;

    status = _svg_transform_multiply (&t2_new, t1, t2);

    *t2 = t2_new;

    return status;
}

static svg_status_t
_svg_transform_multiply (svg_transform_t	*result,
			 const svg_transform_t	*t1,
			 const svg_transform_t	*t2)
{
    int	    row, col, n;
    double  t;

    for (row = 0; row < 3; row++) {
	for (col = 0; col < 2; col++) {
	    if (row == 2)
		t = t2->m[2][col];
	    else
		t = 0;
	    for (n = 0; n < 2; n++) {
		t += t1->m[row][n] * t2->m[n][col];
	    }
	    result->m[row][col] = t;
	}
    }

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_transform_render (svg_transform_t		*transform,
		       svg_render_engine_t	*engine,
		       void			*closure)
{
    return (engine->transform) (closure,
				transform->m[0][0], transform->m[0][1],
				transform->m[1][0], transform->m[1][1],
				transform->m[2][0], transform->m[2][1]);
}


/* The following parse function is:

   Copyright (C) 2000 Eazel, Inc.
   Copyright (C) 2002 Dom Lachowicz <cinamod@hotmail.com>

   Author: Raph Levien <raph@artofcode.com>

   Parse an SVG transform string into an affine matrix. Reference: SVG
   working draft dated 1999-07-06, section 8.5.
*/
extern svg_status_t
_svg_transform_parse_str (svg_transform_t *transform, const char *str)
{
    int idx;
    svg_status_t status;
    char keyword[32];
    double args[6];
    int n_args;
    unsigned int key_len;
    svg_transform_t tmp_transform;

    status = _svg_transform_init (transform);
    if (status)
	return status;

    idx = 0;
    while (str[idx]) {
	/* skip initial whitespace */
	while (_svg_ascii_isspace (str[idx]) || str[idx] == ',')
	    idx++;

	/* parse keyword */
	for (key_len = 0; key_len < sizeof (keyword); key_len++) {
	    char c;

	    c = str[idx];
	    if (_svg_ascii_isalpha (c) || c == '-')
		keyword[key_len] = str[idx++];
	    else
		break;
	}
	/* XXX: This size limitation looks bogus */
	if (key_len >= sizeof (keyword))
	    return SVG_STATUS_PARSE_ERROR;
	keyword[key_len] = '\0';

	/* skip whitespace */
	while (_svg_ascii_isspace (str[idx]))
	    idx++;

	if (str[idx] != '(')
	    return SVG_STATUS_PARSE_ERROR;
	idx++;

	for (n_args = 0; ; n_args++) {
	    char c;
	    const char *end_ptr;

	    /* skip whitespace */
	    while (_svg_ascii_isspace (str[idx]))
		idx++;
	    c = str[idx];
	    if (_svg_ascii_isdigit (c) || c == '+' || c == '-' || c == '.') {
		if (n_args == SVG_ARRAY_SIZE (args))
		    return SVG_STATUS_PARSE_ERROR;
		args[n_args] = _svg_ascii_strtod (str + idx, &end_ptr);
		idx = end_ptr - str;
		
		while (_svg_ascii_isspace (str[idx]))
		    idx++;

		/* skip optional comma */
		if (str[idx] == ',')
		    idx++;
	    } else if (c == ')')
		break;
	    else
		return SVG_STATUS_PARSE_ERROR;
	}
	idx++;

	/* ok, have parsed keyword and args, now modify the transform */
	if (strcmp (keyword, "matrix") == 0) {
	    if (n_args != 6)
		return SVG_STATUS_PARSE_ERROR;
	    _svg_transform_init_matrix (&tmp_transform,
					args[0], args[1],
					args[2], args[3],
					args[4], args[5]);
	    _svg_transform_multiply_into_right (&tmp_transform, transform);
	} else if (strcmp (keyword, "translate") == 0) {
	    if (n_args == 1)
		args[1] = 0;
	    else if (n_args != 2)
		return SVG_STATUS_PARSE_ERROR;
	    _svg_transform_add_translate (transform, args[0], args[1]);
	} else if (strcmp (keyword, "scale") == 0) {
	    if (n_args == 1)
		args[1] = args[0];
	    else if (n_args != 2)
		return SVG_STATUS_PARSE_ERROR;
	    _svg_transform_add_scale (transform, args[0], args[1]);
	} else if (strcmp (keyword, "rotate") == 0) {
	    if (n_args != 1)
		return SVG_STATUS_PARSE_ERROR;
	    _svg_transform_add_rotate (transform, args[0]);
	} else if (strcmp (keyword, "skewX") == 0) {
	    if (n_args != 1)
		return SVG_STATUS_PARSE_ERROR;
	    _svg_transform_add_skew_x (transform, args[0]);
	} else if (strcmp (keyword, "skewY") == 0) {
	    if (n_args != 1)
		return SVG_STATUS_PARSE_ERROR;
	    _svg_transform_add_skew_y (transform, args[0]);
	} else
	    return SVG_STATUS_PARSE_ERROR;
    }

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_transform_apply_attributes (svg_transform_t	*transform,
				 const char		**attributes)
{
    const char *transform_str;

    _svg_attribute_get_string (attributes, "transform", &transform_str, NULL);

    if (transform_str)
	return _svg_transform_parse_str (transform, transform_str);

    return SVG_STATUS_SUCCESS;
}

