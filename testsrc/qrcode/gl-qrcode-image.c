//
//  gl-qrcode.c
//  Photoframe
//
//  Created by Martijn Vernooij on 11/04/2017.
//
//

#include <string.h>
#include <stdlib.h>

#include "qrcode/gl-qrcode-image.h"
#include "qrcode/gl-qrcode-data.h"

#include "gl-texture.h"

static void gl_qrcode_image_free(gl_object *obj_obj);
static void gl_qrcode_image_clear(gl_qrcode_image *obj);
int gl_qrcode_image_set_string(gl_qrcode_image *obj, const char* string);

static struct gl_qrcode_image_funcs gl_qrcode_image_funcs_global = {
	.set_string = &gl_qrcode_image_set_string,
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_qrcode_image_setup()
{
	gl_container_2d *parent = gl_container_2d_new();
	memcpy(&gl_qrcode_image_funcs_global.p, parent->f, sizeof(gl_container_2d_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_qrcode_image_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_qrcode_image_free;
}

int gl_qrcode_image_set_string(gl_qrcode_image *obj, const char* string)
{
	gl_qrcode_image_clear(obj);
	
	gl_qrcode_data *qr_data = gl_qrcode_data_new();
	
	if (!qr_data) {
		return -1;
	}
	
	qr_data->f->set_string(qr_data, string);
	
	if (qr_data->f->render(qr_data)) {
		goto EXIT_1;
	}
	
	gl_texture *new_texture = gl_texture_new();
	if (!new_texture) {
		goto EXIT_1;
	}
	
	new_texture->f->set_immediate(new_texture, 1);
	
	gl_bitmap *new_bitmap = gl_bitmap_new();
	if (!new_bitmap) {
		goto EXIT_2;
	}
	
	unsigned int width = qr_data->f->get_width(qr_data);
	new_bitmap->data.width = width;
	new_bitmap->data.height = width;
	new_bitmap->data.bitmap = calloc (width * width * 4, sizeof(unsigned char));
	
	if (!new_bitmap->data.bitmap) {
		goto EXIT_3;
	}
	size_t imageSize = width * width;
	
	size_t expandCounter_i, expandCounter_o;
	expandCounter_o = 0;
	const unsigned char *o = qr_data->f->get_output(qr_data);
	for (expandCounter_i = 0; expandCounter_i < imageSize; expandCounter_i++)
	{
		new_bitmap->data.bitmap[expandCounter_o++] = o[expandCounter_i];
		new_bitmap->data.bitmap[expandCounter_o++] = o[expandCounter_i];
		new_bitmap->data.bitmap[expandCounter_o++] = o[expandCounter_i];
		new_bitmap->data.bitmap[expandCounter_o++] = 255;
	}
	
	new_texture->f->load_image(new_texture, new_bitmap, width, width);
	
	obj->data._image = gl_tile_new();
	if (!obj->data._image) {
		goto EXIT_3;
	}
	
	obj->data._image->f->set_texture(obj->data._image, new_texture);
	
	((gl_object *)(obj->data._image))->f->ref((gl_object *)obj->data._image);
	((gl_container *)obj)->f->append_child((gl_container *)obj, (gl_shape *)obj->data._image);
	
	((gl_object *)new_bitmap)->f->unref((gl_object *)new_bitmap);
	((gl_object *)qr_data)->f->unref((gl_object *)qr_data);
	
	obj->data.size = width;
	
	return 0;
	
EXIT_3:
	((gl_object *)new_bitmap)->f->unref((gl_object *)new_bitmap);
	
EXIT_2:
	((gl_object *)new_texture)->f->unref((gl_object *)new_texture);

EXIT_1:
	((gl_object *)qr_data)->f->unref((gl_object *)qr_data);
	return -1;
}


gl_qrcode_image *gl_qrcode_image_init(gl_qrcode_image *obj)
{
	gl_container_2d_init((gl_container_2d *)obj);
	
	obj->f = &gl_qrcode_image_funcs_global;
	
	return obj;
}

gl_qrcode_image *gl_qrcode_image_new()
{
	gl_qrcode_image *ret = calloc(1, sizeof(gl_qrcode_image));
	
	return gl_qrcode_image_init(ret);
}

static void gl_qrcode_image_clear(gl_qrcode_image *obj)
{
	if (obj->data._image) {
		((gl_object *)obj->data._image)->f->unref((gl_object *)obj->data._image);

		((gl_container *)obj)->f->remove_child((gl_container *)obj, (gl_shape *)obj->data._image);
		
		obj->data._image = NULL;
	}
}

static void gl_qrcode_image_free(gl_object *obj_obj)
{
	gl_qrcode_image *obj = (gl_qrcode_image *)obj_obj;
	
	gl_qrcode_image_clear(obj);
	
	gl_object_free_org_global(obj_obj);
}
