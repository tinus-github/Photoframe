//
//  gl-texture.c
//  Photoframe
//
//  Created by Martijn Vernooij on 13/05/16.
//
//

#include "gl-texture.h"
#include "gl-renderloop-member.h"
#include "gl-renderloop.h"
#include "egl-driver.h"

#include <string.h>
#include <assert.h>

static void load_image(gl_texture *obj, gl_bitmap *bitmap, unsigned int width, unsigned int height);
static void load_image_r(gl_texture *obj, gl_bitmap *bitmap, unsigned char *rgba_data, unsigned int width, unsigned int height);
static void load_image_monochrome(gl_texture *obj, gl_bitmap *bitmap, unsigned int width, unsigned int height);
static void load_image_horizontal_tile(gl_texture *obj, gl_bitmap *bitmap,
				       unsigned int image_width, unsigned int image_height,
				       unsigned int tile_height, unsigned int tile_y);
static void gl_texture_flip_alpha(gl_texture *obj);
static void gl_texture_cancel_loading(gl_texture *obj);
static void gl_texture_free(gl_object *obj);

static struct gl_texture_funcs gl_texture_funcs_global = {
	.load_image = &load_image,
	.load_image_r = &load_image_r,
	.load_image_monochrome = &load_image_monochrome,
	.load_image_horizontal_tile = &load_image_horizontal_tile,
	.cancel_loading = &gl_texture_cancel_loading,
	.flip_alpha = &gl_texture_flip_alpha
};

typedef struct gl_texture_program_data {
	GLuint program;
	GLint positionLoc;
	GLint texCoordLoc;
	GLint samplerLoc;
	
	GLint widthLoc;
	GLint heightLoc;
} gl_texture_program_data;

static gl_texture_program_data gl_flip_program;
static gl_texture_program_data gl_blur_h_program;
static gl_texture_program_data gl_blur_v_program;

typedef enum {
	gl_texture_program_flip,
	gl_texture_program_blur_h,
	gl_texture_program_blur_v
} gl_texture_manipulation_program;

static uint gl_texture_program_loaded = 0;

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_texture_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_texture_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
	
	gl_object_free_org_global = gl_texture_funcs_global.p.free;
	gl_texture_funcs_global.p.free = &gl_texture_free;
}

gl_texture *gl_texture_init(gl_texture *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_texture_funcs_global;
	
	obj->data.loadedNotice = gl_notice_new();
	
	return obj;
}

gl_texture *gl_texture_new()
{
	gl_texture *ret = calloc(1, sizeof(gl_texture));
	
	return gl_texture_init(ret);
}

static void gl_texture_load_program_attribute_locations(gl_texture_program_data *data)
{
	GLuint program = data->program;
	// Get the attribute locations
	data->positionLoc = glGetAttribLocation ( program, "a_position" );
	data->texCoordLoc = glGetAttribLocation ( program, "a_texCoord" );
	
	// Get the sampler location
	data->samplerLoc = glGetUniformLocation ( program, "s_texture" );
	
	// Get the uniform locations (if present)
	data->widthLoc = glGetUniformLocation ( program, "u_width" );
	data->heightLoc = glGetUniformLocation ( program, "u_height" );
}

