/* svgint.h: Internal interfaces for libsvg

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

#ifndef SVGINT_H
#define SVGINT_H

#if 0
#ifdef LIBSVG_EXPAT

#include <expat.h>
#include "svg_hash.h"

typedef XML_Char xmlChar;
typedef XML_Parser svg_xml_parser_context_t;

#else

#include <libxml/SAX.h>
#include <libxml/xmlmemory.h>
#include <libxml/hash.h>

typedef xmlParserCtxtPtr svg_xml_parser_context_t;
typedef xmlHashTable svg_xml_hash_table_t;

#define _svg_xml_hash_add_entry		xmlHashAddEntry
#define _svg_xml_hash_lookup		xmlHashLookup
#define _svg_xml_hash_create		xmlHashCreate
#define _svg_xml_hash_free(_table)	xmlHashFree(_table, NULL)

#endif
#endif

//small tiny replacement xml parsing lib
typedef int svg_xml_parser_context_t;
#include "svg_hash.h"
typedef char xmlChar;

#ifdef WIN32
#include <windows.h>
#include <io.h>
#include <direct.h>
#define MAXPATHLEN MAX_PATH
#define M_PI 3.14159265
#endif
#include <malloc.h>

#include "config.h"
#include "svg_version.h"
#include "svg.h"
#include "svg_ascii.h"

#define SVG_ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

/* sure wish C had a real enum so that this type would be
   distinguishable from svg_status_t. In the meantime, we'll use the
   otherwise bogus 1000 value. */
typedef enum svgint_status {
    SVGINT_STATUS_BROKEN_IMPLEMENTATION = 1000,
    SVGINT_STATUS_ARGS_EXHAUSTED,
    SVGINT_STATUS_UNKNOWN_ELEMENT,
    SVGINT_STATUS_ATTRIBUTE_NOT_FOUND,
    SVGINT_STATUS_IMAGE_NOT_PNG,
    SVGINT_STATUS_IMAGE_NOT_JPEG,
    SVGINT_STATUS_UNDEFINED_RESULT
} svgint_status_t;

typedef struct svg_pt {
    double x;
    double y;
} svg_pt_t;

/* All commands that may appear in a path data string */
typedef enum svg_path_cmd {
    SVG_PATH_CMD_MOVE_TO,
    SVG_PATH_CMD_REL_MOVE_TO,
    SVG_PATH_CMD_LINE_TO,
    SVG_PATH_CMD_REL_LINE_TO,
    SVG_PATH_CMD_HORIZONTAL_LINE_TO,
    SVG_PATH_CMD_REL_HORIZONTAL_LINE_TO,
    SVG_PATH_CMD_VERTICAL_LINE_TO,
    SVG_PATH_CMD_REL_VERTICAL_LINE_TO,
    SVG_PATH_CMD_CURVE_TO,
    SVG_PATH_CMD_REL_CURVE_TO,
    SVG_PATH_CMD_SMOOTH_CURVE_TO,
    SVG_PATH_CMD_REL_SMOOTH_CURVE_TO,
    SVG_PATH_CMD_QUADRATIC_CURVE_TO,
    SVG_PATH_CMD_REL_QUADRATIC_CURVE_TO,
    SVG_PATH_CMD_SMOOTH_QUADRATIC_CURVE_TO,
    SVG_PATH_CMD_REL_SMOOTH_QUADRATIC_CURVE_TO,
    SVG_PATH_CMD_ARC_TO,
    SVG_PATH_CMD_REL_ARC_TO,
    SVG_PATH_CMD_CLOSE_PATH
} svg_path_cmd_t;

/* Primitive path operators. All of the above svg_path_cmd_t commands
   are stored internally as a sequence of the following svg_path_op_t
   operators.
   
   We synch up the numeric values of these two types as an added
   measure of safety since C doesn't have a real enum type.
*/
typedef enum svg_path_op {
    SVG_PATH_OP_MOVE_TO		= SVG_PATH_CMD_MOVE_TO,
    SVG_PATH_OP_LINE_TO		= SVG_PATH_CMD_LINE_TO,
    SVG_PATH_OP_CURVE_TO   	= SVG_PATH_CMD_CURVE_TO,
    SVG_PATH_OP_QUAD_TO   	= SVG_PATH_CMD_QUADRATIC_CURVE_TO,
    SVG_PATH_OP_ARC_TO   	= SVG_PATH_CMD_ARC_TO,
    SVG_PATH_OP_CLOSE_PATH	= SVG_PATH_CMD_CLOSE_PATH
} svg_path_op_t;

