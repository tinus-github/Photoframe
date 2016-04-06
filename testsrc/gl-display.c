//
//  gl-display.c
//  Photoframe
//
//  Created by Martijn Vernooij on 29/03/16.
//
// This is actually a GL/bcm (for the Raspberry Pi) hybrid

#include "gl-display.h"

#include <bcm_host.h>

#include <assert.h>
#include <sys/time.h>

#include "../lib/linmath/linmath.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

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
	
	
	glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB,
		      width, height,
		      0, GL_RGB, GL_UNSIGNED_BYTE, image );
	
	// Set the filtering mode
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	return textureId;
}

///
// Create a shader object, load the shader source, and
// compile the shader.
//
static GLuint LoadShader(GLenum type, const GLchar *shaderSrc)
{
	GLuint shader;
	GLint compiled;
	// Create the shader object
	shader = glCreateShader(type);
	if(shader == 0)
		return 0;
	// Load the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);
	// Compile the shader
	glCompileShader(shader);
	// Check the compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if(infoLen > 1)
		{
			char* infoLog = malloc(sizeof(char) * infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			fprintf(stderr, "Error compiling shader:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static GLuint LoadProgram ( const GLchar *vertShaderSrc, const GLchar *fragShaderSrc )
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;
	
	// Load the vertex/fragment shaders
	vertexShader = LoadShader ( GL_VERTEX_SHADER, vertShaderSrc );
	if ( vertexShader == 0 )
		return 0;
	
	fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fragShaderSrc );
	if ( fragmentShader == 0 )
	{
		glDeleteShader( vertexShader );
		return 0;
	}
	
	// Create the program object
	programObject = glCreateProgram ( );
	
	if ( programObject == 0 )
		return 0;
	
	glAttachShader ( programObject, vertexShader );
	glAttachShader ( programObject, fragmentShader );
	
	// Link the program
	glLinkProgram ( programObject );
	
	// Check the link status
	glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );
	
	if ( !linked )
	{
		GLint infoLen = 0;
		glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );
		
		if ( infoLen > 1 )
		{
			char* infoLog = malloc (sizeof(char) * infoLen );
			
			glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
			fprintf (stderr, "Error linking program:\n%s\n", infoLog );
			
			free ( infoLog );
		}
		
		glDeleteProgram ( programObject );
		return 0;
	}
	
	// Free up no longer needed shader resources
	glDeleteShader ( vertexShader );
	glDeleteShader ( fragmentShader );
	
	return programObject;
}

static unsigned int orientationFlipsWidthHeight(unsigned int rotation);

///
// Initialize the shader and program object
//
int Init(GL_STATE_T *p_state, unsigned char* image, int width, int height, unsigned int orientation)
{
	
	p_state->user_data = malloc(sizeof(ImageUserData));
	ImageUserData *userData = p_state->user_data;
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
	userData->programObject = LoadProgram ( vShaderStr, fShaderStr );
	
	// Get the attribute locations
	userData->positionLoc = glGetAttribLocation ( userData->programObject, "a_position" );
	userData->texCoordLoc = glGetAttribLocation ( userData->programObject, "a_texCoord" );
	
	// Get the uniform locations
	userData->projectionLoc = glGetUniformLocation ( userData->programObject, "u_projection" );
	userData->modelViewLoc  = glGetUniformLocation ( userData->programObject, "u_modelView" );
	
	// Get the sampler location
	userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );
	// Load the texture
	userData->textureId = CreateSimpleTexture2D (width, height, image);
	
	userData->textureWidth = width;
	userData->textureHeight = height;
	userData->orientation = orientation;
	
	if (orientationFlipsWidthHeight(orientation)) {
		userData->objectWidth = height;
		userData->objectHeight = width;
	} else {
		userData->objectWidth = width;
		userData->objectHeight = height;
	}
	userData->objectX = 0.0f;
	userData->objectY = 0.0f;
	
	glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
	return GL_TRUE;
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

