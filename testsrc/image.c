
/*
 * code stolen from openGL-RPi-tutorial-master/encode_OGL/
 * and from OpenGL® ES 2.0 Programming Guide
 */

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
//#include "jpeg.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <jpeglib.h>

// from esUtil.h
#define TRUE 1
#define FALSE 0

typedef struct
{
	// Handle to a program object
	GLuint programObject;
	
	// Attribute locations
	GLint  positionLoc;
	GLint  texCoordLoc;
	
	// Sampler location
	GLint samplerLoc;
	
	// Texture handle
	GLuint textureId;
	
} UserData;

typedef struct CUBE_STATE_T
{
	uint32_t width;
	uint32_t height;
	
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	
	EGL_DISPMANX_WINDOW_T nativewindow;
	UserData *user_data;
	void (*draw_func) (struct CUBE_STATE_T* );
} CUBE_STATE_T;

char *image;
int tex;

typedef struct upscalestruct {
	unsigned int scalerest;
	float scalefactor;
	unsigned int current_y;
	unsigned int total_x;
	unsigned int total_y;
	void *outputbuf;
	unsigned int *y_contributions;
} upscalestruct;

void *setup_upscale();
void upscaleLine(char *inputbuf, unsigned int inputwidth, unsigned int inputheight,
		 char *outputbuf, unsigned int outputwidth, unsigned int outputheight,
		 unsigned int current_line_inputbuf, struct upscalestruct *data);
void done_upscale(struct upscalestruct *data);


char *esLoadJPEG ( char *fileName, int wantedwidth, int wantedheight,
		  int *width, int *height )
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	
	FILE *f;
	
	char *buffer = NULL;
	
	JSAMPROW *scanbuf = NULL;
	JSAMPROW *scanbufcurrentline;
	unsigned int scanbufheight;
	unsigned int lines_in_scanbuf = 0;
	unsigned int lines_in_buf = 0;
	
	JSAMPROW *row_pointers;
	
	unsigned int counter;
	
	float scalefactor, scalefactortmp;
	void *scaledata;
	
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	
	f = fopen(fileName, "rb");
	if (f == NULL) return NULL;
	
	jpeg_stdio_src(&cinfo, f);
	jpeg_read_header(&cinfo, TRUE);
	
	cinfo.out_color_space = JCS_RGB;
	jpeg_start_decompress(&cinfo);
	
	scalefactor = (float)wantedwidth / cinfo.output_width;
	scalefactortmp = (float)wantedheight / cinfo.output_height;
	
	if (scalefactortmp < scalefactor) {
		scalefactor = scalefactortmp;
	}
	
	if (scalefactor > 1) {
		scaledata = setup_upscale();
	}
	
	*width = cinfo.output_width * scalefactor;
	*height = cinfo.output_height * scalefactor;
	
	buffer = malloc(width[0] * height[0] * 3);
	
	scanbufheight = cinfo.rec_outbuf_height;
	scanbuf = malloc(cinfo.output_width * 3 * scanbufheight);
	row_pointers = malloc(scanbufheight * sizeof(JSAMPROW));
	for (counter = 0; counter < scanbufheight; counter++) {
		row_pointers[counter] = scanbuf + 3 * (lines_in_buf * cinfo.output_width);
	}
	
	while (lines_in_buf < cinfo.output_height) {
		if (!lines_in_scanbuf) {
			lines_in_scanbuf = jpeg_read_scanlines(
							       &cinfo, row_pointers, scanbufheight);
			scanbufcurrentline = scanbuf;
		}
		if (scalefactor != 1.0f) {
			upscaleLine(scanbufcurrentline, cinfo.output_width, cinfo.output_height,
				    buffer, *width, *height, lines_in_buf, scaledata);
		} else {
			memcpy (buffer + 3 * (lines_in_buf * cinfo.output_width),
				scanbufcurrentline,
				3 * cinfo.output_width);
		}
		
		scanbufcurrentline += 3 * cinfo.output_width;
		lines_in_scanbuf--;
		lines_in_buf++;
	}
	
	if (scalefactor > 1.0f) {
		done_upscale(scaledata);
	}
	
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(scanbuf);
	fclose(f);
	
	return buffer;
}


void *setup_upscale()
{
	struct upscalestruct *ret = malloc(sizeof(struct upscalestruct));
	
	ret->scalerest = 0;
	ret->scalefactor = 0.0f;
	ret->current_y = 0;
	ret->y_contributions = NULL;
	return ret;
}

