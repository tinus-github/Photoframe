//
//  gl-texture.c
//  Photoframe
//
//  Created by Martijn Vernooij on 13/05/16.
//
//

#include "gl-texture.h"

static GLuint load_image(gl_texture *obj, char *rgba_data, unsigned int width, unsigned int height);
static void gl_texture_free();

static struct gl_texture_funcs gl_texture_funcs_global = {
	.load_image = &load_image
};

void gl_texture_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(gl_texture_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
	
	gl_texture_funcs_global.gl_object_free = gl_texture_funcs_global.p.free;
	gl_texture_funcs_global.p.free = &gl_texture_free;
}

gl_texture *gl_texture_init(gl_texture *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_texture_funcs_global;
	
	return obj;
}

gl_texture *gl_texture_new()
{
	gl_texture *ret = malloc(sizeof(gl_texture));
	
	return gl_texture_init(ret);
}


static GLuint load_image(gl_texture *obj, char *rgba_data, unsigned int width, unsigned int height)
{
	// Texture object handle
	GLuint textureId;
	
	// Use tightly packed data
	glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
	
	// Generate a texture object
	glGenTextures ( 1, &textureId );
	
	// Bind the texture object
	glBindTexture ( GL_TEXTURE_2D, textureId );
	
	// Load the texture
	
	
	glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA,
		      width, height,
		      0, GL_RGBA, GL_UNSIGNED_BYTE, rgba_data );
	
	// Set the filtering mode
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	
	obj->data.textureId = textureId;
	obj->data.texture_loaded = 1;
	
	return textureId;
}

static void gl_texture_free_texture(gl_texture *obj)
{
	glDeleteTextures ( 1, &obj->data.textureId );
	obj->data.texture_loaded = 0;
}

static void gl_texture_free(gl_object *obj_obj)
{
	gl_texture *obj = (gl_texture *)obj_obj;
	if (obj->data.texture_loaded) {
		gl_texture_free_texture(obj);
	}
	obj->f->free(obj_obj);
}