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

#ifndef SVG_H
#define SVG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct svg svg_t;
typedef struct svg_group svg_group_t;
typedef struct svg_element svg_element_t;

/* XXX: I'm still not convinced I want to export this structure */
typedef struct svg_color {
    int is_current_color;
    unsigned int rgb;
} svg_color_t;

typedef enum svg_length_unit {
    SVG_LENGTH_UNIT_CM,
    SVG_LENGTH_UNIT_EM,
    SVG_LENGTH_UNIT_EX,
    SVG_LENGTH_UNIT_IN,
    SVG_LENGTH_UNIT_MM,
    SVG_LENGTH_UNIT_PC,
    SVG_LENGTH_UNIT_PCT,
    SVG_LENGTH_UNIT_PT,
    SVG_LENGTH_UNIT_PX
} svg_length_unit_t;

typedef enum svg_length_orientation {
    SVG_LENGTH_ORIENTATION_HORIZONTAL,
    SVG_LENGTH_ORIENTATION_VERTICAL,
    SVG_LENGTH_ORIENTATION_OTHER
} svg_length_orientation_t;

typedef struct svg_length {
    double value;
    svg_length_unit_t unit : 4;
    svg_length_orientation_t orientation : 2;
} svg_length_t;

typedef struct svg_rect {
    double x;
    double y;
    double width;
    double height;
} svg_rect_t;

typedef enum svg_preserve_aspect_ratio {
    SVG_PRESERVE_ASPECT_RATIO_UNKNOWN,
    SVG_PRESERVE_ASPECT_RATIO_NONE,
    SVG_PRESERVE_ASPECT_RATIO_XMINYMIN,
    SVG_PRESERVE_ASPECT_RATIO_XMIDYMIN,
    SVG_PRESERVE_ASPECT_RATIO_XMAXYMIN,
    SVG_PRESERVE_ASPECT_RATIO_XMINYMID,
    SVG_PRESERVE_ASPECT_RATIO_XMIDYMID,
    SVG_PRESERVE_ASPECT_RATIO_XMAXYMID,
    SVG_PRESERVE_ASPECT_RATIO_XMINYMAX,
    SVG_PRESERVE_ASPECT_RATIO_XMIDYMAX,
    SVG_PRESERVE_ASPECT_RATIO_XMAXYMAX
} svg_preserve_aspect_ratio_t;

typedef enum svg_meet_or_slice {
   SVG_MEET_OR_SLICE_UNKNOWN,
   SVG_MEET_OR_SLICE_MEET,
   SVG_MEET_OR_SLICE_SLICE
} svg_meet_or_slice_t;

typedef struct svg_view_box {
    svg_rect_t box;
    svg_preserve_aspect_ratio_t aspect_ratio : 4;
    svg_meet_or_slice_t meet_or_slice : 2;
} svg_view_box_t;

/* XXX: I want to think very carefully about the names of these error
   messages. First, they should follow the SVG spec. closely. Second,
   which errors do we really want? */
typedef enum svg_status {
    SVG_STATUS_SUCCESS = 0,
    SVG_STATUS_NO_MEMORY,
    SVG_STATUS_IO_ERROR,
    SVG_STATUS_FILE_NOT_FOUND,
    SVG_STATUS_INVALID_VALUE,
    SVG_STATUS_INVALID_CALL,
    SVG_STATUS_PARSE_ERROR
} svg_status_t;

typedef enum svg_fill_rule {
    SVG_FILL_RULE_NONZERO,
    SVG_FILL_RULE_EVEN_ODD
} svg_fill_rule_t;

typedef enum svg_font_style {
    SVG_FONT_STYLE_NORMAL,
    SVG_FONT_STYLE_ITALIC,
    SVG_FONT_STYLE_OBLIQUE
} svg_font_style_t;

typedef enum svg_stroke_line_cap {
    SVG_STROKE_LINE_CAP_BUTT,
    SVG_STROKE_LINE_CAP_ROUND,
    SVG_STROKE_LINE_CAP_SQUARE
} svg_stroke_line_cap_t;

typedef enum svg_stroke_line_join {
    SVG_STROKE_LINE_JOIN_BEVEL,
    SVG_STROKE_LINE_JOIN_MITER,
    SVG_STROKE_LINE_JOIN_ROUND
} svg_stroke_line_join_t;

