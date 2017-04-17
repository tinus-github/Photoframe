
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
#include "slideshow/gl-slide-image.h"
#include "slideshow/gl-slideshow.h"
#include "slideshow/gl-sequence-selection.h"
#include "config/gl-configuration.h"
#include "fs/gl-tree-cache-directory-ordered.h"

#include "../lib/linmath/linmath.h"

#ifndef __APPLE__
#include "arc4random.h"
#endif

// from esUtil.h
#define TRUE 1
#define FALSE 0

typedef struct slideshowdata {
	gl_tree_cache_directory *dir;
	gl_tree_cache_directory *branch;
	gl_sequence *sequence;
} slideshowdata;

void slideshow_init();

void image_set_alpha(void *target, void *extra_data, GLfloat value)
{
	gl_shape *image_shape = (gl_shape *)extra_data;
	
	image_shape->data.alpha = value;
	image_shape->f->set_computed_alpha_dirty(image_shape);
}

static unsigned int num_files;
static char** filenames;

char *url_from_path(const char* path)
{
	char *ret = malloc(strlen(path) + 8);
	strcpy(ret, "file://");
	strcat(ret, path);
	return ret;
}

gl_slide *get_next_slide(void *target, void *extra_data)
{
	slideshowdata *d = (slideshowdata *)extra_data;
	gl_slide_image *slide_image;
	gl_tree_cache_directory *dirCache = d->dir;
	
	if (!d->branch) {
		size_t branchIndex = arc4random_uniform(dirCache->f->get_num_branches(dirCache, 1));
		d->branch = dirCache->f->get_nth_branch(dirCache, (unsigned int)branchIndex);
		
		if (!d->branch) {
			return NULL;
		}
		size_t fileCount = d->branch->f->get_num_child_leafs(d->branch, 0);
		if (d->sequence) {
			((gl_object *)d->sequence)->f->unref((gl_object *)d->sequence);
			d->sequence = NULL;
		}
		if (!fileCount) {
			d->branch = NULL;
			return NULL;
		}
		d->sequence = (gl_sequence *)gl_sequence_selection_new();
		((gl_sequence_selection *)d->sequence)->data._selection_size = 10;
		
		d->sequence->f->set_count(d->sequence, fileCount);
		d->sequence->f->start(d->sequence);
		char *branchUrl = d->branch->f->get_url(d->branch);
		gl_stage *global_stage = gl_stage_get_global_stage();
		global_stage->f->show_message(global_stage, branchUrl, 0);
		free (branchUrl);
	}
	
	size_t fileIndex;
	
	int ret = d->sequence->f->get_entry(d->sequence, &fileIndex);
	if (ret) {
		d->branch = NULL;
		return NULL;
	}
	
	char *url = d->branch->f->get_nth_child_url(d->branch, (int)fileIndex);
	
	if (!url) {
		return NULL;
	}
	
	slide_image = gl_slide_image_new();
	
	slide_image->data.filename = url;
	return (gl_slide *)slide_image;
}

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

	slideshowdata *d = calloc(1, sizeof(slideshowdata));
	
	gl_config_value *cf_value = gl_configuration_get_value_for_path("Source1/url");
	
	if (!cf_value || (cf_value->f->get_type(cf_value) != gl_config_value_type_string)) {
		global_stage->f->show_message(global_stage, "Configuration issue: Source1/url set incorrectly", 0);
		
		return;
	}
	const char *sourceUrl = cf_value->f->get_value_string(cf_value);
	
	gl_tree_cache_directory *dirCache = (gl_tree_cache_directory *)gl_tree_cache_directory_ordered_new();
	d->dir = dirCache;
	dirCache->f->load(dirCache, sourceUrl);
	dirCache->data._url = strdup(sourceUrl);
	
	gl_slideshow *slideshow = gl_slideshow_new();
	slideshow->data.getNextSlideCallback = &get_next_slide;
	slideshow->data.callbackExtraData = d;
	
	gl_configuration *global_config = gl_configuration_get_global_configuration();
	gl_config_section *cf_section = global_config->f->get_section(global_config, "Slideshow1");
	
	if (cf_section) {
		slideshow->f->set_configuration(slideshow, cf_section);
	} else {
		
		gl_value_animation_easing *animation_e = gl_value_animation_easing_new();
		animation_e->data.easingType = gl_value_animation_ease_linear;
		
		gl_value_animation *animation = (gl_value_animation *)animation_e;
		animation->data.startValue = 0.0;
		animation->data.endValue = 1.0;
		animation->f->set_duration(animation, 0.4);
		animation->data.action = image_set_alpha;
		
		slideshow->f->set_entrance_animation(slideshow, animation);
		
		animation_e = gl_value_animation_easing_new();
		animation_e->data.easingType = gl_value_animation_ease_linear;
		
		animation = (gl_value_animation *)animation_e;
		animation->data.startValue = 1.0;
		animation->data.endValue = 1.0;
		animation->f->set_duration(animation, 0.4);
		animation->data.action = image_set_alpha;
		
		slideshow->f->set_exit_animation(slideshow, animation);

	}
	
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