#define SVG_PATH_BUF_SZ 64

typedef struct svg_path_op_buf {
    int num_ops;
    svg_path_op_t op[SVG_PATH_BUF_SZ];

    struct svg_path_op_buf *next, *prev;
} svg_path_op_buf_t;

typedef struct svg_path_arg_buf {
    int num_args;
    double arg[SVG_PATH_BUF_SZ];

    struct svg_path_arg_buf *next, *prev;
} svg_path_arg_buf_t;

typedef struct svg_path {
    svg_pt_t last_move_pt;
    svg_pt_t current_pt;

    svg_path_op_t last_path_op;
    svg_pt_t reflected_cubic_pt;
    svg_pt_t reflected_quad_pt;

    svg_path_op_buf_t *op_head;
    svg_path_op_buf_t *op_tail;

    svg_path_arg_buf_t *arg_head;
    svg_path_arg_buf_t *arg_tail;
} svg_path_t;

#define SVG_STYLE_FLAG_NONE				0x00000000000
#define SVG_STYLE_FLAG_CLIP_RULE			0x00000000001
#define SVG_STYLE_FLAG_COLOR				0x00000000002
#define SVG_STYLE_FLAG_COLOR_INTERPOLATION		0x00000000004
#define SVG_STYLE_FLAG_COLOR_INTERPOLATION_FILTERS	0x00000000008
#define SVG_STYLE_FLAG_COLOR_PROFILE			0x00000000010
#define SVG_STYLE_FLAG_COLOR_RENDERING			0x00000000020
#define SVG_STYLE_FLAG_CURSOR				0x00000000040
#define SVG_STYLE_FLAG_DIRECTION			0x00000000080
#define SVG_STYLE_FLAG_DISPLAY				0x00000000100
#define SVG_STYLE_FLAG_FILL_OPACITY			0x00000000200
#define SVG_STYLE_FLAG_FILL_PAINT			0x00000000400
#define SVG_STYLE_FLAG_FILL_RULE			0x00000000800
#define SVG_STYLE_FLAG_FONT_FAMILY			0x00000001000
#define SVG_STYLE_FLAG_FONT_SIZE			0x00000002000
#define SVG_STYLE_FLAG_FONT_SIZE_ADJUST			0x00000004000
#define SVG_STYLE_FLAG_FONT_STRETCH			0x00000008000
#define SVG_STYLE_FLAG_FONT_STYLE			0x00000010000
#define SVG_STYLE_FLAG_FONT_VARIANT			0x00000020000
#define SVG_STYLE_FLAG_FONT_WEIGHT			0x00000040000
#define SVG_STYLE_FLAG_GLYPH_ORIENTATION_HORIZONTAL	0x00000080000
#define SVG_STYLE_FLAG_GLYPH_ORIENTATION_VERTICAL	0x00000100000
#define SVG_STYLE_FLAG_IMAGE_RENDERING			0x00000200000
#define SVG_STYLE_FLAG_KERNING				0x00000400000
#define SVG_STYLE_FLAG_LETTER_SPACING			0x00000800000
#define SVG_STYLE_FLAG_MARKER_END			0x00001000000
#define SVG_STYLE_FLAG_MARKER_MID			0x00002000000
#define SVG_STYLE_FLAG_MARKER_START			0x00004000000
#define SVG_STYLE_FLAG_OPACITY				0x00008000000
#define SVG_STYLE_FLAG_POINTER_EVENTS			0x00010000000
#define SVG_STYLE_FLAG_SHAPE_RENDERING			0x00020000000
#define SVG_STYLE_FLAG_STROKE_DASH_ARRAY		0x00040000000
#define SVG_STYLE_FLAG_STROKE_DASH_OFFSET		0x00080000000
#define SVG_STYLE_FLAG_STROKE_LINE_CAP			0x00100000000
#define SVG_STYLE_FLAG_STROKE_LINE_JOIN			0x00200000000
#define SVG_STYLE_FLAG_STROKE_MITER_LIMIT		0x00400000000
#define SVG_STYLE_FLAG_STROKE_OPACITY			0x00800000000
#define SVG_STYLE_FLAG_STROKE_PAINT			0x01000000000
#define SVG_STYLE_FLAG_STROKE_WIDTH			0x02000000000
#define SVG_STYLE_FLAG_TEXT_ANCHOR			0x04000000000
#define SVG_STYLE_FLAG_TEXT_RENDERING			0x08000000000
#define SVG_STYLE_FLAG_VISIBILITY			0x10000000000
#define SVG_STYLE_FLAG_WORD_SPACING			0x20000000000
#define SVG_STYLE_FLAG_WRITING_MODE			0x40000000000

