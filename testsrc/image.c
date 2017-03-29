
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
#include "slideshow/gl-sequence-ordered.h"
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
	gl_slide_image *slide_image = gl_slide_image_new();
	gl_tree_cache_directory *dirCache = d->dir;
	
	size_t fileCount;
	
	int ret = d->sequence->f->get_entry(d->sequence, &fileCount);
	if (ret) {
		d->sequence->f->start(d->sequence);
		((gl_object *)slide_image)->f->unref((gl_object *)slide_image);
		return NULL;
	}
	
	char *url = dirCache->f->get_nth_child_url(dirCache, (int)fileCount);
	
	if (!url) {
		((gl_object *)slide_image)->f->unref((gl_object *)slide_image);
		return NULL;
	}
	
	slide_image->data.filename = url;
	return (gl_slide *)slide_image;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return -1;
	}
	
	num_files = argc-1;
	filenames = argv;

	gl_objects_setup();

	gl_configuration *config = gl_configuration_new_from_file(argv[1]);
	config->f->load(config);
	
#ifdef __APPLE__
	startCocoa(argc, (const char**)argv, &slideshow_init);
#else
	egl_driver_init(&slideshow_init);
	gl_renderloop_loop();
#endif
}

void slideshow_init()
{
	slideshowdata *d = calloc(1, sizeof(slideshowdata));
	
	gl_config_value *cf_value = gl_configuration_get_value_for_path("Source1/url");
	if (!cf_value || (cf_value->f->get_type(cf_value) != gl_config_value_type_string)) {
		printf("Source1/url incorrectly set\n");
		exit(-1);
	}
	const char *sourceUrl = cf_value->f->get_value_string(cf_value);
	
	gl_tree_cache_directory *dirCache = (gl_tree_cache_directory *)gl_tree_cache_directory_ordered_new();
	d->dir = dirCache;
	dirCache->f->load(dirCache, sourceUrl);
	dirCache->data._url = strdup(sourceUrl);
	
	gl_slideshow *slideshow = gl_slideshow_new();
	slideshow->data.getNextSlideCallback = &get_next_slide;
	slideshow->data.callbackExtraData = d;
	
	gl_sequence_ordered *seq = gl_sequence_ordered_new();
	((gl_sequence *)seq)->f->set_count((gl_sequence *)seq, dirCache->f->get_num_child_leafs(dirCache, 1));
	((gl_sequence *)seq)->f->start((gl_sequence *)seq);
	d->sequence = (gl_sequence *)seq;
	
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
	animation->data.endValue = 1.0;
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
	
	((gl_slide *)slideshow)->f->enter((gl_slide *)slideshow);
	
	return; // not reached
}