static int gl_texture_load_program() {
	// The simplest do nothing vertex shader possible
	GLchar vShaderStr[] =
	"attribute vec4 a_position;            \n"
	"attribute vec2 a_texCoord;            \n"
	"varying vec2 v_texCoord;              \n"
	"void main()                           \n"
	"{                                     \n"
	"   gl_Position = a_position;          \n"
	"   v_texCoord = a_texCoord;           \n"
	"}                                     \n";
	
	GLchar fShaderFlipAlphaStr[] =
	"precision mediump float;                            \n"
	"varying vec2 v_texCoord;                            \n"
	"uniform sampler2D s_texture;                        \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  vec4 texelColor = texture2D( s_texture, v_texCoord );\n"
	"  float luminance = 1.0 - texelColor.a;             \n"
	"  gl_FragColor = vec4(1.0, 1.0, 1.0, luminance);    \n"
	"}                                                   \n";
	
	GLchar fShaderAlphaBlurHStr[] =
	"precision mediump float;                            \n"
	"varying vec2 v_texCoord;                            \n"
	"uniform sampler2D s_texture;                        \n"
	"uniform float u_width;                              \n"
	"uniform float u_height;                             \n"
// No static arrays in openGL ES
	"float weight0 = 0.3225806452;                       \n"
	"float weight1 = 0.2419354838;                       \n"
	"float weight2 = 0.0967741935;                       \n"

	"void main()                                         \n"
	"{                                                   \n"
	"  vec4 texelColor = texture2D( s_texture, v_texCoord );\n"
	"  float luminance = texelColor.a * weight0;         \n"
	
	"   texelColor = texture2D( s_texture, vec2(v_texCoord) + vec2(1.0 / u_width, 0.0));\n"
	"   luminance += texelColor.a * weight1;             \n"
	"   texelColor = texture2D( s_texture, vec2(v_texCoord) + vec2(-1.0 / u_width, 0.0));\n"
	"   luminance += texelColor.a * weight1;             \n"
	
	"   texelColor = texture2D( s_texture, vec2(v_texCoord) + vec2(2.0 / u_width, 0.0));\n"
	"   luminance += texelColor.a * weight2;             \n"
	"   texelColor = texture2D( s_texture, vec2(v_texCoord) + vec2(-2.0 / u_width, 0.0));\n"
	"   luminance += texelColor.a * weight2;             \n"
	
	"   gl_FragColor = vec4(0.0, 0.0, 0.0, luminance);   \n"
	"}                                                   \n";

	GLchar fShaderAlphaBlurVStr[] =
	"precision mediump float;                            \n"
	"varying vec2 v_texCoord;                            \n"
	"uniform sampler2D s_texture;                        \n"
	"uniform float u_width;                              \n"
	"uniform float u_height;                             \n"
	// No static arrays in openGL ES
	"float weight0 = 0.3225806452;                       \n"
	"float weight1 = 0.2419354838;                       \n"
	"float weight2 = 0.0967741935;                       \n"
	
	"void main()                                         \n"
	"{                                                   \n"
	"  vec4 texelColor = texture2D( s_texture, v_texCoord );\n"
	"  float luminance = texelColor.a * weight0;         \n"
	
	"   texelColor = texture2D( s_texture, vec2(v_texCoord) + vec2(0.0, 1.0 / u_height));\n"
	"   luminance += texelColor.a * weight1;             \n"
	"   texelColor = texture2D( s_texture, vec2(v_texCoord) + vec2(0.0, -1.0 / u_height));\n"
	"   luminance += texelColor.a * weight1;             \n"
	
	"   texelColor = texture2D( s_texture, vec2(v_texCoord) + vec2(0.0, 2.0 / u_height));\n"
	"   luminance += texelColor.a * weight2;             \n"
	"   texelColor = texture2D( s_texture, vec2(v_texCoord) + vec2(0.0, -2.0 / u_height));\n"
	"   luminance += texelColor.a * weight2;             \n"
	
	"   gl_FragColor = vec4(0.0, 0.0, 0.0, luminance);   \n"
	"}                                                   \n";

	
	// Load the shaders and get a linked program object
	// Flip (testing)
	gl_flip_program.program = egl_driver_load_program ( vShaderStr, fShaderFlipAlphaStr );
	gl_texture_load_program_attribute_locations(&gl_flip_program);

	// Blur horizontally
	gl_blur_h_program.program = egl_driver_load_program ( vShaderStr, fShaderAlphaBlurHStr );
	gl_texture_load_program_attribute_locations(&gl_blur_h_program);
	
	// Blur vertically
	gl_blur_v_program.program = egl_driver_load_program ( vShaderStr, fShaderAlphaBlurVStr );
	gl_texture_load_program_attribute_locations(&gl_blur_v_program);
	
	gl_texture_program_loaded = 1;
	
	return GL_TRUE;
}