void upscaleLine(char *inputbuf, unsigned int inputwidth, unsigned int inputheight,
		 char *outputbuf, unsigned int outputwidth, unsigned int outputheight,
		 unsigned int current_line_inputbuf, struct upscalestruct *data)
{
	unsigned int current_x_in;
	unsigned int current_x_out;
	unsigned int x_scalerest;
	unsigned int x_total[3];
	unsigned int x_contribution;
	unsigned int x_possible_contribution;
	unsigned int x_remaining_contribution;
	unsigned int y_contribution;
	unsigned int y_possible_contribution;
	unsigned int y_remaining_contribution;
	
	int counter;

	char *outputptr;
	char *inputptr;
	
	if (data->scalefactor == 0.0f) {
		data->scalefactor = (float)outputwidth / inputwidth;
		data->total_x = outputwidth;
		data->total_y = outputheight;
		data->outputbuf = outputbuf;
		data->y_contributions = calloc(3 * sizeof(unsigned int), outputwidth);
	}
	
	y_remaining_contribution = outputheight;
	do {
		y_possible_contribution = inputheight - data->scalerest;
		if (y_possible_contribution <= y_remaining_contribution) {
			y_contribution = y_possible_contribution;
		} else {
			y_contribution = y_remaining_contribution;
		}
		
		x_scalerest = 0;
		outputptr = outputbuf + 3 * outputwidth * data->current_y;
		inputptr = inputbuf;
		current_x_out = 0;
		x_total[0] = x_total[1] = x_total[2] = 0;
		for (current_x_in = 0; current_x_in < inputwidth; current_x_in++) {
			x_remaining_contribution = outputwidth;
			do {
				x_possible_contribution = inputwidth - x_scalerest;
				if (x_possible_contribution <= x_remaining_contribution) {
					x_contribution = x_possible_contribution;
					if (x_contribution == inputwidth) {
						outputptr[0] = inputptr[0];
						outputptr[1] = inputptr[1];
						outputptr[2] = inputptr[2];
					} else {
						x_total[0] += x_contribution * inputptr[0];
						x_total[1] += x_contribution * inputptr[1];
						x_total[2] += x_contribution * inputptr[2];
						outputptr[0] = x_total[0] / inputwidth;
						outputptr[1] = x_total[1] / inputwidth;
						outputptr[2] = x_total[2] / inputwidth;
					}
					outputptr += 3;
					
					current_x_out++;
					x_total[0] = x_total[1] = x_total[2] = 0;
					x_remaining_contribution -= x_contribution;
					x_scalerest = 0;
					continue;
				} else {
					x_contribution = x_remaining_contribution;
					x_total[0] += x_contribution * inputptr[0];
					x_total[1] += x_contribution * inputptr[1];
					x_total[2] += x_contribution * inputptr[2];
					x_scalerest += x_remaining_contribution;
					break;
				}
			} while (1);
			
			inputptr += 3;
		}
		if (current_x_out < outputwidth) {
			bzero(outputptr, 3 * (outputwidth - current_x_out));
		}
		if (y_contribution != y_remaining_contribution) {
			if (y_contribution != inputheight) {
				outputptr = outputbuf + 3 * outputwidth * data->current_y;
				for (counter = (3 * outputwidth) - 1 ; counter >= 0; counter--) {
					data->y_contributions[counter] += y_contribution * outputptr[counter];
					outputptr[counter] = data->y_contributions[counter] / inputheight;
				}
			}
			bzero(data->y_contributions, outputwidth * 3 * sizeof(unsigned int));

			data->current_y++;
			y_remaining_contribution -= y_contribution;
			data->scalerest = 0;
			continue;
		} else {
			outputptr = outputbuf + 3 * outputwidth * data->current_y;
			for (counter = (3 * outputwidth) - 1 ; counter >= 0; counter--) {
				data->y_contributions[counter] += y_contribution * outputptr[counter];
			}
			data->scalerest += y_remaining_contribution;
			break;
		}
		
	} while (1);
	assert (data->current_y < outputheight);
}

void done_upscale(struct upscalestruct *data)
{
	if (data->current_y < data->total_y) {
		bzero(data->outputbuf + 3 * data->total_x * data->current_y,
		      3 * data->total_x * (data->total_y - data->current_y));
	}
	free(data);
}