typedef struct svg_style {
    svg_t				*svg;

    uint64_t				flags;

    double				fill_opacity;
    svg_paint_t				fill_paint;
    svg_fill_rule_t			fill_rule;

    char				*font_family;
    svg_length_t			font_size;
    svg_font_style_t			font_style;
    unsigned int			font_weight;

    double				opacity;

    double				*stroke_dash_array;
    int					num_dashes;
    svg_length_t			stroke_dash_offset;

    svg_stroke_line_cap_t		stroke_line_cap;
    svg_stroke_line_join_t		stroke_line_join;
    double				stroke_miter_limit;
    double				stroke_opacity;
    svg_paint_t				stroke_paint;
    svg_length_t			stroke_width;

    svg_color_t				color;
    svg_text_anchor_t			text_anchor;
} svg_style_t;

typedef struct svg_transform {
    double m[3][2];
} svg_transform_t;

struct svg_group {
    svg_element_t **element;
    int num_elements;
    int element_size;

    svg_length_t width;
    svg_length_t height;
    svg_view_box_t view_box;
    svg_length_t x;
    svg_length_t y;
};

typedef struct svg_text {
    svg_length_t x;
    svg_length_t y;
    char *chars;
    unsigned int len;
} svg_text_t;

typedef struct svg_ellipse {
    svg_length_t cx;
    svg_length_t cy;
    svg_length_t rx;
    svg_length_t ry;
} svg_ellipse_t;

typedef struct svg_line {
    svg_length_t x1;
    svg_length_t y1;
    svg_length_t x2;
    svg_length_t y2;
} svg_line_t;

typedef struct svg_rect_element {
    svg_length_t x;
    svg_length_t y;
    svg_length_t width;
    svg_length_t height;
    svg_length_t rx;
    svg_length_t ry;
} svg_rect_element_t;

typedef struct svg_image {
    char *url;

    char *data;
    unsigned int data_width;
    unsigned int data_height;

    /* User-space position and size */
    svg_length_t x;
    svg_length_t y;
    svg_length_t width;
    svg_length_t height;
} svg_image_t;

typedef enum svg_element_type {
    SVG_ELEMENT_TYPE_SVG_GROUP,
    SVG_ELEMENT_TYPE_GROUP,
    SVG_ELEMENT_TYPE_DEFS,
    SVG_ELEMENT_TYPE_USE,
    SVG_ELEMENT_TYPE_SYMBOL,
    SVG_ELEMENT_TYPE_PATH,
    SVG_ELEMENT_TYPE_CIRCLE,
    SVG_ELEMENT_TYPE_ELLIPSE,
    SVG_ELEMENT_TYPE_LINE,
    SVG_ELEMENT_TYPE_RECT,
    SVG_ELEMENT_TYPE_TEXT,
    SVG_ELEMENT_TYPE_GRADIENT,
    SVG_ELEMENT_TYPE_GRADIENT_STOP,
    SVG_ELEMENT_TYPE_PATTERN,
    SVG_ELEMENT_TYPE_IMAGE
} svg_element_type_t;

struct svg_element {

    struct svg_element *parent;

    svg_t *doc;

    svg_transform_t transform;
    svg_style_t style;

    svg_element_type_t type;

    char *id;

