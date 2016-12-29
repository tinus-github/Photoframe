//
//  egl-driver.c
//  Photoframe
//
//  Created by Martijn Vernooij on 29/12/2016.
//
//

#include "egl-driver.h"
#include "gl-texture.h"
#include "gl-shape.h"
#include "gl-tile.h"
#include "gl-stage.h"
#include "gl-container.h"
#include "gl-container-2d.h"
#include "gl-tiled-image.h"
#include "gl-renderloop-member.h"
#include "gl-renderloop.h"

static void egl_display_clear(egl_display_data *data)
{
	gl_stage *stage = gl_stage_get_global_stage();
	
	// Set the viewport
	glViewport(0, 0, stage->data.width, stage->data.height);
	
	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);
}

static void egl_display_clearf(void *target, gl_renderloop_member *renderloop_member, void *action_data)
{
	egl_display_data *data = (egl_display_data *)action_data;
	
	egl_display_draw(data);
}

static void egl_display_swap(egl_display_data *data)
{
	eglSwapBuffers(data->display, data->surface);
}

static void egl_display_swapf(void *target, gl_renderloop_member *renderloop_member, void *action_data)
{
	egl_display_data *data = (egl_display_data *)action_data;
	
	egl_display_swap(data);
}

static void egl_display_init_display(egl_display_data *data)
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
	data->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	
	// initialize the EGL display connection
	result = eglInitialize(data->display, NULL, NULL);
	
	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(data->display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);
	
	// get an appropriate EGL frame buffer configuration
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);
	
	
	// create an EGL rendering context
	data->context = eglCreateContext(data->display, config, EGL_NO_CONTEXT, context_attributes);
	assert(data->context!=EGL_NO_CONTEXT);
	
	// create an EGL window surface
	unsigned int width, height;
	
	success = graphics_get_display_size(0 /* LCD */, &width, &height);
	assert( success >= 0 );
	
	gl_stage *stage = gl_stage_get_global_stage();
	stage->f->set_dimensions(stage, width, height);
	
	EGL_DISPMANX_WINDOW_T nativewindow;
	
	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = width;
	dst_rect.height =height;
	
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = width << 16;
	src_rect.height = height << 16;
	
	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );
	
	dispman_element =
	vc_dispmanx_element_add(dispman_update, dispman_display,
				0/*layer*/, &dst_rect, 0/*src*/,
				&src_rect, DISPMANX_PROTECTION_NONE,
				0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
	
	nativewindow.element = dispman_element;
	nativewindow.width = width;
	nativewindow.height = height;
	vc_dispmanx_update_submit_sync( dispman_update );
	
	data->surface = eglCreateWindowSurface( data->display, config, &nativewindow, NULL );
	assert(data->surface != EGL_NO_SURFACE);
	
	// connect the context to the surface
	result = eglMakeCurrent(data->display, data->surface, data->surface, data->context);
	assert(EGL_FALSE != result);
}

static void egl_display_register_renderloop(egl_display_data *data)
{
	gl_renderloop *renderloop = gl_renderloop_get_global_renderloop();
	
	gl_renderloop_member *renderloop_member = gl_renderloop_member_new();
	renderloop_member->data.action = &egl_display_clearf;
	renderloop_member->data.action_data = data;
	
	renderloop->f->append_child(renderloop, renderloop_member, gl_renderloop_phase_clear);
	
	renderloop_member = gl_renderloop_member_new();
	renderloop_member->data.action = &egl_display_swapf;
	renderloop_member->data.action_data = data;
	
	renderloop->f->append_child(renderloop, renderloop_member, gl_renderloop_phase_swap);
}

egl_display_data *egl_display_init()
{
	egl_display_data *data = calloc(1, sizeof(egl_display_data));

	egl_display_init_display(data);
	
	return data;
}

void egl_display_setup()
{
	gl_texture_setup();
	gl_shape_setup();
	gl_tile_setup();
	gl_stage_setup();
	gl_container_setup();
	gl_container_2d_setup();
	gl_tiled_image_setup();
	gl_renderloop_member_setup();
	gl_renderloop_setup();
}
