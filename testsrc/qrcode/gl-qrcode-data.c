//
//  gl-qrcode-data.c
//  Photoframe
//
//  Created by Martijn Vernooij on 11/04/2017.
//
//

#include "qrcode/gl-qrcode-data.h"

#include <string.h>
#include <stdlib.h>
#include "qrencode.h"

#define QRCODE_FRAME_SIZE 4

static void gl_qrcode_data_free(gl_object *obj_obj);
static void gl_qrcode_set_string(gl_qrcode_data *obj, const char *string);
static void gl_qrcode_set_minimum_version(gl_qrcode_data *obj, int version);
static void gl_qrcode_set_level(gl_qrcode_data *obj, gl_qrcode_data_level level);
static unsigned int gl_qrcode_get_width(gl_qrcode_data *obj);
static const unsigned char *gl_qrcode_get_output(gl_qrcode_data *obj);
static void gl_qrcode_reset_output(gl_qrcode_data *obj);
static int gl_qrcode_render(gl_qrcode_data *obj);

static struct gl_qrcode_data_funcs gl_qrcode_data_funcs_global = {
	.set_string = &gl_qrcode_set_string,
	.set_minimum_version = &gl_qrcode_set_minimum_version,
	.set_level = &gl_qrcode_set_level,
	.get_width = &gl_qrcode_get_width,
	.get_output = &gl_qrcode_get_output,
	.render = &gl_qrcode_render
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_qrcode_data_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_qrcode_data_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_qrcode_data_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_qrcode_data_free;
}

static void gl_qrcode_set_string(gl_qrcode_data *obj, const char *string)
{
	if (obj->data._string) {
		free (obj->data._string);
		obj->data._string = NULL;
	}
	obj->data._string = strdup(string);
}

static void gl_qrcode_set_minimum_version(gl_qrcode_data *obj, int version)
{
	obj->data._version = version;
}

static void gl_qrcode_set_level(gl_qrcode_data *obj, gl_qrcode_data_level level)
{
	obj->data._level = level;
}

static unsigned int gl_qrcode_get_width(gl_qrcode_data *obj)
{
	return obj->data._width;
}

static const unsigned char *gl_qrcode_get_output(gl_qrcode_data *obj)
{
	return obj->data._output;
}

static void gl_qrcode_reset_output(gl_qrcode_data *obj)
{
	if (obj->data._output) {
		free (obj->data._output);
		obj->data._output = NULL;
	}
	obj->data._width = 0;
}

static QRecLevel liblevel(gl_qrcode_data_level l)
{
	switch (l) {
		case gl_qrcode_data_level_l: return QR_ECLEVEL_L;
		case gl_qrcode_data_level_m: return QR_ECLEVEL_M;
		case gl_qrcode_data_level_q: return QR_ECLEVEL_Q;
		case gl_qrcode_data_level_h: return QR_ECLEVEL_H;
	}
	
	return 0;
}

static int gl_qrcode_render(gl_qrcode_data *obj)
{
	gl_qrcode_reset_output(obj);
	
	QRcode *result = QRcode_encodeString(obj->data._string,
					     obj->data._version,
					     liblevel(obj->data._level),
					     QR_MODE_8,
					     1);
	
	if (!result) {
		return -1;
	}
	
	result->width += 2*QRCODE_FRAME_SIZE;
	
	obj->data._width = result->width;
	obj->data._output = calloc(sizeof(char), result->width * result->width);
	if (!obj->data._output) {
		return -1;
	}
	
	unsigned int xCounter, yCounter;
	size_t outputIndex = 0;
	size_t inputIndex = 0;
	for (yCounter = 0; yCounter < result->width; yCounter++) {
		if ((yCounter < QRCODE_FRAME_SIZE) || ((result->width - yCounter) <= QRCODE_FRAME_SIZE)) {
			for (xCounter = 0; xCounter < result->width; xCounter++) {
				obj->data._output[outputIndex++] = 255;
			}
		} else {
			for (xCounter = 0; xCounter < result->width; xCounter++) {
				if ((xCounter < QRCODE_FRAME_SIZE) || ((result->width - xCounter) <= QRCODE_FRAME_SIZE)) {
					obj->data._output[outputIndex] = 255;
				} else {
					if (result->data[inputIndex] & 1) { // LSB indicates output color, the rest is informational data
						obj->data._output[outputIndex] = 0;
					} else {
						obj->data._output[outputIndex] = 255;
					}
					inputIndex++;
				}
				outputIndex++;
			}
		}
	}
	
	QRcode_free(result);
	
	return 0;
}

gl_qrcode_data *gl_qrcode_data_init(gl_qrcode_data *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_qrcode_data_funcs_global;
	
	obj->data._level = gl_qrcode_data_level_m;
	
	return obj;
}

gl_qrcode_data *gl_qrcode_data_new()
{
	gl_qrcode_data *ret = calloc(1, sizeof(gl_qrcode_data));
	
	return gl_qrcode_data_init(ret);
}

static void gl_qrcode_data_free(gl_object *obj_obj)
{
	gl_qrcode_data *obj = (gl_qrcode_data *)obj_obj;
	
	if (obj->data._string) {
		free (obj->data._string);
		obj->data._string = NULL;
	}
	
	gl_qrcode_reset_output(obj);
	
	gl_object_free_org_global(obj_obj);
}
