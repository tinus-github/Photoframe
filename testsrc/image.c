
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
#include "labels/gl-label-scroller.h"
#include "infrastructure/gl-notice-subscription.h"
#include "slideshow/gl-slide-image.h"

#include "../lib/linmath/linmath.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

void image_set_alpha(void *target, void *extra_data, GLfloat value)
{
	gl_shape *image_shape = (gl_shape *)extra_data;
	
	image_shape->data.alpha = value;
	image_shape->f->set_computed_alpha_dirty(image_shape);
}

static unsigned int slide_counter = 1;
static unsigned int num_files;

gl_slide *get_next_slide(void *target, void *extra_data)
{
	gl_slide_image *slide_image = gl_slide_image_new();
	char *argv[] = (char**) extra_data;
	
	char *filename = argv[counter++];
	if (counter > (num_files + 1)) {
		counter = 1;
	}
	
	slide_image->data.filename = strdup(filename);
	return (gl_slide *)slide_image;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return -1;
	}
	
	num_files = argc-1;
	
	egl_driver_setup();
	egl_driver_init();
	
	gl_slideshow *slideshow = gl_slideshow_new();
	slideshow->data.getNextSlideCallback = &get_next_slide;
	slideshow->data.callbackExtraData = argv;
	
	gl_value_animation_easing *animation_e = gl_value_animation_easing_new();
	animation_e->data.easingType = gl_value_animation_ease_linear;
	
	gl_value_animation *animation = (gl_value_animation *)animation_e;
	animation->data.startValue = 0.0;
	animation->data.endValue = 1.0;
	animation->data.duration = 0.4;
	animation->data.action = image_set_alpha;
	
	slideshow->f->set_entrance_animation(slideshow, animation);
	
	animation_e = gl_value_animation_easing_new();
	animation_e->data.easingType = gl_value_animation_ease_linear;
	
	animation = (gl_value_animation *)animation_e;
	animation->data.startValue = 1.0;
	animation->data.endValue = 0.0;
	animation->data.duration = 0.4;
	animation->data.action = image_set_alpha;
	
	slideshow->f->set_exit_animation(slideshow, animation);
	
	gl_container_2d *main_container_2d = gl_container_2d_new();
	gl_container *main_container_2d_container = (gl_container *)main_container_2d;
	gl_shape *main_container_2d_shape = (gl_shape *)main_container_2d;

	main_container_2d_container->f->append_child(main_container_2d_container, (gl_shape *)slideshow);
	
	gl_label_scroller *scroller = gl_label_scroller_new();
	gl_shape *scroller_shape = (gl_shape *)scroller;
	scroller_shape->data.objectHeight = 160;
	scroller_shape->data.objectWidth = 1920;
	scroller->data.text = "AVAVAVABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	scroller->f->start(scroller);
	main_container_2d_container->f->append_child(main_container_2d_container, scroller_shape);

	main_container_2d_shape->data.objectX = 0.0;
	main_container_2d->data.scaleH = 1.0;
	main_container_2d->data.scaleV = 1.0;
	
	gl_stage *global_stage = gl_stage_get_global_stage();
	global_stage->f->set_shape(global_stage, (gl_shape *)main_container_2d);
	
	slideshow->f->start(slideshow);
	
	gl_renderloop_loop();
	
	return 0; // not reached
}