    union {
	svg_group_t group;
	svg_path_t path;
	svg_ellipse_t ellipse;
	svg_line_t line;
	svg_rect_element_t rect;
	svg_text_t text;
	svg_image_t image;
	svg_gradient_t gradient;
	svg_gradient_stop_t gradient_stop;
	svg_pattern_t pattern;
    } e;
};

typedef struct svg_parser svg_parser_t;

typedef svg_status_t (svg_parser_parse_element_t)(svg_parser_t	*parser,
						  const char	**attributes,
						  svg_element_t	**element_ret);

typedef svg_status_t (svg_parser_parse_characters_t) (svg_parser_t	*parser,
						      const char	*ch,
						      int		len);

typedef struct svg_parser_cb {
    svg_parser_parse_element_t		*parse_element;
    svg_parser_parse_characters_t	*parse_characters;
} svg_parser_cb_t;

typedef struct svg_parser_state {
    const svg_parser_cb_t	*cb;
    svg_element_t		*group_element;
    svg_text_t			*text;

    struct svg_parser_state	*next;
} svg_parser_state_t;

struct svg_parser {

    svg_t *svg;

    svg_xml_parser_context_t ctxt;

    unsigned int unknown_element_depth;
    svg_parser_state_t *state;

    svg_xml_hash_table_t *entities;

    svg_status_t status;
};

struct svg {
    double dpi;

    char *dir_name;

    svg_element_t *group_element;

    svg_xml_hash_table_t *element_ids;

    svg_parser_t parser;

    svg_render_engine_t *engine;
};

extern svg_t* doc;

/* svg.c */

svg_status_t
_svg_store_element_by_id (svg_t *svg, svg_element_t *element);

svg_status_t
_svg_fetch_element_by_id (svg_t *svg, const char *id, svg_element_t **element_ret);

/* libsvg_features.c */

extern const unsigned int libsvg_major_version, libsvg_minor_version, libsvg_micro_version;
extern const char *libsvg_version;

void libsvg_preinit(void *app, void *modinfo);
void libsvg_postinit(void *app, void *modinfo);

/* svg_attribute.c */

svgint_status_t
_svg_attribute_get_double (const char	**attributes,
			   const char	*name,
			   double	*value,
			   double	 default_value);

svgint_status_t
_svg_attribute_get_string (const char	**attributes,
			   const char	*name,
			   const char	**value,
			   const char	*default_value);

svgint_status_t
_svg_attribute_get_length (const char	**attributes,
			   const char	*name,
			   svg_length_t	*value,
			   const char	*default_value);

/* svg_color.c */

svg_status_t
_svg_color_init_rgb (svg_color_t *color,
		     unsigned int r,
		     unsigned int g,
		     unsigned int b);

svg_status_t
_svg_color_init_from_str (svg_color_t *color, const char *str);

svg_status_t
_svg_color_deinit (svg_color_t *color);

/* svg_element.c */

svgint_status_t
_svg_element_create (svg_element_t	**element,
		     svg_element_type_t	type,
		     svg_element_t	*parent,
		     svg_t		*doc);

svg_status_t
_svg_element_init (svg_element_t	*element,
		   svg_element_type_t	type,
		   svg_element_t	*parent,
		   svg_t		*doc);

svg_status_t
_svg_element_init_copy (svg_element_t	*element,
			svg_element_t	*other);

svg_status_t
_svg_element_deinit (svg_element_t *element);

svg_status_t
_svg_element_destroy (svg_element_t *element);

svgint_status_t
_svg_element_clone (svg_element_t	**element,
		    svg_element_t	*other);

svg_status_t
_svg_element_apply_attributes (svg_element_t	*group_element,
			       const char	**attributes);

svg_status_t
_svg_element_parse_aspect_ratio (const char *aspect_ratio_str,
				 svg_view_box_t *view_box);

svg_status_t
_svg_element_parse_view_box (const char	*view_box_str,
			     double	*x,
			     double	*y,
			     double	*width,
			     double	*height);

svg_status_t
_svg_element_get_nearest_viewport (svg_element_t *element, svg_element_t **viewport);

/* svg_gradient.c */

svg_status_t
_svg_gradient_init (svg_gradient_t *gradient);

