/* svg_image.c: Data structures for SVG image elements
 
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
#include "../libpng/png.h"
#include "../jpeglib/jpeglib.h"
#include "../jpeglib/jerror.h"
#include <setjmp.h>

#include "svgint.h"

static svg_status_t
_svg_image_read_image (svg_image_t *image);

static svg_status_t
_svg_image_read_png (const char		*filename,
		     char	 	**data,
		     unsigned int	*width,
		     unsigned int	*height);

static svg_status_t
_svg_image_read_jpeg (const char	*filename,
		      char	 	**data,
		      unsigned int	*width,
		      unsigned int	*height);

svg_status_t
_svg_image_init (svg_image_t *image)
{
    _svg_length_init_unit (&image->x, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_HORIZONTAL);
    _svg_length_init_unit (&image->y, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_VERTICAL);
    _svg_length_init_unit (&image->width, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_HORIZONTAL);
    _svg_length_init_unit (&image->height, 0, SVG_LENGTH_UNIT_PX, SVG_LENGTH_ORIENTATION_VERTICAL);

    image->url = NULL;

    image->data = NULL;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_image_init_copy (svg_image_t *image,
		      svg_image_t *other)
{
    *image = *other;
    if (other->url)
	image->url = strdup (other->url);
    else
	image->url = NULL;

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_image_deinit (svg_image_t *image)
{
    if (image->url) {
	free (image->url);
	image->url = NULL;
    }

    if (image->data) {
	free (image->data);
	image->data = NULL;
    }

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_image_apply_attributes (svg_image_t	*image,
			     const char		**attributes)
{
    const char *aspect, *href;

    _svg_attribute_get_length (attributes, "x", &image->x, "0");
    _svg_attribute_get_length (attributes, "y", &image->y, "0");
    _svg_attribute_get_length (attributes, "width", &image->width, "0");
    _svg_attribute_get_length (attributes, "height", &image->height, "0");
    /* XXX: I'm not doing anything with preserveAspectRatio yet */
    _svg_attribute_get_string (attributes,
			       "preserveAspectRatio",
			       &aspect,
			       "xMidyMid meet");
    /* XXX: This is 100% bogus with respect to the XML namespaces spec. */
    _svg_attribute_get_string (attributes, "xlink:href", &href, "");

    if (image->width.value < 0 || image->height.value < 0)
	return SVG_STATUS_PARSE_ERROR;

    /* XXX: We really need to do something like this to resolve
       relative URLs. It involves linking the tree up in the other
       direction. Or, another approach would be to simply throw out
       the SAX parser and use the tree interface of libxml2 which
       takes care of things like xml:base for us.

    image->url = _svg_element_resolve_uri_alloc (image->element, href);

       For now, the bogus code below will let me test the rest of the
       image support:
    */

    image->url = strdup ((char*)href);

    return SVG_STATUS_SUCCESS;
}

svg_status_t
_svg_image_render (svg_image_t		*image,
		   svg_render_engine_t	*engine,
		   void			*closure)
{
    svg_status_t status;

    if (image->width.value == 0 || image->height.value == 0)
	return SVG_STATUS_SUCCESS;

    status = _svg_image_read_image (image);
    if (status)
	return status;

    status = (engine->render_image) (closure,
				     (unsigned char*) image->data,
				     image->data_width,
				     image->data_height,
				     &image->x,
				     &image->y,
				     &image->width,
				     &image->height);
    if (status)
	return status;

    return SVG_STATUS_SUCCESS;
}

static svg_status_t
_svg_image_read_image (svg_image_t *image)
{
    svgint_status_t status;

    if (image->data)
	return SVG_STATUS_SUCCESS;

    /* XXX: _svg_image_read_png only deals with filenames, not URLs */
    status = _svg_image_read_png (image->url,
				  &image->data,
				  &image->data_width,
				  &image->data_height);
    if (status == 0)
	return SVG_STATUS_SUCCESS;

    if (status != SVGINT_STATUS_IMAGE_NOT_PNG)
	return status;

    /* XXX: _svg_image_read_jpeg only deals with filenames, not URLs */
    status = _svg_image_read_jpeg (image->url,
				   &image->data,
				   &image->data_width,
				   &image->data_height);
    if (status == 0)
	return SVG_STATUS_SUCCESS;

    /* XXX: need to support SVG images as well */

    if (status != SVGINT_STATUS_IMAGE_NOT_JPEG)
	return status;

    return SVG_STATUS_PARSE_ERROR;
}

static void
premultiply_data (png_structp png, png_row_infop row_info, png_bytep data)
{
    int i;
  
    for (i = 0; i < row_info->rowbytes; i += 4) {
	unsigned char *b = &data[i];
	unsigned char alpha = b[3];
	unsigned long pixel = ((((b[0] * alpha) / 255) << 0) |
			       (((b[1] * alpha) / 255) << 8) |
			       (((b[2] * alpha) / 255) << 16) |
			       (alpha << 24));
	unsigned long *p = (unsigned long *) b;
	*p = pixel;
    }
}

