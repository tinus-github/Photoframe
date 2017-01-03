//
//  gl-tile.c
//  Photoframe
//
//  Created by Martijn Vernooij on 15/11/2016.
//
//

#include "gl-tile.h"
#include "egl-driver.h"
#include "gl-stage.h"

static void gl_tile_set_texture(gl_tile *obj, gl_texture *texture);
static void gl_tile_draw(gl_shape *self);

static struct gl_tile_funcs gl_tile_funcs_global = {
	.set_texture = &gl_tile_set_texture,
};

static gl_shape *gl_tile_obj_parent;

static GLuint gl_tile_programObject;
static GLint
		gl_tile_programPositionLoc,
		gl_tile_programTexCoordLoc,
		gl_tile_programProjectionLoc,
		gl_tile_programModelViewLoc,
		gl_tile_programSamplerLoc;

static uint gl_tile_program_loaded = 0;

static int gl_tile_load_program() {
	GLchar vShaderStr[] =
	"attribute vec4 a_position;            \n"
	"attribute vec2 a_texCoord;            \n"
	"uniform mat4 u_projection;            \n"
	"uniform mat4 u_modelView;             \n"
	"varying vec2 v_texCoord;              \n"
	"void main()                           \n"
	"{                                     \n"
	"   vec4 p = u_modelView * a_position; \n"
	"   gl_Position = u_projection * p;    \n"
	"   v_texCoord = a_texCoord;           \n"
	"}                                     \n";
	
	GLchar fShaderStr[] =
	"precision mediump float;                            \n"
	"varying vec2 v_texCoord;                            \n"
	"uniform sampler2D s_texture;                        \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
	"}                                                   \n";
	
	// Load the shaders and get a linked program object
	gl_tile_programObject = egl_driver_load_program ( vShaderStr, fShaderStr );
	
	// Get the attribute locations
	gl_tile_programPositionLoc = glGetAttribLocation ( gl_tile_programObject, "a_position" );
	gl_tile_programTexCoordLoc = glGetAttribLocation ( gl_tile_programObject, "a_texCoord" );
	
	// Get the uniform locations
	gl_tile_programProjectionLoc = glGetUniformLocation ( gl_tile_programObject, "u_projection" );
	gl_tile_programModelViewLoc  = glGetUniformLocation ( gl_tile_programObject, "u_modelView" );
	
	// Get the sampler location
	gl_tile_programSamplerLoc = glGetUniformLocation ( gl_tile_programObject, "s_texture" );
	
	
	gl_tile_program_loaded = 1;
	
	return GL_TRUE;
}


// Takes over the reference that was held by the caller
// You may also send NULL to remove the texture
static void gl_tile_set_texture(gl_tile *obj, gl_texture *texture)
{
	gl_texture *org_texture = obj->data.texture;
	gl_shape *shape_self = (gl_shape *)obj;
	
	if (org_texture) {
		gl_object *org_texture_obj = (gl_object *)org_texture;
		
		org_texture_obj->f->unref(org_texture_obj);
	}
	
	obj->data.texture = texture;
	
	if (texture) {
		shape_self->data.objectWidth = texture->data.width;
		shape_self->data.objectHeight = texture->data.height;
	} else {
		shape_self->data.objectWidth = 0;
		shape_self->data.objectHeight = 0;
	}
	
	shape_self->f->set_computed_projection_dirty(shape_self);
}

static void gl_tile_free(gl_object *tile_obj)
{
	gl_tile *tile = (gl_tile *)tile_obj;
	
	tile->f->set_texture(tile, NULL);
	
	gl_object *parent_obj = (gl_object *)gl_tile_obj_parent;
	parent_obj->f->free(tile_obj);
}

void gl_tile_setup()
{
	gl_shape *parent = gl_shape_new();
	memcpy(&gl_tile_funcs_global.p, parent->f, sizeof(gl_shape_funcs));
	
	gl_object_funcs *objf = (gl_object_funcs *)&gl_tile_funcs_global;
	objf->free = &gl_tile_free;
	
	gl_shape_funcs *shapef = (gl_shape_funcs *)&gl_tile_funcs_global;
	shapef->draw = &gl_tile_draw;
	
	gl_tile_obj_parent = parent;
}

gl_tile *gl_tile_init(gl_tile *obj)
{
	gl_shape_init((gl_shape *)obj);
	
	obj->f = &gl_tile_funcs_global;
	obj->data.texture = NULL;
	
	return obj;
}

gl_tile *gl_tile_new()
{
	gl_tile *ret = calloc(1, sizeof(gl_tile));
	
	return gl_tile_init(ret);
}


///
// Draw triangles using the shader pair created in Init()
//
static void gl_tile_draw(gl_shape *shape_self)
{
	gl_tile *self = (gl_tile *)shape_self;
	gl_texture *texture = self->data.texture;
	gl_stage *stage = gl_stage_get_global_stage();
	
	if (!gl_tile_program_loaded) {
		gl_tile_load_program();
	}
	
	GLfloat vVertices[] = { 0.0f, 0.0f, 0.0f,  // Position 0
		0.0f,  0.0f,        // TexCoord 0
		0.0f,  1.0f, 0.0f,  // Position 1
		0.0f,  1.0f,        // TexCoord 1
		1.0f,  1.0f, 0.0f,  // Position 2
		1.0f,  1.0f,        // TexCoord 2
		1.0f,  0.0f, 0.0f,  // Position 3
		1.0f,  0.0f         // TexCoord 3
	};
	
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
	
	// Use the program object
	glUseProgram ( gl_tile_programObject );
	
	// Load the vertex position
	glVertexAttribPointer ( gl_tile_programPositionLoc, 3, GL_FLOAT,
			       GL_FALSE, 5 * sizeof(GLfloat), vVertices );
	// Load the texture coordinate
	glVertexAttribPointer ( gl_tile_programTexCoordLoc, 2, GL_FLOAT,
			       GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] );
	
	glEnableVertexAttribArray ( gl_tile_programPositionLoc );
	glEnableVertexAttribArray ( gl_tile_programTexCoordLoc );
	
	// Bind the texture
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, texture->data.textureId );
	
	// Set the sampler texture unit to 0
	glUniform1i ( gl_tile_programSamplerLoc, 0 );
	
	shape_self->f->compute_projection(shape_self);
	
	glUniformMatrix4fv ( gl_tile_programProjectionLoc, 1, GL_FALSE, (GLfloat *)stage->data.projection);
	glUniformMatrix4fv ( gl_tile_programModelViewLoc,  1, GL_FALSE, (GLfloat *)shape_self->data.computed_modelView);

	
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
}