svg_status_t
_svg_gradient_init_copy (svg_gradient_t *gradient,
			 svg_gradient_t *other);

svg_status_t
_svg_gradient_deinit (svg_gradient_t *gradient);

svg_status_t
_svg_gradient_set_type (svg_gradient_t *gradient,
			svg_gradient_type_t type);

svg_status_t
_svg_gradient_add_stop (svg_gradient_t *gradient,
			double		offset,
			svg_color_t	*color,
			double		opacity);

svg_status_t
_svg_gradient_apply_attributes (svg_gradient_t	*gradient,
				svg_t		*svg,
				const char	**attributes);

/* svg_group.c */

svg_status_t
_svg_group_init (svg_group_t *group);

svg_status_t
_svg_group_init_copy (svg_group_t *group,
		      svg_group_t *other);

svg_status_t
_svg_group_deinit (svg_group_t *group);

svg_status_t
_svg_group_add_element (svg_group_t *group, svg_element_t *element);

svg_status_t
_svg_group_render (svg_group_t		*group,
		   svg_render_engine_t	*engine,
		   void			*closure);

svg_status_t
_svg_symbol_render (svg_element_t	*group,
		    svg_render_engine_t	*engine,
		    void		*closure);

svg_status_t
_svg_group_apply_svg_attributes (svg_group_t	*group,
				 const char	**attributes);

svg_status_t
_svg_group_apply_group_attributes (svg_group_t		*group,
				   const char		**attributes);

svg_status_t
_svg_group_apply_use_attributes (svg_element_t		*group,
				 const char		**attributes);

svg_status_t
_svg_group_get_size (svg_group_t *group, svg_length_t *width, svg_length_t *height);

/* svg_image.c */

svg_status_t
_svg_image_init (svg_image_t *image);

svg_status_t
_svg_image_init_copy (svg_image_t *image,
		      svg_image_t *other);

svg_status_t
_svg_image_deinit (svg_image_t *image);

svg_status_t
_svg_image_apply_attributes (svg_image_t	*image,
			     const char		**attributes);

svg_status_t
_svg_image_render (svg_image_t		*image,
		   svg_render_engine_t	*engine,
		   void			*closure);

/* svg_length.c */

svg_status_t
_svg_length_init (svg_length_t *length, double value);

svg_status_t
_svg_length_init_unit (svg_length_t		*length,
		       double			value,
		       svg_length_unit_t	unit,
		       svg_length_orientation_t orientation);

svg_status_t
_svg_length_init_from_str (svg_length_t *length, const char *str);

svg_status_t
_svg_length_deinit (svg_length_t *length);

/* svg_paint.c */

svg_status_t
_svg_paint_init (svg_paint_t *paint, svg_t *svg, const char *str);

svg_status_t
_svg_paint_deinit (svg_paint_t *paint);

/* svg_parser.c */

svg_status_t
_svg_parser_init (svg_parser_t *parser, svg_t *svg);

svg_status_t
_svg_parser_deinit (svg_parser_t *parser);

svg_status_t
_svg_parser_begin (svg_parser_t *parser);

svg_status_t
_svg_parser_parse_chunk (svg_parser_t *parser, const char *buf, size_t count);

svg_status_t
_svg_parser_end (svg_parser_t *parser);

void
_svg_parser_sax_start_element (void		*closure,
			       const xmlChar	*name,
			       const xmlChar	**atts);

void
_svg_parser_sax_end_element (void		*closure,
			     const xmlChar	*name);

void
_svg_parser_sax_characters (void		*closure,
			    const xmlChar	*ch,
			    int			len);

/* svg_path.c */

svg_status_t
_svg_path_create (svg_path_t **path);

svg_status_t
_svg_path_init (svg_path_t *path);

svg_status_t
_svg_ellipse_init (svg_ellipse_t *ellipse);

svg_status_t
_svg_ellipse_init_copy (svg_ellipse_t *ellipse, svg_ellipse_t *other);

svg_status_t
_svg_line_init (svg_line_t *line);

svg_status_t
_svg_line_init_copy (svg_line_t *line, svg_line_t *other);

svg_status_t
_svg_rect_init (svg_rect_element_t *rect);

svg_status_t
_svg_path_init_copy (svg_path_t *path,
		     svg_path_t *other);