static svg_status_t
_svg_image_read_png (const char		*filename,
		     char	 	**data,
		     unsigned int	*width,
		     unsigned int	*height)
{
    int i;
    FILE *file;
#define PNG_SIG_SIZE 8
    unsigned char png_sig[PNG_SIG_SIZE];
    int sig_bytes;
    png_struct *png;
    png_info *info;
    png_uint_32 png_width, png_height;
    int depth, color_type, interlace;
    unsigned int pixel_size;
    png_byte **row_pointers;

    file = fopen (filename, "rb");
    if (file == NULL)
	return SVG_STATUS_FILE_NOT_FOUND;

    sig_bytes = fread (png_sig, 1, PNG_SIG_SIZE, file);
    if (png_check_sig (png_sig, sig_bytes) == 0) {
	fclose (file);
	return SVGINT_STATUS_IMAGE_NOT_PNG;
    }

    /* XXX: Perhaps we'll want some other error handlers? */
    png = png_create_read_struct (PNG_LIBPNG_VER_STRING,
				  NULL,
				  NULL,
				  NULL);
    if (png == NULL) {
	fclose (file);
	return SVG_STATUS_NO_MEMORY;
    }

    info = png_create_info_struct (png);
    if (info == NULL) {
	fclose (file);
	png_destroy_read_struct (&png, NULL, NULL);
	return SVG_STATUS_NO_MEMORY;
    }

    png_init_io (png, file);
    png_set_sig_bytes (png, sig_bytes);

    png_read_info (png, info);

    png_get_IHDR (png, info,
		  &png_width, &png_height, &depth,
		  &color_type, &interlace, NULL, NULL);
    *width = png_width;
    *height = png_height;

    /* XXX: I still don't know what formats will be exported in the
       libsvg -> svg_render_engine interface. For now, I'm converting
       everything to 32-bit RGBA. */

    /* convert palette/gray image to rgb */
    if (color_type == PNG_COLOR_TYPE_PALETTE)
	png_set_palette_to_rgb (png);

    /* expand gray bit depth if needed */
    if (color_type == PNG_COLOR_TYPE_GRAY && depth < 8)
	png_set_gray_1_2_4_to_8 (png);

    /* transform transparency to alpha */
    if (png_get_valid(png, info, PNG_INFO_tRNS))
	png_set_tRNS_to_alpha (png);

    if (depth == 16)
	png_set_strip_16 (png);

    if (depth < 8)
	png_set_packing (png);

    /* convert grayscale to RGB */
    if (color_type == PNG_COLOR_TYPE_GRAY
	|| color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	png_set_gray_to_rgb (png);

    if (interlace != PNG_INTERLACE_NONE)
	png_set_interlace_handling (png);

    png_set_bgr (png);
    png_set_filler (png, 0xff, PNG_FILLER_AFTER);

    png_set_read_user_transform_fn (png, premultiply_data);

    png_read_update_info (png, info);

    pixel_size = 4;
    *data = malloc (png_width * png_height * pixel_size);
    if (*data == NULL) {
	fclose (file);
	return SVG_STATUS_NO_MEMORY;
    }

    row_pointers = malloc (png_height * sizeof(char *));
    for (i=0; i < png_height; i++)
	row_pointers[i] = (png_byte *) (*data + i * png_width * pixel_size);

    png_read_image (png, row_pointers);
    png_read_end (png, info);

    free (row_pointers);
    fclose (file);

    png_destroy_read_struct (&png, &info, NULL);

    return SVG_STATUS_SUCCESS;
}

typedef struct _svg_image_jpeg_err {
    struct jpeg_error_mgr pub;    /* "public" fields */
    jmp_buf setjmp_buf;           /* for return to caller */
} svg_image_jpeg_err_t;

static void
_svg_image_jpeg_error_exit (j_common_ptr cinfo)
{
    svgint_status_t status;
    svg_image_jpeg_err_t *err = (svg_image_jpeg_err_t *) cinfo->err;

    /* Are there any other error codes we might care about? */
    switch (err->pub.msg_code) {
    case JERR_NO_SOI:
	status = SVGINT_STATUS_IMAGE_NOT_JPEG;
	break;
    default:
	status = SVG_STATUS_PARSE_ERROR;
	break;
    }

    longjmp (err->setjmp_buf, status);
}

static svg_status_t
_svg_image_read_jpeg (const char	*filename,
		      char	 	**data,
		      unsigned int	*width,
		      unsigned int	*height)
{
    FILE *file;
    svgint_status_t status;
    struct jpeg_decompress_struct cinfo;
    svg_image_jpeg_err_t jpeg_err;
    JSAMPARRAY buf;
    int i, row_stride;
    unsigned char *out, *in;
    
    file = fopen (filename, "rb");
    if (file == NULL)
	return SVG_STATUS_FILE_NOT_FOUND;
    
    cinfo.err = jpeg_std_error (&jpeg_err.pub);
    jpeg_err.pub.error_exit = _svg_image_jpeg_error_exit;
    
    status = setjmp (jpeg_err.setjmp_buf);
    if (status) {
	jpeg_destroy_decompress(&cinfo);
	fclose(file);
	return status;
    }
    
    jpeg_create_decompress (&cinfo);
    jpeg_stdio_src (&cinfo, file);
    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);
    
    row_stride = cinfo.output_width * cinfo.output_components;
    *width = cinfo.output_width;
    *height= cinfo.output_height;
    buf = (*cinfo.mem->alloc_sarray)
	((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
    
    *data = malloc (cinfo.output_width * cinfo.output_height * 4);
    out = (unsigned char*) *data;
    while (cinfo.output_scanline < cinfo.output_height) {
	jpeg_read_scanlines (&cinfo, buf, 1);
	in = buf[0];
	for (i=0; i < cinfo.output_width; i++ ) {
	    switch (cinfo.num_components) {
	    case 1:
		out[3] = 0xff;
		out[2] = in[0];
		out[1] = in[1];
		out[0] = in[2];
		in += 1;
		out += 4;
		break;
	    default:
	    case 4:
		out[3] = 0xff;
		out[2] = in[0];
		out[1] = in[1];
		out[0] = in[2];
		in += 3;
		out += 4;
	    }
	}
    }
    jpeg_finish_decompress (&cinfo);
    jpeg_destroy_decompress (&cinfo);
    fclose(file);
    
    return SVG_STATUS_SUCCESS;
}

