/* svg_path.c: Data structures for SVG paths
 
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

#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "svgint.h"

svg_status_t
_svg_path_parse_str (svg_path_t *path, const xmlChar *path_str);

typedef struct svg_path_cmd_info {
    char cmd_char;
    int num_args;
    svg_path_cmd_t cmd;
} svg_path_cmd_info_t;
    
#define SVG_PATH_CMD_MAX_ARGS 7

/* This must be in the same order and include at least all values in
   svg_path_cmd_t, (for direct indexing) */
static const svg_path_cmd_info_t SVG_PATH_CMD_INFO[] = {
    { 'M', 2, SVG_PATH_CMD_MOVE_TO },
    { 'm', 2, SVG_PATH_CMD_REL_MOVE_TO },
    { 'L', 2, SVG_PATH_CMD_LINE_TO },
    { 'l', 2, SVG_PATH_CMD_REL_LINE_TO },
    { 'H', 1, SVG_PATH_CMD_HORIZONTAL_LINE_TO },
    { 'h', 1, SVG_PATH_CMD_REL_HORIZONTAL_LINE_TO },
    { 'V', 1, SVG_PATH_CMD_VERTICAL_LINE_TO },
    { 'v', 1, SVG_PATH_CMD_REL_VERTICAL_LINE_TO },
    { 'C', 6, SVG_PATH_CMD_CURVE_TO },
    { 'c', 6, SVG_PATH_CMD_REL_CURVE_TO },
    { 'S', 4, SVG_PATH_CMD_SMOOTH_CURVE_TO },
    { 's', 4, SVG_PATH_CMD_REL_SMOOTH_CURVE_TO },
    { 'Q', 4, SVG_PATH_CMD_QUADRATIC_CURVE_TO },
    { 'q', 4, SVG_PATH_CMD_REL_QUADRATIC_CURVE_TO },
    { 'T', 2, SVG_PATH_CMD_SMOOTH_QUADRATIC_CURVE_TO },
    { 't', 2, SVG_PATH_CMD_REL_SMOOTH_QUADRATIC_CURVE_TO },
    { 'A', 7, SVG_PATH_CMD_ARC_TO },
    { 'a', 7, SVG_PATH_CMD_REL_ARC_TO },
    { 'Z', 0, SVG_PATH_CMD_CLOSE_PATH },
    { 'z', 0, SVG_PATH_CMD_CLOSE_PATH }
};

static int
_svg_path_is_empty (svg_path_t *path);

static svg_status_t
_svg_path_cmd_info_lookup (char cmd_char, const svg_path_cmd_info_t **cmd_info);

static void
_svg_path_add_op_buf (svg_path_t *path, svg_path_op_buf_t *op);

static svg_status_t
_svg_path_new_op_buf (svg_path_t *path);

static svg_status_t
_svg_path_add_arg_buf (svg_path_t *path, svg_path_arg_buf_t *arg);

static svg_status_t
_svg_path_new_arg_buf (svg_path_t *path);

static svg_status_t
_svg_path_add (svg_path_t *path, svg_path_op_t op, ...);

static svg_status_t
_svg_path_add_va (svg_path_t *path, svg_path_op_t op, va_list va);

static svg_path_op_buf_t *
_svg_path_op_buf_create (void);

static svg_status_t
_svg_path_op_buf_destroy (svg_path_op_buf_t *op);

static svg_status_t
_svg_path_op_buf_add (svg_path_op_buf_t *op_buf, svg_path_op_t op);

static svg_path_arg_buf_t *
_svg_path_arg_buf_create (void);

static svg_status_t
_svg_path_arg_buf_destroy (svg_path_arg_buf_t *arg_buf);

static svg_status_t
_svg_path_arg_buf_add (svg_path_arg_buf_t *arg_buf, double val);

svg_status_t
_svg_path_create (svg_path_t **path)
{
    *path = malloc (sizeof (svg_path_t));
    if (*path == NULL)
	return SVG_STATUS_NO_MEMORY;

    return _svg_path_init (*path);
}

