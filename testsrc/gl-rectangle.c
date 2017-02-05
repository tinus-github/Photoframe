//
//  gl-rectangle.c
//  Photoframe
//
//  Created by Martijn Vernooij on 10/01/2017.
//
//

#include "gl-rectangle.h"
#include "driver.h"
#include "gl-stage.h"

#include <string.h>
#include <stdlib.h>

static void gl_rectangle_draw(gl_shape *self);
static void gl_rectangle_set_color(gl_rectangle *obj, GLfloat r, GLfloat g, GLfloat b, GLfloat alpha);

static struct gl_rectangle_funcs gl_rectangle_funcs_global = {
	.set_color = &gl_rectangle_set_color
};

typedef struct gl_rectangle_program_data {
	GLuint program;
	GLint positionLoc;
	GLint projectionLoc;
	GLint modelViewLoc;
	GLint colorLoc;
} gl_rectangle_program_data;

static gl_rectangle_program_data gl_rgba_program;

static uint gl_rectangle_program_loaded = 0;

static void gl_rectangle_load_program_attribute_locations(gl_rectangle_program_data *data)
{
	GLuint program = data->program;
	// Get the attribute locations
	data->positionLoc = glGetAttribLocation ( program, "a_position" );
	
	// Get the uniform locations
	data->projectionLoc = glGetUniformLocation ( program, "u_projection" );
	data->modelViewLoc  = glGetUniformLocation ( program, "u_modelView" );
	data->colorLoc = glGetUniformLocation ( program, "u_color" );
}

static int gl_rectangle_load_program() {
	GLchar vShaderStr[] =
	"attribute vec4 a_position;            \n"
	"uniform mat4 u_projection;            \n"
	"uniform mat4 u_modelView;             \n"
	"void main()                           \n"
	"{                                     \n"
	"   vec4 p = u_modelView * a_position; \n"
	"   gl_Position = u_projection * p;    \n"
	"}                                     \n";
	
	GLchar fShaderStr[] =
	"precision mediump float;                            \n"
	"uniform vec4 u_color;                               \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  gl_FragColor = vec4(u_color.r * u_color.a, u_color.g * u_color.a, u_color.b * u_color.a, u_color.a); \n"
	"}                                                   \n";
	
	// Load the shaders and get a linked program object
	gl_rgba_program.program = driver_load_program ( vShaderStr, fShaderStr );
	gl_rectangle_load_program_attribute_locations(&gl_rgba_program);
	
	gl_rectangle_program_loaded = 1;
	
	return GL_TRUE;
}

void gl_rectangle_setup()
{
	gl_shape *parent = gl_shape_new();
	memcpy(&gl_rectangle_funcs_global.p, parent->f, sizeof(gl_shape_funcs));
	
	gl_shape_funcs *shapef = (gl_shape_funcs *)&gl_rectangle_funcs_global;
	shapef->draw = &gl_rectangle_draw;
}

gl_rectangle *gl_rectangle_init(gl_rectangle *obj)
{
	gl_shape_init((gl_shape *)obj);
	
	obj->f = &gl_rectangle_funcs_global;
	
	return obj;
}

gl_rectangle *gl_rectangle_new()
{
	gl_rectangle *ret = calloc(1, sizeof(gl_rectangle));
	
	return gl_rectangle_init(ret);
}

static void gl_rectangle_set_color(gl_rectangle *obj, GLfloat r, GLfloat g, GLfloat b, GLfloat alpha)
{
	obj->data.color[0] = r;
	obj->data.color[1] = g;
	obj->data.color[2] = b;
	obj->data.color[3] = alpha;
}

///
// Draw triangles using the shader pair created in Init()
//
static void gl_rectangle_draw(gl_shape *shape_self)
{
	gl_rectangle *self = (gl_rectangle *)shape_self;
	gl_stage *stage = gl_stage_get_global_stage();
	
	if (!gl_rectangle_program_loaded) {
		gl_rectangle_load_program();
	}
	
	gl_rectangle_program_data *program;
	
	program = &gl_rgba_program;
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	
	GLfloat vVertices[] = { 0.0f, 0.0f, 0.0f,  // Position 0
		0.0f,  1.0f, 0.0f,  // Position 1
		1.0f,  1.0f, 0.0f,  // Position 2
		1.0f,  0.0f, 0.0f,  // Position 3
	};
	
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
	
	// Use the program object
	glUseProgram ( program->program );
	
	// Load the vertex position
	glVertexAttribPointer ( program->positionLoc, 3, GL_FLOAT,
			       GL_FALSE, 3 * sizeof(GLfloat), vVertices );
	
	glEnableVertexAttribArray ( program->positionLoc );
	
	shape_self->f->compute_projection(shape_self);
	
	glUniformMatrix4fv(program->projectionLoc, 1, GL_FALSE, (GLfloat *)stage->data.projection);
	glUniformMatrix4fv(program->modelViewLoc,  1, GL_FALSE, (GLfloat *)shape_self->data.computed_modelView);
	
	shape_self->f->compute_alpha(shape_self);
	
	GLfloat finalAlpha = shape_self->data.computedAlpha * self->data.color[3];
	
	glUniform4f(program->colorLoc, self->data.color[0], self->data.color[1], self->data.color[2], finalAlpha);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}
