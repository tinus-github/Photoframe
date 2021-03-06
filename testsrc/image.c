
/*
 * code stolen from openGL-RPi-tutorial-master/encode_OGL/
 * and from OpenGL® ES 2.0 Programming Guide
 */

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

#include "gl-includes.h"

#include "images/loadimage.h"
#include "gl-texture.h"
#include "gl-tile.h"
#include "gl-container-2d.h"
#include "gl-stage.h"
#include "gl-tiled-image.h"
#include "gl-renderloop.h"
#include "driver.h"
#include "gl-value-animation.h"
#include "gl-value-animation-easing.h"
#include "labels/gl-label-scroller.h"
#include "infrastructure/gl-notice-subscription.h"
#include "slideshow/gl-slideshow.h"
#include "slideshow/gl-slideshow-images.h"
#include "config/gl-configuration.h"
#include "../lib/linmath/linmath.h"

#ifndef __APPLE__
#include "arc4random.h"
#endif

// from esUtil.h
#define TRUE 1
#define FALSE 0

void slideshow_init();

void image_set_alpha(void *target, void *extra_data, GLfloat value)
{
	gl_shape *image_shape = (gl_shape *)extra_data;
	
	image_shape->data.alpha = value;
	image_shape->f->set_computed_alpha_dirty(image_shape);
}

static unsigned int num_files;
static char** filenames;

void cf_fail_init()
{
	gl_stage *global_stage = gl_stage_get_global_stage();
	global_stage->f->show_message(global_stage, "Can't open configuration file", 1);
	return;
}

int main(int argc, char *argv[])
{
	void (*initFunc)() = &slideshow_init;
	
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return -1;
	}
	
	num_files = argc-1;
	filenames = argv;

	gl_objects_setup();

	gl_configuration *config = gl_configuration_new_from_file(argv[1]);
	if (config->f->load(config)) {
		initFunc = &cf_fail_init;
	}
	
#ifdef __APPLE__
	startCocoa(argc, (const char**)argv, initFunc);
#else
	egl_driver_init(initFunc);
	gl_renderloop_loop();
#endif
}

void slideshow_init()
{
	gl_stage *global_stage = gl_stage_get_global_stage();
	
	if (global_stage->data.fatal_error_occurred) {
		return;
	}
	
	gl_slideshow_images *slideshow_images = gl_slideshow_images_new();
	gl_slideshow *slideshow = (gl_slideshow *)slideshow_images;
	
	gl_configuration *global_config = gl_configuration_get_global_configuration();
	gl_config_section *cf_section = global_config->f->get_section(global_config, "Slideshow1");
	
	if (!cf_section) {
		global_stage->f->show_message(global_stage, "Configuration section for main slideshow not found", 1);
		return;
	}
	
	slideshow->f->set_configuration(slideshow, cf_section);
	
	gl_container_2d *main_container_2d = gl_container_2d_new();
	gl_container *main_container_2d_container = (gl_container *)main_container_2d;
	gl_shape *main_container_2d_shape = (gl_shape *)main_container_2d;

	main_container_2d_container->f->append_child(main_container_2d_container, (gl_shape *)slideshow);
	
	gl_label_scroller *scroller = gl_label_scroller_new();
	gl_shape *scroller_shape = (gl_shape *)scroller;
	scroller_shape->data.objectHeight = 160;
	scroller_shape->data.objectWidth = global_stage->data.width;
	scroller->data.text = "AVAVAVABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	scroller->f->start(scroller);
	main_container_2d_container->f->append_child(main_container_2d_container, scroller_shape);
	
	main_container_2d_shape->data.objectX = 0.0;
	main_container_2d->data.scaleH = 1.0;
	main_container_2d->data.scaleV = 1.0;
	
	if (global_stage->data.fatal_error_occurred) {
		return;
	}
	
	global_stage->f->set_shape(global_stage, (gl_shape *)main_container_2d);
	
	((gl_slide *)slideshow)->f->enter((gl_slide *)slideshow);
	
	return; // not reached
}