svg_status_t
_svg_path_init (svg_path_t *path)
{
    path->last_path_op = SVG_PATH_OP_MOVE_TO;

    path->last_move_pt.x = 0;
    path->last_move_pt.y = 0;

    path->current_pt.x = 0;
    path->current_pt.y = 0;

    path->reflected_cubic_pt.x = 0;
    path->reflected_cubic_pt.y = 0;
    path->reflected_quad_pt.x = 0;
    path->reflected_quad_pt.y = 0;

    path->op_head = NULL;
    path->op_tail = NULL;

    path->arg_head = NULL;
    path->arg_tail = NULL;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_ellipse_init (svg_ellipse_t *ellipse)
{
    _svg_length_init_unit (&ellipse->cx, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_HORIZONTAL);
    _svg_length_init_unit (&ellipse->cy, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_VERTICAL);
    _svg_length_init_unit (&ellipse->rx, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_HORIZONTAL);
    _svg_length_init_unit (&ellipse->ry, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_VERTICAL);

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_ellipse_init_copy (svg_ellipse_t *ellipse, svg_ellipse_t *other)
{
    *ellipse = *other;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_line_init (svg_line_t *line)
{
	_svg_length_init_unit (&line->x1, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_HORIZONTAL);
	_svg_length_init_unit (&line->y1, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_VERTICAL);
	_svg_length_init_unit (&line->x2, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_HORIZONTAL);
	_svg_length_init_unit (&line->y2, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_VERTICAL);

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_line_init_copy (svg_line_t *line, svg_line_t *other)
{
    *line = *other;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_rect_init (svg_rect_element_t *rect)
{
	_svg_length_init_unit (&rect->x, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_HORIZONTAL);
	_svg_length_init_unit (&rect->y, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_VERTICAL);
	_svg_length_init_unit (&rect->width, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_HORIZONTAL);
	_svg_length_init_unit (&rect->height, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_VERTICAL);
	_svg_length_init_unit (&rect->rx, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_HORIZONTAL);
	_svg_length_init_unit (&rect->ry, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_VERTICAL);

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_rect_init_copy (svg_rect_element_t *rect,
		     svg_rect_element_t *other)
{
    *rect = *other;

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_path_do_nothing (void *closure)
{
    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_path_init_copy (svg_path_t *path,
		     svg_path_t *other)
{
    /* An ugly little hack. Cleaner would be to fix up the
       render_engine so that the 4 relevant fields here made part of a
       new path_interpreter struct */
    static svg_render_engine_t svg_path_copy_engine = {
	NULL, NULL, NULL, NULL,
	(svg_status_t (*) (void *, double, double)) _svg_path_move_to,
	(svg_status_t (*) (void *, double, double)) _svg_path_line_to,
	(svg_status_t (*) (void *,
			   double, double,
			   double, double,
			   double, double)) _svg_path_curve_to,
	(svg_status_t (*) (void *,
			   double, double,
			   double, double)) _svg_path_quadratic_curve_to,
	(svg_status_t (*) (void *,
			   double, double, double,
			   int, int,
			   double, double)) _svg_path_arc_to,
	(svg_status_t (*) (void *)) _svg_path_close_path,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	_svg_path_do_nothing,
	NULL, NULL, NULL, NULL
    };

    _svg_path_init (path);

    return _svg_path_render (other, &svg_path_copy_engine, path);
}

static int
_svg_path_is_empty (svg_path_t *path)
{
    return path->op_head == NULL;
}

svg_status_t
_svg_path_deinit (svg_path_t *path)
{
    svg_path_op_buf_t *op;
    svg_path_arg_buf_t *arg;

    while (path->op_head) {
	op = path->op_head;
	path->op_head = op->next;
	_svg_path_op_buf_destroy (op);
    }
    path->op_tail = NULL;

    while (path->arg_head) {
	arg = path->arg_head;
	path->arg_head = arg->next;
	_svg_path_arg_buf_destroy (arg);
    }
    path->arg_tail = NULL;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_path_destroy (svg_path_t *path)
{
    svg_status_t status;

    status = _svg_path_deinit (path);

    free (path);

    return status;
}

svg_status_t
_svg_path_render (svg_path_t		*path,
		  svg_render_engine_t	*engine,
		  void			*closure)
{
    int i, j;
    double arg[SVG_PATH_CMD_MAX_ARGS];
    svg_status_t status = SVG_STATUS_SUCCESS;
    svg_path_op_buf_t *op_buf;
    svg_path_op_t op;
    svg_path_arg_buf_t *arg_buf = path->arg_head;
    int buf_i = 0;

    for (op_buf = path->op_head; op_buf; op_buf = op_buf->next) {
	for (i=0; i < op_buf->num_ops; i++) {
	    op = op_buf->op[i];

	    for (j=0; j < SVG_PATH_CMD_INFO[op].num_args; j++) {
		arg[j] = arg_buf->arg[buf_i];
		buf_i++;
		if (buf_i >= arg_buf->num_args) {
		    arg_buf = arg_buf->next;
		    buf_i = 0;
		}
	    }

	    switch (op) {
	    case SVG_PATH_OP_MOVE_TO:
		status = (engine->move_to) (closure, arg[0], arg[1]);
		break;
	    case SVG_PATH_OP_LINE_TO:
		status = (engine->line_to) (closure, arg[0], arg[1]);
		break;
	    case SVG_PATH_OP_CURVE_TO:
		status = (engine->curve_to) (closure,
					     arg[0], arg[1],
					     arg[2], arg[3],
					     arg[4], arg[5]);
		break;
	    case SVG_PATH_OP_QUAD_TO:
		status = (engine->quadratic_curve_to) (closure,
						       arg[0], arg[1],
						       arg[2], arg[3]);
		break;
	    case SVG_PATH_OP_ARC_TO:
		status = (engine->arc_to) (closure,
					   arg[0], arg[1],
					   arg[2], arg[3], arg[4],
					   arg[5], arg[6]);
		break;
	    case SVG_PATH_OP_CLOSE_PATH:
		status = (engine->close_path) (closure);
		break;
	    }
	    if (status)
		return status;
	}
    }

    status = (engine->render_path) (closure);
    if (status)
	return status;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_path_apply_attributes (svg_path_t		*path,
			    const char		**attributes)
{
    svg_status_t status;
    const char *path_str;

    if (_svg_path_is_empty (path)) {
	_svg_attribute_get_string (attributes, "d", &path_str, NULL);

	/* XXX: Need to check spec. for this error case */
	if (path_str == NULL)
	    return SVG_STATUS_PARSE_ERROR;

	status = _svg_path_add_from_str (path, path_str);
	if (status)
	    return status;
    }

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_path_cmd_info_lookup (char cmd_char, const svg_path_cmd_info_t **cmd_info)
{
    unsigned int i;

    for (i = 0; i < SVG_ARRAY_SIZE(SVG_PATH_CMD_INFO); i++)
	if (SVG_PATH_CMD_INFO[i].cmd_char == cmd_char) {
	    *cmd_info = &SVG_PATH_CMD_INFO[i];
	    return SVG_STATUS_SUCCESS;
	}

    return SVG_STATUS_PARSE_ERROR;
}

svg_status_t
_svg_path_add_from_str (svg_path_t *path, const char *path_str)
{
    const char *s;
    const char *end;
    svg_status_t status;
    const svg_path_cmd_info_t *cmd_info;
    double arg[SVG_PATH_CMD_MAX_ARGS];

    s = path_str;
    while (*s) {
	if (_svg_ascii_isspace (*s)) {
	    s++;
	    continue;
	}

	status = _svg_path_cmd_info_lookup (s[0], &cmd_info);
	if (status)
	    return status;
	s++;

	while (1) {
	    status = _svg_str_parse_csv_doubles (s, arg, cmd_info->num_args, &end);
	    s = end;
	    if (status == SVGINT_STATUS_ARGS_EXHAUSTED)
		goto NEXT_CMD;
	    if (status)
		return status;
	    switch (cmd_info->cmd) {
	    case SVG_PATH_CMD_MOVE_TO:
		status = _svg_path_move_to (path, arg[0], arg[1]);
		break;
	    case SVG_PATH_CMD_REL_MOVE_TO:
		status = _svg_path_rel_move_to (path, arg[0], arg[1]);
		break;
	    case SVG_PATH_CMD_LINE_TO:
		status = _svg_path_line_to (path, arg[0], arg[1]);
		break;
	    case SVG_PATH_CMD_REL_LINE_TO:
		status = _svg_path_rel_line_to (path, arg[0], arg[1]);
		break;
	    case SVG_PATH_CMD_HORIZONTAL_LINE_TO:
		status = _svg_path_horizontal_line_to (path, arg[0]);
		break;
	    case SVG_PATH_CMD_REL_HORIZONTAL_LINE_TO:
		status = _svg_path_rel_horizontal_line_to (path, arg[0]);
		break;
	    case SVG_PATH_CMD_VERTICAL_LINE_TO:
		status = _svg_path_vertical_line_to (path, arg[0]);
		break;
	    case SVG_PATH_CMD_REL_VERTICAL_LINE_TO:
		status = _svg_path_rel_vertical_line_to (path, arg[0]);
		break;
	    case SVG_PATH_CMD_CURVE_TO:
		status = _svg_path_curve_to (path,
					     arg[0], arg[1],
					     arg[2], arg[3],
					     arg[4], arg[5]);
		break;
	    case SVG_PATH_CMD_REL_CURVE_TO:
		status = _svg_path_rel_curve_to (path,
						 arg[0], arg[1],
						 arg[2], arg[3],
						 arg[4], arg[5]);
		break;
	    case SVG_PATH_CMD_SMOOTH_CURVE_TO:
		status = _svg_path_smooth_curve_to (path,
						    arg[0], arg[1],
						    arg[2], arg[3]);
		break;
	    case SVG_PATH_CMD_REL_SMOOTH_CURVE_TO:
		status = _svg_path_rel_smooth_curve_to (path,
							arg[0], arg[1],
							arg[2], arg[3]);
		break;
	    case SVG_PATH_CMD_QUADRATIC_CURVE_TO:
		status = _svg_path_quadratic_curve_to (path,
						       arg[0], arg[1],
						       arg[2], arg[3]);
		break;
	    case SVG_PATH_CMD_REL_QUADRATIC_CURVE_TO:
		status = _svg_path_rel_quadratic_curve_to (path,
							   arg[0], arg[1],
							   arg[2], arg[3]);
		break;
	    case SVG_PATH_CMD_SMOOTH_QUADRATIC_CURVE_TO:
		status = _svg_path_smooth_quadratic_curve_to (path,
							      arg[0], arg[1]);
		break;
	    case SVG_PATH_CMD_REL_SMOOTH_QUADRATIC_CURVE_TO:
		status = _svg_path_rel_smooth_quadratic_curve_to (path,
								  arg[0], arg[1]);
		break;
	    case SVG_PATH_CMD_ARC_TO:
		status = _svg_path_arc_to (path,
					   arg[0], arg[1],
					   arg[2], arg[3],
					   arg[4], arg[5],
					   arg[6]);
		break;
	    case SVG_PATH_CMD_REL_ARC_TO:
		status = _svg_path_rel_arc_to (path,
					       arg[0], arg[1],
					       arg[2], arg[3],
					       arg[4], arg[5],
					       arg[6]);
		break;
	    case SVG_PATH_CMD_CLOSE_PATH:
		status = _svg_path_close_path (path);
		goto NEXT_CMD;
		break;
	    default:
		status = SVG_STATUS_PARSE_ERROR;
		break;
	    }
	    if (status)
		return status;
	}
    NEXT_CMD:
	;
    }

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_path_add (svg_path_t *path, svg_path_op_t op, ...)
{
    va_list va;
    svg_status_t status;

    va_start (va, op);
    status = _svg_path_add_va (path, op, va);
    va_end (va);

    return status;
}

static svg_status_t
_svg_path_add_va (svg_path_t *path, svg_path_op_t op, va_list va)
{
    int i;
    svg_status_t status;
    const svg_path_cmd_info_t *cmd_info;
    int num_args;

    cmd_info = &SVG_PATH_CMD_INFO[op];
    num_args = cmd_info->num_args;

    if (path->op_tail == NULL || path->op_tail->num_ops + 1 > SVG_PATH_BUF_SZ) {
        status = _svg_path_new_op_buf (path);
        if (status)
            return status;
    }
    _svg_path_op_buf_add (path->op_tail, op);

    if (path->arg_tail == NULL || path->arg_tail->num_args + num_args > SVG_PATH_BUF_SZ) {
        status = _svg_path_new_arg_buf (path);
        if (status)
            return status;
    }
    for (i=0; i < num_args; i++)
	_svg_path_arg_buf_add (path->arg_tail, va_arg (va, double));

    path->last_path_op = op;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_path_move_to (svg_path_t *path, double x, double y)
{
    svg_status_t status;

    status = _svg_path_add (path, SVG_PATH_OP_MOVE_TO, x, y);

    path->last_move_pt.x = x;
    path->last_move_pt.y = y;

    path->current_pt.x = x;
    path->current_pt.y = y;

    return status;
}

svg_status_t
_svg_path_rel_move_to (svg_path_t *path, double dx, double dy)
{
    return _svg_path_move_to (path,
			      path->current_pt.x + dx,
			      path->current_pt.y + dy);
}

svg_status_t
_svg_path_line_to (svg_path_t *path, double x, double y)
{
    svg_status_t status;

    status = _svg_path_add (path, SVG_PATH_OP_LINE_TO, x, y);

    path->current_pt.x = x;
    path->current_pt.y = y;

    return status;
}

svg_status_t
_svg_path_rel_line_to (svg_path_t *path, double dx, double dy)
{
    return _svg_path_line_to (path,
			      path->current_pt.x + dx,
			      path->current_pt.y + dy);
}

svg_status_t
_svg_path_horizontal_line_to (svg_path_t *path, double x)
{
    return _svg_path_line_to (path,
			      x,
			      path->current_pt.y);
}

svg_status_t
_svg_path_rel_horizontal_line_to (svg_path_t *path, double dx)
{
    return _svg_path_horizontal_line_to (path,
					 path->current_pt.x + dx);
}

svg_status_t
_svg_path_vertical_line_to (svg_path_t *path, double y)
{
    return _svg_path_line_to (path,
			      path->current_pt.x,
			      y);
}

svg_status_t
_svg_path_rel_vertical_line_to (svg_path_t *path, double dy)
{
    return _svg_path_vertical_line_to (path,
				       path->current_pt.y + dy);
}

svg_status_t
_svg_path_curve_to (svg_path_t *path,
		    double x1, double y1,
		    double x2, double y2,
		    double x3, double y3)
{
    svg_status_t status;

    status = _svg_path_add (path, SVG_PATH_OP_CURVE_TO,
			    x1, y1,
			    x2, y2,
			    x3, y3);

    path->current_pt.x = x3;
    path->current_pt.y = y3;

    path->reflected_cubic_pt.x = x3 + x3 - x2;
    path->reflected_cubic_pt.y = y3 + y3 - y2;

    return status;
}

svg_status_t
_svg_path_rel_curve_to (svg_path_t *path,
			double dx1, double dy1,
			double dx2, double dy2,
			double dx3, double dy3)
{
    return _svg_path_curve_to (path,
			       path->current_pt.x + dx1,
			       path->current_pt.y + dy1,
			       path->current_pt.x + dx2,
			       path->current_pt.y + dy2,
			       path->current_pt.x + dx3,
			       path->current_pt.y + dy3);
}

svg_status_t
_svg_path_smooth_curve_to (svg_path_t *path,
			   double x2, double y2,
			   double x3, double y3)
{
    if (path->last_path_op == SVG_PATH_OP_CURVE_TO)
	return _svg_path_curve_to (path,
				   path->reflected_cubic_pt.x,
				   path->reflected_cubic_pt.y,
				   x2, y2,
				   x3, y3);
    else
	return _svg_path_curve_to (path,
				   path->current_pt.x,
				   path->current_pt.y,
				   x2, y2,
				   x3, y3);
}

svg_status_t
_svg_path_rel_smooth_curve_to (svg_path_t *path,
			       double dx2, double dy2,
			       double dx3, double dy3)
{
    return _svg_path_smooth_curve_to (path,
				      path->current_pt.x + dx2,
				      path->current_pt.y + dy2,
				      path->current_pt.x + dx3,
				      path->current_pt.y + dy3);
}

svg_status_t
_svg_path_quadratic_curve_to (svg_path_t *path,
			      double x1, double y1,
			      double x2, double y2)
{
    svg_status_t status;

    status = _svg_path_add (path, SVG_PATH_OP_QUAD_TO,
			    x1, y1,
			    x2, y2);

    path->current_pt.x = x2;
    path->current_pt.y = y2;

    path->reflected_quad_pt.x = x2 + x2 - x1;
    path->reflected_quad_pt.y = y2 + y2 - y1;

    return status;
}

svg_status_t
_svg_path_rel_quadratic_curve_to (svg_path_t *path,
				  double dx1, double dy1,
				  double dx2, double dy2)
{
    return _svg_path_quadratic_curve_to (path,
					 path->current_pt.x + dx1,
					 path->current_pt.y + dy1,
					 path->current_pt.x + dx2,
					 path->current_pt.y + dy2);
}

svg_status_t
_svg_path_smooth_quadratic_curve_to (svg_path_t *path,
				     double x2, double y2)
{
    if (path->last_path_op == SVG_PATH_OP_QUAD_TO)
	return _svg_path_quadratic_curve_to (path,
					     path->reflected_quad_pt.x,
					     path->reflected_quad_pt.y,
					     x2, y2);
    else
	return _svg_path_quadratic_curve_to (path,
					     path->current_pt.x,
					     path->current_pt.y,
					     x2, y2);
}

svg_status_t
_svg_path_rel_smooth_quadratic_curve_to (svg_path_t *path,
					 double dx2, double dy2)
{
    return _svg_path_smooth_quadratic_curve_to (path,
						path->current_pt.x + dx2,
						path->current_pt.y + dy2);
}

svg_status_t
_svg_path_close_path (svg_path_t *path)
{
    svg_status_t status;

    status = _svg_path_add (path, SVG_PATH_OP_CLOSE_PATH);

    path->current_pt = path->last_move_pt;

    return status;
}

svg_status_t
_svg_line_render (svg_line_t	*line,
		  svg_render_engine_t	*engine,
		  void			*closure)
{
    return (engine->render_line) (closure,
				  &line->x1, &line->y1,
				  &line->x2, &line->y2);
}

svg_status_t
_svg_rect_render (svg_rect_element_t	*rect,
		  svg_render_engine_t	*engine,
		  void			*closure)
{
    return (engine->render_rect) (closure,
				  &rect->x, &rect->y,
				  &rect->width, &rect->height,
				  &rect->rx, &rect->ry);
}

svg_status_t
_svg_circle_render (svg_ellipse_t	*circle,
		    svg_render_engine_t	*engine,
		    void		*closure)
{
    if (circle->rx.value == 0)
	return SVG_STATUS_SUCCESS;

    return (engine->render_ellipse) (closure,
				     &circle->cx, &circle->cy,
				     &circle->rx, &circle->rx);
}

svg_status_t
_svg_ellipse_render (svg_ellipse_t	*ellipse,
		     svg_render_engine_t	*engine,
		     void			*closure)
{
    if (ellipse->rx.value == 0 || ellipse->ry.value == 0)
	return SVG_STATUS_SUCCESS;

    return (engine->render_ellipse) (closure,
				     &ellipse->cx, &ellipse->cy,
				     &ellipse->rx, &ellipse->ry);
}

/**
 * _svg_path_arc_to: Add an arc to the given path
 *
 * rx: Radius in x direction (before rotation).
 * ry: Radius in y direction (before rotation).
 * x_axis_rotation: Rotation angle for axes.
 * large_arc_flag: 0 for arc length <= 180, 1 for arc >= 180.
 * sweep: 0 for "negative angle", 1 for "positive angle".
 * x: New x coordinate.
 * y: New y coordinate.
 *
 **/
svg_status_t
_svg_path_arc_to (svg_path_t	*path,
		  double	rx,
		  double	ry,
		  double	x_axis_rotation,
		  int		large_arc_flag,
		  int		sweep_flag,
		  double	x,
		  double	y)
{
    svg_status_t status;

    status = _svg_path_add (path, SVG_PATH_OP_ARC_TO,
			    rx, ry, x_axis_rotation,
			    (double) large_arc_flag, (double) sweep_flag,
			    x, y);

    path->current_pt.x = x;
    path->current_pt.y = y;

    return status;
}

svg_status_t
_svg_path_rel_arc_to (svg_path_t	*path,
		      double		rx,
		      double		ry,
		      double		x_axis_rotation,
		      int		large_arc_flag,
		      int		sweep_flag,
		      double		dx,
		      double		dy)

{
    return _svg_path_arc_to (path,
			     rx,
			     ry,
			     x_axis_rotation,
			     large_arc_flag,
			     sweep_flag,
			     path->current_pt.x + dx,
			     path->current_pt.y + dy);
}

static void
_svg_path_add_op_buf (svg_path_t *path, svg_path_op_buf_t *op)
{
    op->next = NULL;
    op->prev = path->op_tail;

    if (path->op_tail) {
	path->op_tail->next = op;
    } else {
	path->op_head = op;
    }

    path->op_tail = op;
}

static svg_status_t
_svg_path_new_op_buf (svg_path_t *path)
{
    svg_path_op_buf_t *op;

    op = _svg_path_op_buf_create ();
    if (op == NULL)
	return SVG_STATUS_NO_MEMORY;

    _svg_path_add_op_buf (path, op);

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_path_add_arg_buf (svg_path_t *path, svg_path_arg_buf_t *arg)
{
    arg->next = NULL;
    arg->prev = path->arg_tail;

    if (path->arg_tail) {
	path->arg_tail->next = arg;
    } else {
	path->arg_head = arg;
    }

    path->arg_tail = arg;

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_path_new_arg_buf (svg_path_t *path)
{
    svg_path_arg_buf_t *arg;

    arg = _svg_path_arg_buf_create ();

    if (arg == NULL)
	return SVG_STATUS_NO_MEMORY;

    _svg_path_add_arg_buf (path, arg);

    return SVG_STATUS_SUCCESS;
}

static svg_path_op_buf_t *
_svg_path_op_buf_create (void)
{
    svg_path_op_buf_t *op;

    op = malloc( sizeof (svg_path_op_buf_t));

    if (op) {
	op->num_ops = 0;
	op->next = NULL;
    }

    return op;
}

static svg_status_t
_svg_path_op_buf_destroy (svg_path_op_buf_t *op)
{
    free (op);

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_path_op_buf_add (svg_path_op_buf_t *op_buf, svg_path_op_t op)
{
    op_buf->op[op_buf->num_ops++] = op;

    return SVG_STATUS_SUCCESS;
}

static svg_path_arg_buf_t *
_svg_path_arg_buf_create (void)
{
    svg_path_arg_buf_t *arg_buf;

    arg_buf = malloc (sizeof (svg_path_arg_buf_t));

    if (arg_buf) {
	arg_buf->num_args = 0;
	arg_buf->next = NULL;
    }

    return arg_buf;
}

static svg_status_t
_svg_path_arg_buf_destroy (svg_path_arg_buf_t *arg_buf)
{
    free (arg_buf);

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_path_arg_buf_add (svg_path_arg_buf_t *arg_buf, double arg)
{
    arg_buf->arg[arg_buf->num_args++] = arg;

    return SVG_STATUS_SUCCESS;
}