typedef enum svg_text_anchor {
    SVG_TEXT_ANCHOR_START,
    SVG_TEXT_ANCHOR_MIDDLE,
    SVG_TEXT_ANCHOR_END
} svg_text_anchor_t;

typedef enum svg_gradient_type_t {
    SVG_GRADIENT_LINEAR,
    SVG_GRADIENT_RADIAL
} svg_gradient_type_t;

typedef enum svg_gradient_units {
    SVG_GRADIENT_UNITS_USER,
    SVG_GRADIENT_UNITS_BBOX
} svg_gradient_units_t;

typedef enum svg_gradient_spread {
    SVG_GRADIENT_SPREAD_PAD,
    SVG_GRADIENT_SPREAD_REPEAT,
    SVG_GRADIENT_SPREAD_REFLECT
} svg_gradient_spread_t;

typedef struct svg_gradient_stop {
    svg_color_t color;
    double offset;
    double opacity;
} svg_gradient_stop_t;

typedef struct svg_gradient {
    svg_gradient_type_t type;
    union {
	struct {
	    svg_length_t x1;
	    svg_length_t y1;
	    svg_length_t x2;
	    svg_length_t y2;
	} linear;
	struct {
	    svg_length_t cx;
	    svg_length_t cy;
	    svg_length_t r;
	    svg_length_t fx;
	    svg_length_t fy;
	} radial;
    } u;
    svg_gradient_units_t units;
    svg_gradient_spread_t spread;
    /* XXX: Why not just a regular svg_transform here? */
    double transform[6];

    svg_gradient_stop_t *stops;
    int num_stops;
    int stops_size;
} svg_gradient_t;

typedef enum svg_pattern_units {
    SVG_PATTERN_UNITS_USER,
    SVG_PATTERN_UNITS_BBOX
} svg_pattern_units_t;

/* XXX: I'm still not convinced I want to export this structure */
typedef struct svg_pattern {
    svg_element_t *group_element;
    svg_pattern_units_t units;
    svg_pattern_units_t content_units;
    svg_length_t x, y, width, height;
    double transform[6];
} svg_pattern_t;

/* XXX: I'm still not convinced I want to export this structure */
/* XXX: This needs to be fleshed out considerably more than this */
typedef enum svg_paint_type {
    SVG_PAINT_TYPE_NONE,
    SVG_PAINT_TYPE_COLOR,
    SVG_PAINT_TYPE_GRADIENT,
    SVG_PAINT_TYPE_PATTERN
} svg_paint_type_t;

/* XXX: I'm still not convinced I want to export this structure */
typedef struct svg_paint {
    svg_paint_type_t type;
    union {
	svg_color_t	color;
	svg_gradient_t	*gradient;
	svg_element_t	*pattern_element;
    } p;
} svg_paint_t;

