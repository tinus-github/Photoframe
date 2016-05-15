//
//  gl-image.c
//  Photoframe
//
//  Created by Martijn Vernooij on 12/04/16.
//
//

#include "gl-image.h"
#include "gl-display.h"
#include "gl-texture.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

#include "../lib/linmath/linmath.h"

///
// Create a simple width x height texture image with four different colors
//
static GLuint CreateSimpleTexture2D(int width, int height, unsigned char *image )
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
		      0, GL_RGBA, GL_UNSIGNED_BYTE, image );
	
	// Set the filtering mode
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	return textureId;
}

static unsigned int orientationFlipsWidthHeight(unsigned int rotation)
{
	switch (rotation) {
		case 5:
		case 6:
		case 7:
		case 8:
			return TRUE;
		default:
			return FALSE;
	}
}

static void TexCoordsForRotation(unsigned int rotation, GLfloat *coords)
{
	GLfloat coordSets[8][8] = {
		{0.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f,  1.0f, 0.0f}, //1: Normal
		{1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f}, //2: Flipped horizontally
		{1.0f, 1.0f,  1.0f, 0.0f,  0.0f, 0.0f,  0.0f, 1.0f}, //3: Upside down
		{0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f}, //4: Flipped vertically
		{1.0f, 1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f, 0.0f}, //5: Rotated left, then flipped vertically
		{0.0f, 1.0f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f, 0.0f}, //6: Rotated right
		{0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f}, //7: Rotated right, then flipped vertically
		{1.0f, 0.0f,  0.0f, 0.0f,  0.0f, 1.0f,  1.0f, 1.0f} }; //8: Rotated left
	
	if ((rotation < 1) || (rotation > 8)) {
		rotation = 1;
	}
	
	memcpy (coords, coordSets[rotation - 1], sizeof(GLfloat) * 8);
}

static int Init_image_gl_state(GL_STATE_T *p_state) {
	p_state->imageDisplayData = malloc(sizeof(GLImageDisplayData));
	GLImageDisplayData *displayData = p_state->imageDisplayData;
	
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
	displayData->programObject = gl_display_load_program ( vShaderStr, fShaderStr );
	
	// Get the attribute locations
	displayData->positionLoc = glGetAttribLocation ( displayData->programObject, "a_position" );
	displayData->texCoordLoc = glGetAttribLocation ( displayData->programObject, "a_texCoord" );
	
	// Get the uniform locations
	displayData->projectionLoc = glGetUniformLocation ( displayData->programObject, "u_projection" );
	displayData->modelViewLoc  = glGetUniformLocation ( displayData->programObject, "u_modelView" );
	
	// Get the sampler location
	displayData->samplerLoc = glGetUniformLocation ( displayData->programObject, "s_texture" );
	
	return GL_TRUE;
}

///
// Initialize the shader and program object
//
int gl_image_init(GL_STATE_T *p_state, unsigned char* image, int width, int height, unsigned int orientation)
{
	
	p_state->user_data = malloc(sizeof(ImageInstanceData));
	ImageInstanceData *userData = p_state->user_data;
	GLShapeInstanceData *shapeData = &userData->shape;
	
	Init_image_gl_state(p_state);
	userData->textureObj = gl_texture_new();
	
	shapeData->objectWidth = width;
	shapeData->objectHeight = height;
	shapeData->orientation = orientation;
	
	if (orientationFlipsWidthHeight(orientation)) {
		userData->textureWidth = height;
		userData->textureHeight = width;
	} else {
		userData->textureWidth = width;
		userData->textureHeight = height;
	}
	
	// Load the texture
	userData->textureObj->f->load_image(userData->textureObj,
					    image,
					    userData->textureWidth
					    userData->textureHeight);
	userData->textureId = userData->textureObj->data.textureId;
	
	shapeData->objectX = 0.0f;
	shapeData->objectY = 0.0f;
	
	return GL_TRUE;
}

///
// Draw triangles using the shader pair created in Init()
//
void gl_image_draw(GL_STATE_T *p_state)
{
	ImageInstanceData *userData = p_state->user_data;
	GLShapeInstanceData *shapeData = &userData->shape;
	GLImageDisplayData *displayData = p_state->imageDisplayData;
	
	mat4x4 projection;
	mat4x4 modelView;
	mat4x4 projection_scaled;
	mat4x4 translation;
	mat4x4 projection_final;
	
	GLfloat vVertices[] = { 0.0f, 0.0f, 0.0f,  // Position 0
		0.0f,  0.0f,        // TexCoord 0
		0.0f,  1.0f, 0.0f,  // Position 1
		0.0f,  1.0f,        // TexCoord 1
		1.0f,  1.0f, 0.0f,  // Position 2
		1.0f,  1.0f,        // TexCoord 2
		1.0f,  0.0f, 0.0f,  // Position 3
		1.0f,  0.0f         // TexCoord 3
	};
	
	GLfloat texCoords[8];
	TexCoordsForRotation(shapeData->orientation, texCoords);
	vVertices[3] = texCoords[0];
	vVertices[4] = texCoords[1];
	vVertices[8] = texCoords[2];
	vVertices[9] = texCoords[3];
	vVertices[13] = texCoords[4];
	vVertices[14] = texCoords[5];
	vVertices[18] = texCoords[6];
	vVertices[19] = texCoords[7];
	
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
	//GLushort indices[] = {1, 0, 3, 0, 2, 0, 1 };
	
	// Use the program object
	glUseProgram ( displayData->programObject );
	
	// Load the vertex position
	glVertexAttribPointer ( displayData->positionLoc, 3, GL_FLOAT,
			       GL_FALSE, 5 * sizeof(GLfloat), vVertices );
	// Load the texture coordinate
	glVertexAttribPointer ( displayData->texCoordLoc, 2, GL_FLOAT,
			       GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] );
	
	glEnableVertexAttribArray ( displayData->positionLoc );
	glEnableVertexAttribArray ( displayData->texCoordLoc );
	
	// Bind the texture
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, userData->textureId );
	
	// Set the sampler texture unit to 0
	glUniform1i ( displayData->samplerLoc, 0 );
	
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
	
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
	//glDrawElements ( GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, indices );
}