static void load_image_gen_r_work(void *target, gl_renderloop_member *renderloop_member, void *extra_data)
{
	gl_texture *obj = (gl_texture *)target;
	gl_object *bitmap_obj;
	
	// Texture object handle
	GLuint textureId;
	
	// Use tightly packed data
	glPixelStorei ( GL_UNPACK_ALIGNMENT, 1 );
	
	// Generate a texture object
	glGenTextures ( 1, &textureId );
	
	// Bind the texture object
	glBindTexture ( GL_TEXTURE_2D, textureId );
	
	// Load the texture
	if (obj->data.dataType != gl_texture_data_type_rgba) {
		glTexImage2D ( GL_TEXTURE_2D, 0, GL_ALPHA,
			      obj->data.width, obj->data.height,
			      0, GL_ALPHA, GL_UNSIGNED_BYTE, obj->data.imageDataBitmap );
	} else {
		glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA,
			      obj->data.width, obj->data.height,
			      0, GL_RGBA, GL_UNSIGNED_BYTE, obj->data.imageDataBitmap );
	}
	
	// Set the filtering mode
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	
	// Wrapping mode
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	obj->data.textureId = textureId;
	obj->data.loadState = gl_texture_loadstate_done;
	bitmap_obj = (gl_object *)obj->data.imageDataStore;
	bitmap_obj->f->unref(bitmap_obj);
	obj->data.imageDataStore = NULL;
	obj->data.imageDataBitmap = NULL;
	((gl_object *)obj->data.uploadRenderloopMember)->f->unref((gl_object *)obj->data.uploadRenderloopMember);
	obj->data.uploadRenderloopMember = NULL;
	
	obj->data.loadedNotice->f->fire(obj->data.loadedNotice);
}

static void load_image_gen_r(gl_texture *obj, gl_bitmap *bitmap, unsigned char *data)
{
	obj->data.loadState = gl_texture_loadstate_started;
	obj->data.imageDataStore = bitmap;
	gl_object *bitmap_obj = (gl_object *)bitmap;
	bitmap_obj->f->ref(bitmap_obj);
	obj->data.imageDataBitmap = data;

	gl_renderloop_member *renderloop_member = gl_renderloop_member_new();
	renderloop_member->data.target = obj;
	renderloop_member->data.action = &load_image_gen_r_work;
	renderloop_member->data.loadPriority = obj->data.loadPriority;
	
	gl_renderloop *renderloop = gl_renderloop_get_global_renderloop();
	renderloop->f->append_child(renderloop, gl_renderloop_phase_load, renderloop_member);
	obj->data.uploadRenderloopMember = renderloop_member;
	((gl_object *)renderloop_member)->f->ref((gl_object *)renderloop_member);
}

static void load_image_gen(gl_texture *obj, gl_bitmap *bitmap)
{
	load_image_gen_r(obj, bitmap, bitmap->data.bitmap);
}

static void load_image(gl_texture *obj, gl_bitmap *bitmap, unsigned int width, unsigned int height)
{
	obj->data.dataType = gl_texture_data_type_rgba;
	obj->data.width = width;
	obj->data.height = height;
	load_image_gen(obj, bitmap);
}

static void load_image_r(gl_texture *obj, gl_bitmap *bitmap, unsigned char *rgba_data, unsigned int width, unsigned int height)
{
	obj->data.dataType = gl_texture_data_type_rgba;
	obj->data.width = width;
	obj->data.height = height;
	load_image_gen_r(obj, bitmap, rgba_data);
}

static void load_image_monochrome(gl_texture *obj, gl_bitmap *bitmap, unsigned int width, unsigned int height)
{
	obj->data.dataType = gl_texture_data_type_monochrome;
	obj->data.width = width;
	obj->data.height = height;
	load_image_gen(obj, bitmap);
}

static void load_image_horizontal_tile(gl_texture *obj, gl_bitmap *bitmap,
					 unsigned int image_width, unsigned int image_height,
					 unsigned int tile_height, unsigned int tile_y)
{
	assert (tile_y < image_height);
	
	unsigned char *tile_data = (bitmap->data.bitmap) + (4 * sizeof(unsigned char) * image_width * tile_y);
	
	obj->f->load_image_r(obj, bitmap, tile_data, image_width, tile_height);
}

