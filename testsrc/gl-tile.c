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

typedef struct gl_tile_program_data {
	GLuint program;
	GLint positionLoc;
	GLint texCoordLoc;
	GLint projectionLoc;
	GLint modelViewLoc;
	GLint alphaLoc;
	GLint samplerLoc;
} gl_tile_program_data;

static gl_tile_program_data gl_rgba_program;
static gl_tile_program_data gl_mono_program;
static gl_tile_program_data gl_alpha_program;
static gl_tile_program_data gl_flip_program;

static uint gl_tile_program_loaded = 0;

static void gl_tile_load_program_attribute_locations(gl_tile_program_data *data)
{
	GLuint program = data->program;
	// Get the attribute locations
	data->positionLoc = glGetAttribLocation ( program, "a_position" );
	data->texCoordLoc = glGetAttribLocation ( program, "a_texCoord" );
	
	// Get the uniform locations
	data->projectionLoc = glGetUniformLocation ( program, "u_projection" );
	data->modelViewLoc  = glGetUniformLocation ( program, "u_modelView" );
	data->alphaLoc = glGetUniformLocation ( program, "u_alpha" );
	
	// Get the sampler location
	data->samplerLoc = glGetUniformLocation ( program, "s_texture" );
}

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
	"uniform float u_alpha;                              \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  vec4 color = texture2D( s_texture, v_texCoord );  \n"
	"  color.a = color.a * u_alpha;                      \n"
	"  gl_FragColor = vec4(color.r * color.a, color.g * color.a, color.b * color.a, color.a);\n"
	"}                                                   \n";
	
	GLchar fShaderBWStr[] =
	"precision mediump float;                            \n"
	"varying vec2 v_texCoord;                            \n"
	"uniform sampler2D s_texture;                        \n"
	"uniform float u_alpha;                              \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  vec4 texelColor = texture2D( s_texture, v_texCoord );\n"
	"  float luminance = texelColor.a;                   \n"
	"  gl_FragColor = vec4(luminance, luminance, luminance, u_alpha);\n"
	"}                                                   \n";
	
	GLchar fShaderAlphaStr[] =
	"precision mediump float;                            \n"
	"varying vec2 v_texCoord;                            \n"
	"uniform sampler2D s_texture;                        \n"
	"uniform float u_alpha;                              \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  vec4 texelColor = texture2D( s_texture, v_texCoord );\n"
	"  float luminance = texelColor.a * u_alpha;         \n"
	"  gl_FragColor = vec4(luminance, luminance, luminance, luminance);\n"
	"}                                                   \n";
	
	GLchar fShaderFlipAlphaStr[] =
	"precision mediump float;                            \n"
	"varying vec2 v_texCoord;                            \n"
	"uniform sampler2D s_texture;                        \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  vec4 texelColor = texture2D( s_texture, v_texCoord );\n"
	"  float luminance = 1 - texelColor.a;               \n"
	"  gl_FragColor = vec4(luminance, luminance, luminance, luminance);\n"
	"}                                                   \n";
	
	// Load the shaders and get a linked program object
	gl_rgba_program.program = egl_driver_load_program ( vShaderStr, fShaderStr );
	gl_tile_load_program_attribute_locations(&gl_rgba_program);
	
	// Monochrome
	gl_mono_program.program = egl_driver_load_program ( vShaderStr, fShaderBWStr );
	gl_tile_load_program_attribute_locations(&gl_mono_program);

	// Alpha
	gl_alpha_program.program = egl_driver_load_program ( vShaderStr, fShaderAlphaStr );
	gl_tile_load_program_attribute_locations(&gl_alpha_program);

	// Flip (testing)
	gl_flip_program.program = egl_driver_load_program ( vShaderStr, fShaderFlipAlphaStr );
	gl_tile_load_program_attribute_locations(&gl_flip_program);
	
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
	
	if (!texture || (texture->data.loadState != gl_texture_loadstate_done)) {
		return;
	}
	
	gl_tile_program_data *program;
	
	switch (texture->data.dataType) {
		default:
		case gl_texture_data_type_rgba:
			program = &gl_rgba_program;
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			break;
		case gl_texture_data_type_monochrome:
			program = &gl_mono_program;
			break;
		case gl_texture_data_type_alpha:
			program = &gl_alpha_program;
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			break;
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
	
	// Bind the texture
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, texture->data.textureId );
	
	// Use the program object
	glUseProgram ( program->program );
	
	// Load the vertex position
	glVertexAttribPointer ( program->positionLoc, 3, GL_FLOAT,
			       GL_FALSE, 5 * sizeof(GLfloat), vVertices );
	// Load the texture coordinate
	glVertexAttribPointer ( program->texCoordLoc, 2, GL_FLOAT,
			       GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] );
	
	glEnableVertexAttribArray ( program->positionLoc );
	glEnableVertexAttribArray ( program->texCoordLoc );
	
	// Set the sampler texture unit to 0
	glUniform1i ( program->samplerLoc, 0 );
	
	shape_self->f->compute_projection(shape_self);
	
	glUniformMatrix4fv(program->projectionLoc, 1, GL_FALSE, (GLfloat *)stage->data.projection);
	glUniformMatrix4fv(program->modelViewLoc,  1, GL_FALSE, (GLfloat *)shape_self->data.computed_modelView);

	shape_self->f->compute_alpha(shape_self);
	
	glUniform1f(program->alphaLoc, shape_self->data.computedAlpha);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}