char* esLoadTGA ( char *fileName, int *width, int *height )
{
	char *buffer = NULL;
	FILE *f;
	unsigned char tgaheader[12];
	unsigned char attributes[6];
	unsigned int imagesize;
	
	f = fopen(fileName, "rb");
	if(f == NULL) return NULL;
	
	if(fread(&tgaheader, sizeof(tgaheader), 1, f) == 0)
	{
		fclose(f);
		return NULL;
	}
	
	if(fread(attributes, sizeof(attributes), 1, f) == 0)
	{
		fclose(f);
		return 0;
	}
	
	*width = attributes[1] * 256 + attributes[0];
	*height = attributes[3] * 256 + attributes[2];
	imagesize = attributes[4] / 8 * *width * *height;
	//imagesize *= 4/3;
	printf("Origin bits: %d\n", attributes[5] & 030);
	printf("Pixel depth %d\n", attributes[4]);
	buffer = malloc(imagesize);
	if (buffer == NULL)
	{
		fclose(f);
		return 0;
	}
	
#if 1
	// invert - should be reflect, easier is 180 rotate
	int n = 1;
	while (n <= imagesize) {
		fread(&buffer[imagesize - n], 1, 1, f);
		n++;
	}
#else
	// as is - upside down
	if(fread(buffer, 1, imagesize, f) != imagesize)
	{
		free(buffer);
		return NULL;
	}
#endif
	fclose(f);
	return buffer;
}

///
// Create a simple width x height texture image with four different colors
//
GLuint CreateSimpleTexture2D(int width, int height )
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
GLuint LoadShader(GLenum type, const char *shaderSrc)
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

GLuint LoadProgram ( const char *vertShaderSrc, const char *fragShaderSrc )
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

///
// Initialize the shader and program object
//
int Init(CUBE_STATE_T *p_state)
{
	
	p_state->user_data = malloc(sizeof(UserData));
	UserData *userData = p_state->user_data;
	GLbyte vShaderStr[] =
	"attribute vec4 a_position;   \n"
	"attribute vec2 a_texCoord;   \n"
	"varying vec2 v_texCoord;     \n"
	"void main()                  \n"
	"{                            \n"
	"   gl_Position = a_position; \n"
	"   v_texCoord = a_texCoord;  \n"
	"}                            \n";
	
	GLbyte fShaderStr[] =
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
	
	// Get the sampler location
	userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );
	// Load the texture
	userData->textureId = CreateSimpleTexture2D (p_state->width, p_state->height);
	
	glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
	return GL_TRUE;
}

///
// Draw triangles using the shader pair created in Init()
//
void Draw(CUBE_STATE_T *p_state)
{
	UserData *userData = p_state->user_data;
	
	GLfloat vVertices[] = { -1.0f,  1.0f, 0.0f,  // Position 0
		0.0f,  0.0f,        // TexCoord 0
		-1.0f, -1.0f, 0.0f,  // Position 1
		0.0f,  1.0f,        // TexCoord 1
		1.0f, -1.0f, 0.0f,  // Position 2
		1.0f,  1.0f,        // TexCoord 2
		1.0f,  1.0f, 0.0f,  // Position 3
		1.0f,  0.0f         // TexCoord 3
	};
	
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
	
	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );
	//glDrawElements ( GL_TRIANGLE_STRIP, 6, GL_UNSIGNED_SHORT, indices );
}

CUBE_STATE_T state, *p_state = &state;

void init_ogl(CUBE_STATE_T *state, int width, int height)
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
	
	state->width = width;
	state->height = height;
	
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

void esInitContext ( CUBE_STATE_T *p_state )
{
	if ( p_state != NULL )
	{
		memset( p_state, 0, sizeof( CUBE_STATE_T) );
	}
}

void esRegisterDrawFunc(CUBE_STATE_T *p_state, void (*draw_func) (CUBE_STATE_T* ) )
{
	p_state->draw_func = draw_func;
}

void  esMainLoop (CUBE_STATE_T *esContext )
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

int main(int argc, char *argv[])
{
	UserData user_data;
	int width, height;
	
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	
	//image = esLoadTGA("jan.tga", &width, &height);
	gettimeofday ( &t1 , &tz );
	
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return -1;
	}
	
	image = esLoadJPEG(argv[1], 1920, 1080, &width, &height);
	if (image == NULL) {
		fprintf(stderr, "No such image\n");
		exit(1);
	}
	fprintf(stderr, "Image is %d x %d\n", width, height);
	gettimeofday(&t2, &tz);
	deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
	
	printf("Image loaded in %1.4f seconds\n", deltatime);
	
	
	bcm_host_init();
	esInitContext(p_state);
	
	init_ogl(p_state, width, height);
	
	p_state->user_data = &user_data;
	p_state->width = width;
	p_state->height = height;
	
	if(!Init(p_state))
		return 0;
	
	esRegisterDrawFunc(p_state, Draw);
	
	eglSwapBuffers(p_state->display, p_state->surface);
	esMainLoop(p_state);
}
