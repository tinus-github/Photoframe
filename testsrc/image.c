
/*
 * code stolen from openGL-RPi-tutorial-master/encode_OGL/
 * and from OpenGL® ES 2.0 Programming Guide
 */

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <bcm_host.h>

#include "loadimage.h"
#include "gl-display.h"
#include "gl-texture.h"
#include "gl-tile.h"
#include "gl-container-2d.h"
#include "gl-stage.h"
#include "gl-tiled-image.h"
#include "gl-renderloop.h"

#include "../lib/linmath/linmath.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

void gl_stage_draw(GL_STATE_T *p_state)
{
	gl_renderloop *global_loop = gl_renderloop_get_global_renderloop();
	global_loop->f->run(global_loop);
}

int main(int argc, char *argv[])
{
	int width, height;
	unsigned int orientation;
	
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	
	unsigned char *image;
	GL_STATE_T state, *p_state = &state;
	
	//image = esLoadTGA("jan.tga", &width, &height);
	gettimeofday ( &t1 , &tz );
	
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return -1;
	}
	
	image = loadJPEG(argv[1], 1920, 1080, &width, &height, &orientation);
	if (image == NULL) {
		fprintf(stderr, "No such image\n");
		exit(1);
	}
	fprintf(stderr, "Image is %d x %d\n", width, height);
	gettimeofday(&t2, &tz);
	deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
	
	printf("Image loaded in %1.4f seconds\n", deltatime);
	
	
	bcm_host_init();
	
	gl_display_init(p_state);

#if 0
	if(!gl_image_init(p_state, image, width, height, orientation))
		return 0;
	
	gl_display_register_draw_func(p_state, gl_image_draw);
#endif

#if 1
	gl_tiled_image *tiled_image = gl_tiled_image_new();
	gl_container_2d *main_container_2d = gl_container_2d_new();
	gl_container *main_container_2d_container = (gl_container *)main_container_2d;
	gl_shape *main_container_2d_shape = (gl_shape *)main_container_2d;
	
	tiled_image->f->load_image(tiled_image, image, width, height, orientation, 128);
	
	main_container_2d_container->f->append_child(main_container_2d_container, (gl_shape *)tiled_image);
	
//	image_tile = gl_tile_new();
//	image_tile->f->set_texture(image_tile, image_texture);
//	main_container_2d_container->f->append_child(main_container_2d_container, (gl_shape *)image_tile);
//	gl_shape *image_tile_shape = (gl_shape *)image_tile;
//	image_tile_shape->data.objectX = 150;
	
	gl_display_register_draw_func(p_state, gl_stage_draw);
	
	main_container_2d_shape->data.objectX = 50.0;
	main_container_2d->data.scaleH = 2.0;
	main_container_2d->data.scaleV = 2.0;
	
	gl_stage *global_stage = gl_stage_get_global_stage();
	global_stage->f->set_shape(global_stage, (gl_shape *)main_container_2d);
#endif

#if 0
	if (!gl_rect_init(p_state, width, height, 1.0f, 0.0f, 0.0f))
		return 0;
	
	gl_display_register_draw_func(p_state, gl_rect_draw);
#endif
	
	eglSwapBuffers(p_state->display, p_state->surface);
	esMainLoop(p_state);
	
	return 0; // not reached
}