svg_status_t
_svg_rect_init_copy (svg_rect_element_t *path,
		     svg_rect_element_t *other);

svg_status_t
_svg_path_deinit (svg_path_t *path);

svg_status_t
_svg_path_destroy (svg_path_t *path);

svg_status_t
_svg_path_render (svg_path_t		*path,
		  svg_render_engine_t	*engine,
		  void			*closure);

svg_status_t
_svg_circle_render (svg_ellipse_t	*circle,
		    svg_render_engine_t	*engine,
		    void		*closure);

svg_status_t
_svg_ellipse_render (svg_ellipse_t	*ellipse,
		     svg_render_engine_t	*engine,
		     void			*closure);

svg_status_t
_svg_line_render (svg_line_t	*line,
		  svg_render_engine_t	*engine,
		  void			*closure);

svg_status_t
_svg_rect_render (svg_rect_element_t	*rect,
		  svg_render_engine_t	*engine,
		  void			*closure);

svg_status_t
_svg_path_apply_attributes (svg_path_t		*path,
			    const char		**attributes);

svg_status_t
_svg_path_add_from_str (svg_path_t *path, const char *path_str);

svg_status_t
_svg_path_move_to (svg_path_t *path, double x, double y);

svg_status_t
_svg_path_rel_move_to (svg_path_t *path, double dx, double dy);

svg_status_t
_svg_path_line_to (svg_path_t *path, double x, double y);

svg_status_t
_svg_path_rel_line_to (svg_path_t *path, double dx, double dy);

svg_status_t
_svg_path_horizontal_line_to (svg_path_t *path, double x);

svg_status_t
_svg_path_rel_horizontal_line_to (svg_path_t *path, double dx);

svg_status_t
_svg_path_vertical_line_to (svg_path_t *path, double y);

svg_status_t
_svg_path_rel_vertical_line_to (svg_path_t *path, double dy);

svg_status_t
_svg_path_curve_to (svg_path_t *path,
		    double x1, double y1,
		    double x2, double y2,
		    double x3, double y3);

svg_status_t
_svg_path_rel_curve_to (svg_path_t *path,
			double dx1, double dy1,
			double dx2, double dy2,
			double dx3, double dy3);

svg_status_t
_svg_path_smooth_curve_to (svg_path_t *path,
			   double x2, double y2,
			   double x3, double y3);

svg_status_t
_svg_path_rel_smooth_curve_to (svg_path_t *path,
			       double dx2, double dy2,
			       double dx3, double dy3);

svg_status_t
_svg_path_quadratic_curve_to (svg_path_t *path,
			      double x1, double y1,
			      double x2, double y2);

svg_status_t
_svg_path_rel_quadratic_curve_to (svg_path_t *path,
				  double dx1, double dy1,
				  double dx2, double dy2);

svg_status_t
_svg_path_smooth_quadratic_curve_to (svg_path_t *path,
				     double x2, double y2);

svg_status_t
_svg_path_rel_smooth_quadratic_curve_to (svg_path_t *path,
					 double dx2, double dy2);

svg_status_t
_svg_path_close_path (svg_path_t *path);

svg_status_t
_svg_path_arc_to (svg_path_t	*path,
		  double	rx,
		  double	ry,
		  double	x_axis_rotation,
		  int		large_arc_flag,
		  int		sweep_flag,
		  double	x,
		  double	y);

svg_status_t
_svg_path_rel_arc_to (svg_path_t	*path,
		      double		rx,
		      double		ry,
		      double		x_axis_rotation,
		      int		large_arc_flag,
		      int		sweep_flag,
		      double		x,
		      double		y);

/* svg_pattern.c */

svg_status_t
_svg_pattern_init (svg_pattern_t *pattern,
		   svg_element_t *parent,
		   svg_t	 *doc);

svg_status_t
_svg_pattern_init_copy (svg_pattern_t *pattern,
			svg_pattern_t *other);

svg_status_t
_svg_pattern_deinit (svg_pattern_t *pattern);

svg_status_t
_svg_pattern_apply_attributes (svg_pattern_t	*pattern,
			       const char	**attributes);