/* XXX: Here's another piece of the API that needs deep consideration. */
typedef struct svg_render_engine {
    /* hierarchy */
    svg_status_t (* begin_group) (void *closure, double opacity);
    svg_status_t (* begin_element) (void *closure);
    svg_status_t (* end_element) (void *closure);
    svg_status_t (* end_group) (void *closure, double opacity);
    /* path creation */
    svg_status_t (* move_to) (void *closure, double x, double y);
    svg_status_t (* line_to) (void *closure, double x, double y);
    svg_status_t (* curve_to) (void *closure,
			       double x1, double y1,
			       double x2, double y2,
			       double x3, double y3);
    svg_status_t (* quadratic_curve_to) (void *closure,
					 double x1, double y1,
					 double x2, double y2);
    svg_status_t (* arc_to) (void *closure,
		    	     double	rx,
			     double	ry,
			     double	x_axis_rotation,
			     int	large_arc_flag,
			     int	sweep_flag,
			     double	x,
			     double	y);
    svg_status_t (* close_path) (void *closure);
    /* style */
    svg_status_t (* set_color) (void *closure, const svg_color_t *color);
    svg_status_t (* set_fill_opacity) (void *closure, double fill_opacity);
    svg_status_t (* set_fill_paint) (void *closure, const svg_paint_t *paint);
    svg_status_t (* set_fill_rule) (void *closure, svg_fill_rule_t fill_rule);
    svg_status_t (* set_font_family) (void *closure, const char *family);
    svg_status_t (* set_font_size) (void *closure, double size);
    svg_status_t (* set_font_style) (void *closure, svg_font_style_t font_style);
    svg_status_t (* set_font_weight) (void *closure, unsigned int font_weight);
    svg_status_t (* set_opacity) (void *closure, double opacity);
    svg_status_t (* set_stroke_dash_array) (void *closure, double *dash_array, int num_dashes);
    svg_status_t (* set_stroke_dash_offset) (void *closure, svg_length_t *offset);
    svg_status_t (* set_stroke_line_cap) (void *closure, svg_stroke_line_cap_t line_cap);
    svg_status_t (* set_stroke_line_join) (void *closure, svg_stroke_line_join_t line_join);
    svg_status_t (* set_stroke_miter_limit) (void *closure, double limit);
    svg_status_t (* set_stroke_opacity) (void *closure, double stroke_opacity);
    svg_status_t (* set_stroke_paint) (void *closure, const svg_paint_t *paint);
    svg_status_t (* set_stroke_width) (void *closure, svg_length_t *width);
    svg_status_t (* set_text_anchor) (void *closure, svg_text_anchor_t text_anchor);
    /* transform */
    svg_status_t (* transform) (void *closure,
				double a, double b,
				double c, double d,
				double e, double f);
    svg_status_t (* apply_view_box) (void *closure,
				     svg_view_box_t view_box,
				     svg_length_t *width,
				     svg_length_t *height);
    svg_status_t (* set_viewport_dimension) (void *closure,
				    	     svg_length_t *width,
				    	     svg_length_t *height);
    /* drawing */
    svg_status_t (* render_line) (void *closure,
				  svg_length_t *x1,
				  svg_length_t *y1,
				  svg_length_t *x2,
				  svg_length_t *y2);
    svg_status_t (* render_path) (void *closure);
    svg_status_t (* render_ellipse) (void *closure,
				     svg_length_t *cx,
				     svg_length_t *cy,
				     svg_length_t *rx,
				     svg_length_t *ry);
    svg_status_t (* render_rect) (void *closure,
				     svg_length_t *x,
				     svg_length_t *y,
				     svg_length_t *width,
				     svg_length_t *height,
				     svg_length_t *rx,
				     svg_length_t *ry);
    svg_status_t (* render_text) (void *closure,
				  svg_length_t *x,
				  svg_length_t *y,
				  const char   *utf8);
    svg_status_t (* render_image) (void		 *closure,
				   unsigned char *data,
				   unsigned int	 data_width,
				   unsigned int	 data_height,
				   svg_length_t	 *x,
				   svg_length_t	 *y,
				   svg_length_t	 *width,
				   svg_length_t	 *height);
} svg_render_engine_t;

svg_status_t
svg_create (svg_t **svg);

svg_status_t
svg_destroy (svg_t *svg);

svg_status_t
svg_parse (svg_t *svg, const char *filename);

svg_status_t
svg_parse_file (svg_t *svg, FILE *file);

svg_status_t
svg_parse_buffer (svg_t *svg, const char *buf, size_t count);

svg_status_t
svg_parse_chunk_begin (svg_t *svg);
svg_status_t
svg_parse_chunk       (svg_t *svg, const char *buf, size_t count);
svg_status_t
svg_parse_chunk_end   (svg_t *svg);

svg_status_t
svg_render (svg_t		*svg,
	    svg_render_engine_t	*engine,
	    void		*closure);

void
svg_get_size (svg_t *svg,
	      svg_length_t *width,
	      svg_length_t *height);

/* svg_color */

unsigned int
svg_color_get_red (const svg_color_t *color);

unsigned int
svg_color_get_green (const svg_color_t *color);

unsigned int
svg_color_get_blue (const svg_color_t *color);

/* svg_element */

svg_status_t
svg_element_render (svg_element_t		*element,
		    svg_render_engine_t	*engine,
		    void			*closure);

svg_pattern_t *
svg_element_pattern (svg_element_t *element);

#ifdef __cplusplus
}
#endif

#endif
