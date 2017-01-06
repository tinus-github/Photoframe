
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
#include "gl-texture.h"
#include "gl-tile.h"
#include "gl-container-2d.h"
#include "gl-stage.h"
#include "gl-tiled-image.h"
#include "gl-renderloop.h"
#include "egl-driver.h"
#include "gl-value-animation.h"
#include "gl-value-animation-easing.h"
#include "labels/gl-label-scroller-segment.h"

#include "../lib/linmath/linmath.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

void animation(void *target, void *extra_data, GLfloat value)
{
	gl_shape *container_shape = (gl_shape *)target;
	gl_label_scroller_segment *label_seg = (gl_label_scroller_segment *)target;
	container_shape->data.objectX = value;
	container_shape->f->set_computed_projection_dirty(container_shape);
	
	label_seg->data.exposedSectionLeft = -value;
	label_seg->f->render(label_seg);
}

int main(int argc, char *argv[])
{
	int width, height;
	unsigned int orientation;
	
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	
	unsigned char *image;
	
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
	
	egl_driver_setup();
	egl_driver_init();

	
	gl_tiled_image *tiled_image = gl_tiled_image_new();
	gl_container_2d *main_container_2d = gl_container_2d_new();
	gl_container *main_container_2d_container = (gl_container *)main_container_2d;
	gl_shape *main_container_2d_shape = (gl_shape *)main_container_2d;
	
	tiled_image->f->load_image(tiled_image, image, width, height, orientation, 128);
	main_container_2d_container->f->append_child(main_container_2d_container, (gl_shape *)tiled_image);
	
	gl_label_scroller_segment *label = gl_label_scroller_segment_new();
	label->data.width = 1024;
	label->data.height = 160;
	label->data.text = "AVAVAVABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
	label->f->layout(label);
	label->data.exposedSectionWidth = 1920;
	main_container_2d_container->f->append_child(main_container_2d_container, (gl_shape *)label);

	main_container_2d_shape->data.objectX = 0.0;
	main_container_2d->data.scaleH = 1.0;
	main_container_2d->data.scaleV = 1.0;
	
	gl_stage *global_stage = gl_stage_get_global_stage();
	global_stage->f->set_shape(global_stage, (gl_shape *)main_container_2d);
 
	gl_value_animation_easing *anim_easing = gl_value_animation_easing_new();
	gl_value_animation *anim = (gl_value_animation *)anim_easing;
	anim->data.target = label;
	anim->data.action = &animation;
	anim->data.startValue = 1920.0;
	anim->data.endValue = 0.0 - label->data.textWidth;
	anim->f->set_speed(anim, 180);
	anim->data.repeats = TRUE;
	anim_easing->data.easingType = gl_value_animation_ease_linear;
	
	anim->f->start(anim);
	
	gl_renderloop_loop();
	
	return 0; // not reached
}