///
// Draw triangles using the shader pair created in Init()
//
void Draw(GL_STATE_T *p_state)
{
	ImageUserData *userData = p_state->user_data;
	
	mat4x4 projection;
	mat4x4 modelView;
	mat4x4 projection_scaled;
	mat4x4 translation;
	mat4x4 projection_final;
	
	GLfloat leftc = userData->objectX;
	GLfloat topc = userData->objectY;
	GLfloat rightc = userData->objectWidth;
	GLfloat bottomc = userData->objectHeight;
	
	GLfloat vVertices[] = { leftc,  topc, 0.0f,  // Position 0
		0.0f,  0.0f,        // TexCoord 0
		leftc, bottomc, 0.0f,  // Position 1
		0.0f,  1.0f,        // TexCoord 1
		rightc, bottomc, 0.0f,  // Position 2
		1.0f,  1.0f,        // TexCoord 2
		rightc,  topc, 0.0f,  // Position 3
		1.0f,  0.0f         // TexCoord 3
	};
	
	GLfloat texCoords[8];
	TexCoordsForRotation(userData->orientation, texCoords);
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
	
	// Set the viewport
	glViewport ( 0, 0, p_state->width, p_state->height );
	
	// Clear the color buffer
	glClear ( GL_COLOR_BUFFER_BIT );
	
	// Use the program object
	glUseProgram ( userData->programObject );
	
	// Load the vertex position
	glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT,
			       GL_FALSE, 5 * sizeof(GLfloat), vVertices );
	// Load the texture coordinate
	glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
			       GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] );
	
	glEnableVertexAttribArray ( userData->positionLoc );
	glEnableVertexAttribArray ( userData->texCoordLoc );
	
	// Bind the texture
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, userData->textureId );
	
	// Set the sampler texture unit to 0
	glUniform1i ( userData->samplerLoc, 0 );
	
	mat4x4_identity(projection);
	mat4x4_scale_aniso(projection_scaled, projection,
			   2.0/p_state->width,
			   -2.0/p_state->height,
			   1.0);
	mat4x4_translate(translation, -0.5 * p_state->width, -0.5 * p_state->height, 0);
	mat4x4_mul(projection_final, projection_scaled, translation);
	mat4x4_identity(modelView);
	
	glUniformMatrix4fv ( userData->projectionLoc, 1, GL_FALSE, (GLfloat *)projection_final);
	glUniformMatrix4fv ( userData->modelViewLoc,  1, GL_FALSE, (GLfloat *)modelView);
	
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
	//glDrawElements ( GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, indices );
}

void init_ogl(GL_STATE_T *state)
{
	int32_t success = 0;
	EGLBoolean result;
	EGLint num_config;
	
	bcm_host_init();
	
	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	
	static const EGLint attribute_list[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
	
	static const EGLint context_attributes[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	
	EGLConfig config;
	
	// get an EGL display connection
	state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	
	// initialize the EGL display connection
	result = eglInitialize(state->display, NULL, NULL);
	
	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(state->display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);
	
	// get an appropriate EGL frame buffer configuration
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);
	
	
	// create an EGL rendering context
	state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
	assert(state->context!=EGL_NO_CONTEXT);
	
	// create an EGL window surface
	success = graphics_get_display_size(0 /* LCD */, &state->width, &state->height);
	assert( success >= 0 );
	

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = state->width;
	dst_rect.height = state->height;
	
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = state->width << 16;
	src_rect.height = state->height << 16;
	
	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );
	
	dispman_element =
	vc_dispmanx_element_add(dispman_update, dispman_display,
				0/*layer*/, &dst_rect, 0/*src*/,
				&src_rect, DISPMANX_PROTECTION_NONE,
				0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
	
	state->nativewindow.element = dispman_element;
	state->nativewindow.width = state->width;
	state->nativewindow.height = state->height;
	vc_dispmanx_update_submit_sync( dispman_update );
	
	state->surface = eglCreateWindowSurface( state->display, config, &(state->nativewindow), NULL );
	assert(state->surface != EGL_NO_SURFACE);
	
	// connect the context to the surface
	result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
	assert(EGL_FALSE != result);
}

void  esMainLoop (GL_STATE_T *esContext )
{
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	float totaltime = 0.0f;
	unsigned int frames = 0;
	
	gettimeofday ( &t1 , &tz );
	
	while(1)
	{
		gettimeofday(&t2, &tz);
		deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
		t1 = t2;
		
		if (esContext->draw_func != NULL)
			esContext->draw_func(esContext);
		
		eglSwapBuffers(esContext->display, esContext->surface);
		
		totaltime += deltatime;
		frames++;
		if (totaltime >  2.0f)
		{
			printf("%4d frames rendered in %1.4f seconds -> FPS=%3.4f\n", frames, totaltime, frames/totaltime);
			totaltime -= 2.0f;
			frames = 0;
		}
	}
}

void esInitContext ( GL_STATE_T *p_state )
{
	if ( p_state != NULL )
	{
		memset( p_state, 0, sizeof( GL_STATE_T) );
	}
}

void esRegisterDrawFunc(GL_STATE_T *p_state, void (*draw_func) (GL_STATE_T* ) )
{
	p_state->draw_func = draw_func;
}