static void gl_texture_apply_shader(gl_texture *obj, gl_texture_manipulation_program programNumber)
{
	if (!gl_texture_program_loaded) {
		gl_texture_load_program();
	}
	
	assert (obj->data.loadState == gl_texture_loadstate_done);
	
	switch (programNumber) {
		case gl_texture_program_flip:
			gl_texture_program_data *program = &gl_flip_program;
			break;
		case gl_texture_program_blur_h:
			gl_texture_program_data *program = &gl_blur_h_program;
			break;
		case gl_texture_program_blur_v:
			gl_texture_program_data *program = &gl_blur_h_program;
			break;
	}
	
	GLfloat vVertices[] = { -1.0f, -1.0f, 0.0f,  // Position 0
		0.0f,  0.0f,        // TexCoord 0
		-1.0f,  1.0f, 0.0f,  // Position 1
		0.0f,  1.0f,        // TexCoord 1
		1.0f,  1.0f, 0.0f,  // Position 2
		1.0f,  1.0f,        // TexCoord 2
		1.0f,  -1.0f, 0.0f,  // Position 3
		1.0f,  0.0f         // TexCoord 3
	};
	
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
	
	
	glActiveTexture ( GL_TEXTURE0 );
	
	// Create output texture
	GLuint newTexture;
	glGenTextures(1, &newTexture);
	glBindTexture ( GL_TEXTURE_2D, newTexture );
	glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA,
		      obj->data.width, obj->data.height,
		      0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	
	// Set the filtering mode
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	
	// Create and setup FBO
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			       GL_TEXTURE_2D, newTexture, 0);
	
	// Setup rendering
	glViewport(0,0, obj->data.width, obj->data.height);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glBindTexture ( GL_TEXTURE_2D, obj->data.textureId );
	
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
	
	switch (programNumber) {
		case gl_texture_program_blur_h:
		case gl_texture_program_blur_v:
			glUniform1f(program->widthLoc, obj->data.width);
			glUniform1f(program->heightLoc, obj->data.height);
			break;
		default:
			break;
	}
	
	// Draw
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
	
	// Swap out the old texture for the newly created one
	glDeleteTextures(1, &obj->data.textureId);
	obj->data.textureId = newTexture;
	obj->data.dataType = gl_texture_data_type_rgba;
	
	// Make sure to unbind the fbo
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void gl_texture_flip_alpha(gl_texture *obj)
{
	gl_texture_apply_program(obj, gl_texture_program_flip);
}


static void gl_texture_free_texture(gl_texture *obj)
{
	glDeleteTextures ( 1, &obj->data.textureId );
	obj->data.loadState = gl_texture_loadstate_clear;
}

static void gl_texture_cancel_loading(gl_texture *obj)
{
	assert (obj->data.loadState == gl_texture_loadstate_started);
	gl_renderloop_member *member = obj->data.uploadRenderloopMember;
	gl_renderloop *renderloop = member->data.owner;
	renderloop->f->remove_child(renderloop, member);
	
	((gl_object *)member)->f->unref((gl_object *)member);
	obj->data.uploadRenderloopMember = NULL;
	
	gl_object *bitmap_obj = (gl_object *)obj->data.imageDataStore;
	bitmap_obj->f->unref(bitmap_obj);
	obj->data.imageDataStore = NULL;
	obj->data.imageDataBitmap = NULL;
	
	obj->data.loadState = gl_texture_loadstate_clear;
}

static void gl_texture_free(gl_object *obj_obj)
{
	gl_texture *obj = (gl_texture *)obj_obj;
	switch (obj->data.loadState) {
		case gl_texture_loadstate_clear:
			break;
		case gl_texture_loadstate_done:
			gl_texture_free_texture(obj);
			break;
		case gl_texture_loadstate_started:
			obj->f->cancel_loading(obj);
			break;
	}
	((gl_object *)obj->data.loadedNotice)->f->unref((gl_object *)obj->data.loadedNotice);
	
	gl_object_free_org_global(obj_obj);
}