svg_status_t
_svg_pattern_render (svg_element_t		*pattern,
		     svg_render_engine_t	*engine,
		     void			*closure);

/* svg_str.c */

void
_svg_str_skip_space (const char **str);

void
_svg_str_skip_char (const char **str, char c);

void
_svg_str_skip_space_or_char (const char **str, char c);

svgint_status_t
_svg_str_parse_csv_doubles (const char *str, double *value, int num_values, const char **end);

svgint_status_t
_svg_str_parse_all_csv_doubles (const char *str, double **value, int *num_values, const char **end);

/* svg_style.c */

svg_status_t
_svg_style_init_empty (svg_style_t *style, svg_t *svg);

svg_status_t
_svg_style_init_copy (svg_style_t *style, svg_style_t *other);

svg_status_t
_svg_style_init_defaults (svg_style_t *style, svg_t *svg);

svg_status_t
_svg_style_deinit (svg_style_t *style);

svg_status_t
_svg_style_render (svg_style_t		*style,
		   svg_render_engine_t	*engine,
		   void			*closure);

svg_status_t
_svg_style_apply_attributes (svg_style_t	*style, 
			     const char		**attributes);

double
_svg_style_get_opacity (svg_style_t *style);

svg_status_t
_svg_style_get_display (svg_style_t *style);

svg_status_t
_svg_style_get_visibility (svg_style_t *style);

/* svg_text.c */

svg_status_t
_svg_text_init (svg_text_t *text);

svg_status_t
_svg_text_init_copy (svg_text_t *text,
		     svg_text_t *other);

svg_status_t
_svg_text_deinit (svg_text_t *text);

svg_status_t
_svg_text_append_chars (svg_text_t	*text,
			const char	*chars,
			int		len);

svg_status_t
_svg_text_render (svg_text_t		*text,
		  svg_render_engine_t	*engine,
		  void			*closure);

svg_status_t
_svg_text_apply_attributes (svg_text_t		*text,
			    const char		**attributes);

/* svg_transform.c */

svg_status_t
_svg_transform_init (svg_transform_t *transform);

svg_status_t
_svg_transform_init_matrix (svg_transform_t *transform,
			    double a, double b,
			    double c, double d,
			    double e, double f);

svg_status_t
_svg_transform_init_translate (svg_transform_t *transform,
			       double tx, double ty);

svg_status_t
_svg_transform_init_scale (svg_transform_t *transform,
			   double sx, double sy);

svg_status_t
_svg_transform_init_rotate (svg_transform_t *transform,
			    double angle_degrees);

svg_status_t
_svg_transform_init_skew_x (svg_transform_t *transform,
			    double angle_degrees);

svg_status_t
_svg_transform_init_skew_y (svg_transform_t *transform,
			    double angle_degrees);

svg_status_t
_svg_transform_deinit (svg_transform_t *transform);

svg_status_t
_svg_transform_add_translate (svg_transform_t *transform,
			      double tx, double ty);

svg_status_t
_svg_transform_add_scale_uniform (svg_transform_t *transform, double s);

svg_status_t
_svg_transform_add_scale (svg_transform_t *transform,
			  double sx, double sy);

extern svg_status_t
_svg_transform_parse_str (svg_transform_t *transform, const char *str);

/* silly SVG spec. uses degrees */
svg_status_t
_svg_transform_add_rotate (svg_transform_t *transform,
			   double angle_degrees);

/* silly SVG spec. uses degrees */
svg_status_t
_svg_transform_add_skew_x (svg_transform_t *transform,
			   double angle_degrees);

/* silly SVG spec. uses degrees */
svg_status_t
_svg_transform_add_skew_y (svg_transform_t *transform,
			   double angle_degrees);

svg_status_t
_svg_transform_multiply_into_left (svg_transform_t *t1, const svg_transform_t *t2);

svg_status_t
_svg_transform_multiply_into_right (const svg_transform_t *t1, svg_transform_t *t2);

svg_status_t
_svg_transform_render (svg_transform_t		*transform,
		       svg_render_engine_t	*engine,
		       void			*closure);

svg_status_t
_svg_transform_apply_attributes (svg_transform_t	*transform,
				 const char		**attributes);

#endif
