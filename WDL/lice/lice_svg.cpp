/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice_svg.cpp (SVG support for LICE)
  See lice.h for license and other information
*/

#include "lice.h"
#include "../libsvg/svg.h"

static LICE_IBitmap *m_curBmp;
static double m_x, m_y;
static double m_opacity = 1.0f;
static int m_color;

static svg_status_t begin_group(void *closure, double opacity)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t begin_element(void *closure)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t end_element(void *closure)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t end_group(void *closure, double opacity)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t move_to(void *closure, double x, double y)
{
  m_x = x; m_y = y;
  return SVG_STATUS_SUCCESS;
}
static svg_status_t line_to(void *closure, double x, double y)
{
  LICE_Line(m_curBmp, m_x, m_y, x, y, m_color, m_opacity, 0);
  m_x = x; m_y = y;
  return SVG_STATUS_SUCCESS;
}
static svg_status_t curve_to(void *closure,
                           double x1, double y1,
                           double x2, double y2,
                           double x3, double y3)
{
  LICE_Line(m_curBmp, m_x, m_y, x1, y1, m_color, m_opacity, 0);
  LICE_Line(m_curBmp, x1, y1, x2, y2, m_color, m_opacity, 0);
  LICE_Line(m_curBmp, x2, y2, x3, y3, m_color, m_opacity, 0);
  //LICE_DrawCBezier(m_curBmp, x1, y1, x2, y2, x2, y2, x3, y3, m_color, m_opacity, 0);
  m_x = x3; m_y = y3;
  return SVG_STATUS_SUCCESS;
}
static svg_status_t quadratic_curve_to(void *closure,
                                     double x1, double y1,
                                     double x2, double y2)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t arc_to(void *closure,
                         double	rx,
                         double	ry,
                         double	x_axis_rotation,
                         int	large_arc_flag,
                         int	sweep_flag,
                         double	x,
                         double	y)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t close_path(void *closure)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_color(void *closure, const svg_color_t *color)
{
  m_color = color->rgb;
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_fill_opacity(void *closure, double fill_opacity)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_fill_paint(void *closure, const svg_paint_t *paint)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_fill_rule(void *closure, svg_fill_rule_t fill_rule)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_font_family(void *closure, const char *family)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_font_size(void *closure, double size)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_font_style(void *closure, svg_font_style_t font_style)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_font_weight(void *closure, unsigned int font_weight)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_opacity(void *closure, double opacity)
{
  m_opacity = opacity;
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_stroke_dash_array(void *closure, double *dash_array, int num_dashes)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_stroke_dash_offset(void *closure, svg_length_t *offset)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_stroke_line_cap(void *closure, svg_stroke_line_cap_t line_cap)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_stroke_line_join(void *closure, svg_stroke_line_join_t line_join)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_stroke_miter_limit(void *closure, double limit)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_stroke_opacity(void *closure, double stroke_opacity)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_stroke_paint(void *closure, const svg_paint_t *paint)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_stroke_width(void *closure, svg_length_t *width)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_text_anchor(void *closure, svg_text_anchor_t text_anchor)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t transform(void *closure,
                            double a, double b,
                            double c, double d,
                            double e, double f)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t apply_view_box(void *closure,
                                 svg_view_box_t view_box,
                                 svg_length_t *width,
                                 svg_length_t *height)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t set_viewport_dimension(void *closure,
                                         svg_length_t *width,
                                         svg_length_t *height)
{
  if(m_curBmp) m_curBmp->resize(width->value, height->value);
  else m_curBmp=new LICE_MemBitmap(width->value,height->value);

  LICE_FillRect(m_curBmp, 0, 0, m_curBmp->getWidth(), m_curBmp->getHeight(), 0, 1.0f);
  return SVG_STATUS_SUCCESS;
}
static svg_status_t render_line(void *closure,
                              svg_length_t *x1,
                              svg_length_t *y1,
                              svg_length_t *x2,
                              svg_length_t *y2)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t render_path(void *closure)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t render_ellipse(void *closure,
                                 svg_length_t *cx,
                                 svg_length_t *cy,
                                 svg_length_t *rx,
                                 svg_length_t *ry)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t render_rect(void *closure,
                              svg_length_t *x,
                              svg_length_t *y,
                              svg_length_t *width,
                              svg_length_t *height,
                              svg_length_t *rx,
                              svg_length_t *ry)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t render_text(void *closure,
                              svg_length_t *x,
                              svg_length_t *y,
                              const char   *utf8)
{
  return SVG_STATUS_SUCCESS;
}
static svg_status_t render_image(void		 *closure,
                               unsigned char *data,
                               unsigned int	 data_width,
                               unsigned int	 data_height,
                               svg_length_t	 *x,
                               svg_length_t	 *y,
                               svg_length_t	 *width,
                               svg_length_t	 *height)
{
  return SVG_STATUS_SUCCESS;
}

LICE_IBitmap *LICE_LoadSVG(const char *filename, LICE_IBitmap *bmp)
{
  svg_t *svg;
  svg_create (&svg);

  if(svg_parse(svg, filename) != SVG_STATUS_SUCCESS)
  {
    svg_destroy(svg);
    return NULL;
  }

  svg_render_engine_t myEngine =
  {
    begin_group,
    begin_element,
    end_element,
    end_group,
    move_to,
    line_to,
    curve_to,
    quadratic_curve_to,
    arc_to,
    close_path,
    set_color,
    set_fill_opacity,
    set_fill_paint,
    set_fill_rule,
    set_font_family,
    set_font_size,
    set_font_style,
    set_font_weight,
    set_opacity,
    set_stroke_dash_array,
    set_stroke_dash_offset,
    set_stroke_line_cap,
    set_stroke_line_join,
    set_stroke_miter_limit,
    set_stroke_opacity,
    set_stroke_paint,
    set_stroke_width,
    set_text_anchor,
    transform,
    apply_view_box,
    set_viewport_dimension,
    render_line,
    render_path,
    render_ellipse,
    render_rect,
    render_text,
    render_image,
  };

  m_curBmp = bmp;
  svg_render(svg, &myEngine, NULL);

  svg_destroy(svg);

  return m_curBmp;
}
