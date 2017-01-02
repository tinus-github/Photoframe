//
//  gl-label.c
//  Photoframe
//
//  Created by Martijn Vernooij on 02/01/2017.
//
//

#include "gl-label.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <assert.h>

#define FONT_FILE "/home/pi/lib/share/font/DejaVuSerif.ttf"
#define LABEL_HEIGHT 64

void gl_label_free(gl_object *obj);

static struct gl_label_funcs gl_label_funcs_global = {
	.render = &gl_label_render
};

struct rendering_data {
	FT_Library library;
	FT_Face face;
	
};

struct rendering_data global_rendering_data;

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_label_setup_freetype()
{
	FT_Error errorret;
	assert (!(errorret = FT_Init_FreeType(&global_rendering_data.library)));
	
	assert (!(errorret = FT_New_Face(global_rendering_data.library, FONT_FILE, 0, &global_rendering_data.face)));

	assert (!(errorret = FT_Set_Char_Size(global_rendering_data.face, 0, LABEL_HEIGHT*64, 72,72)));
}

void gl_label_setup()
{
	gl_shape *parent = gl_shape_new();
	gl_object *parent_obj = (gl_object *)parent;
	memcpy(&gl_label_funcs_global.p, parent->f, sizeof(gl_shape_funcs));
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_label_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_label_free;
	
	parent_obj->f->free(parent_obj);
	
	gl_label_setup_freetype();
}

gl_label *gl_label_init(gl_label *obj)
{
	gl_shape_init((gl_shape *)obj);
	
	obj->f = &gl_label_funcs_global;
	
	return obj;
}

gl_label *gl_label_new()
{
	gl_label *ret = calloc(1, sizeof(gl_label));
	
	return gl_label_init(ret);
}

void gl_label_free(gl_object *obj_obj)
{
	gl_label *obj = (gl_label *)obj_obj;
	free (obj->data.text);
	
	gl_object_free_org_global(obj_obj);
}
