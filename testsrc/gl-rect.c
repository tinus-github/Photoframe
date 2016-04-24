//
//  gl-rect.c
//  Photoframe
//
//  Created by Martijn Vernooij on 24/04/16.
//
//

#include "gl-rect.h"
#include "gl-display.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

#include "../lib/linmath/linmath.h"

static int Init_rect_gl_state(GL_STATE_T *p_state) {
	p_state->rectDisplayData = malloc(sizeof(GLRectDisplayData));
	GLRectDisplayData *displayData = p_state->rectDisplayData;
	
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
	"uniform vec3 u_color;                               \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  gl_FragColor = vec4(u_color.r, u_color.g, u_color.b, 1.0);\n"
	"}                                                   \n";
	
	// Load the shaders and get a linked program object
	displayData->programObject = gl_display_load_program ( vShaderStr, fShaderStr );
	
	// Get the attribute locations
	displayData->positionLoc = glGetAttribLocation ( displayData->programObject, "a_position" );
	
	// Get the uniform locations
	displayData->projectionLoc = glGetUniformLocation ( displayData->programObject, "u_projection" );
	displayData->modelViewLoc  = glGetUniformLocation ( displayData->programObject, "u_modelView" );
	displayData->colorLoc = glGetUniformLocation ( displayData->programObject, "u_color" );
	
	return GL_TRUE;
}

///
// Initialize the shader and program object
//
int gl_rect_init(GL_STATE_T *p_state, int width, int height, float red, float green, float blue)
{
	
	p_state->user_data = malloc(sizeof(RectInstanceData));
	RectInstanceData *userData = p_state->user_data;
	GLShapeInstanceData *shapeData = &userData->shape;
	
	Init_rect_gl_state(p_state);
	
	shapeData->objectWidth = width;
	shapeData->objectHeight = height;
	
	shapeData->objectX = 0.0f;
	shapeData->objectY = 0.0f;
	
	glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
	return GL_TRUE;
	
	userData->red = red; userData->green = green; userData->blue = blue;
}

///
// Draw triangles using the shader pair created in Init()
//
void gl_rect_draw(GL_STATE_T *p_state)
{
	RectInstanceData *userData = p_state->user_data;
	GLShapeInstanceData *shapeData = &userData->shape;
	GLRectDisplayData *displayData = p_state->rectDisplayData;
	
	mat4x4 projection;
	mat4x4 modelView;
	mat4x4 projection_scaled;
	mat4x4 translation;
	mat4x4 projection_final;
	
	GLfloat vVertices[] = { 0.0f, 0.0f, 0.0f,  // Position 0
		0.0f,  1.0f, 0.0f,  // Position 1
		1.0f,  1.0f, 0.0f,  // Position 2
		1.0f,  0.0f, 0.0f,  // Position 3
	};
	
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
	//GLushort indices[] = {1, 0, 3, 0, 2, 0, 1 };
	
	// Use the program object
	glUseProgram ( displayData->programObject );
	
	// Load the vertex position
	glVertexAttribPointer ( displayData->positionLoc, 3, GL_FLOAT,
			       GL_FALSE, 3 * sizeof(GLfloat), vVertices );
	
	glEnableVertexAttribArray ( displayData->positionLoc );

	mat4x4_identity(projection);
	mat4x4_scale_aniso(projection_scaled, projection,
			   2.0/p_state->width,
			   -2.0/p_state->height,
			   1.0);
	mat4x4_translate(translation, -1, 1, 0);
	mat4x4_mul(projection_final, translation, projection_scaled);
	
	mat4x4_translate(translation, shapeData->objectX, shapeData->objectY, 0.0);
	mat4x4_identity(projection);
	mat4x4_scale_aniso(projection_scaled, projection,
			   shapeData->objectWidth,
			   shapeData->objectHeight,
			   1.0);
	mat4x4_mul(modelView, translation, projection_scaled);
	
	glUniformMatrix4fv ( displayData->projectionLoc, 1, GL_FALSE, (GLfloat *)projection_final);
	glUniformMatrix4fv ( displayData->modelViewLoc,  1, GL_FALSE, (GLfloat *)modelView);
	
	glUniform3f ( displayData->colorLoc, userData->red, userData->green, userData->blue );
	
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
	//glDrawElements ( GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, indices );
}
