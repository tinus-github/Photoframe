//
//  gl-slideshow-images.c
//  Photoframe
//
//  Created by Martijn Vernooij on 17/04/2017.
//
//

#include "slideshow/gl-slideshow-images.h"
#include "slideshow/gl-slide-image.h"
#include "fs/gl-source-manager.h"

#include <string.h>
#include <stdlib.h>

#ifndef __APPLE__
#include "arc4random.h"
#endif

void gl_slideshow_images_free(gl_object *obj_obj);
static void gl_slideshow_images_set_configuration(gl_slideshow *obj_slideshow, gl_config_section *config);

static struct gl_slideshow_images_funcs gl_slideshow_images_funcs_global = {

};

static void (*gl_object_free_org_global) (gl_object *obj);
static void (*gl_slideshow_set_configuration_org_global) (gl_slideshow *obj, gl_config_section *config);

void gl_slideshow_images_setup()
{
	gl_slideshow *parent = gl_slideshow_new();
	memcpy(&gl_slideshow_images_funcs_global.p, parent->f, sizeof(gl_slideshow_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_slideshow_images_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_slideshow_images_free;
	
	gl_slideshow_funcs *slideshow_funcs_global = (gl_slideshow_funcs *) &gl_slideshow_images_funcs_global;
	gl_slideshow_set_configuration_org_global = slideshow_funcs_global->set_configuration;
	slideshow_funcs_global->set_configuration = &gl_slideshow_images_set_configuration;
}

static void gl_slideshow_images_set_configuration(gl_slideshow *obj_slideshow, gl_config_section *config)
{
	gl_slideshow_set_configuration_org_global(obj_slideshow, config);
	
	gl_slideshow_images *obj = (gl_slideshow_images *)obj_slideshow;
	
	gl_config_value *value;
	
	value = config->f->get_value(config, "source");
	if (!value) {
		// TODO: Show error?
		return;
	}
	const char *url = value->f->get_value_string(value);
	if (!url) {
		return;
	}
	gl_source_manager *global_manager = gl_source_manager_get_global_manager();
	
	obj->data._source_dir = global_manager->f->get_source(global_manager, url);
	if (!obj->data._source_dir) {
		return;
	}
	
	static gl_config_value_selection selectionTypes[] = {
		{ "allRecursive", gl_slideshow_images_selection_type_all_recursive },
		{ "all", gl_slideshow_images_selection_type_all },
		{ "onedir", gl_slideshow_images_selection_type_onedir },
		{ NULL, 0 }
	};
	
	value = config->f->get_value(config, "selectionType");
	if (value) {
		int32_t selectionType = value->f->get_value_string_selection(value, selectionTypes);
		if (selectionType == GL_CONFIG_VALUE_SELECTION_NOT_FOUND) {
			selectionType = gl_slideshow_images_selection_type_all_recursive;
		}
		obj->data._selection_type = (gl_slideshow_images_selection_type)selectionType;
	} else {
		obj->data._selection_type = gl_slideshow_images_selection_type_all_recursive;
	}
	
	static gl_config_value_selection sequenceTypes[] =  {
		{ "ordered", gl_sequence_type_ordered },
		{ "random", gl_sequence_type_random },
		{ "selection", gl_sequence_type_selection },
		{ NULL, 0 }
	};
	
	gl_sequence_type sequence_type = gl_sequence_type_random;
	value = config->f->get_value(config, "sequenceType");
	if (value) {
		int32_t sequenceTypeInt = value->f->get_value_string_selection(value, sequenceTypes);
		if (sequenceTypeInt != GL_CONFIG_VALUE_SELECTION_NOT_FOUND) {
			sequence_type = (gl_sequence_type)sequenceTypeInt;
		}
	}
	value = NULL;
	
	obj->data._sequence = gl_sequence_new_with_type(sequence_type);
	obj->data._sequence->f->set_configuration(obj->data._sequence, config);
}

static gl_tree_cache_directory *gl_slideshow_images_resolve_source(gl_slideshow_images *obj)
{
	size_t branchCount, branchIndex;
	
	switch (obj->data._selection_type) {
		case gl_slideshow_images_selection_type_all:
		case gl_slideshow_images_selection_type_all_recursive:
			return obj->data._source_dir;
		case gl_slideshow_images_selection_type_onedir:
			branchCount = obj->data._source_dir->f->get_num_branches(obj->data._source_dir, 1);
			branchIndex = arc4random_uniform((uint32_t)branchCount + 1);
			
			return obj->data._source_dir->f->get_nth_branch(obj->data._source_dir, (unsigned int)branchIndex);
	}
	
	return NULL;
}

static gl_slide *gl_slideshow_images_get_next_slide(void *target, void *extra_data)
{
	gl_slideshow_images *obj = (gl_slideshow_images *)target;
	
	if ((!obj->data._source_dir) ||
	    (!obj->data._sequence)) {
		return NULL;
	}
	
	if (!obj->data._current_selection) {
		obj->data._current_selection = gl_slideshow_images_resolve_source(obj);
		if (!obj->data._current_selection) {
			return NULL;
		}
		
		((gl_object *)obj->data._current_selection)->f->ref((gl_object *)obj->data._current_selection);
		
		int is_recursive = 0;
		if (obj->data._selection_type == gl_slideshow_images_selection_type_all_recursive) {
			is_recursive = 1;
		}
		
		size_t fileCount = obj->data._current_selection->f->get_num_child_leafs(obj->data._current_selection, is_recursive);
		
		obj->data._sequence->f->set_count(obj->data._sequence, fileCount);
		obj->data._sequence->f->start(obj->data._sequence);
	}
	
	size_t fileIndex;
	int retVal = obj->data._sequence->f->get_entry(obj->data._sequence, &fileIndex);
	if (retVal) {
		((gl_object *)obj->data._current_selection)->f->unref((gl_object *)obj->data._current_selection);
		obj->data._current_selection = 0;
		
		return NULL;
	}
	
	char *url = obj->data._current_selection->f->get_nth_child_url(obj->data._current_selection, (int)fileIndex);
	if (!url) { // not sure how this can happen
		return NULL;
	}
	
	gl_slide_image *ret_image = gl_slide_image_new();
	ret_image->data.filename = url;
	
	return (gl_slide *)ret_image;
}

gl_slideshow_images *gl_slideshow_images_init(gl_slideshow_images *obj)
{
	gl_slideshow *obj_slideshow = (gl_slideshow *)obj;
	gl_slideshow_init(obj_slideshow);
	
	obj->f = &gl_slideshow_images_funcs_global;
	
	obj_slideshow->data.getNextSlideCallback = &gl_slideshow_images_get_next_slide;
	obj_slideshow->data.callbackTarget = obj;
	
	return obj;
}

gl_slideshow_images *gl_slideshow_images_new()
{
	gl_slideshow_images *ret = calloc(1, sizeof(gl_slideshow_images));
	
	return gl_slideshow_images_init(ret);
}

void gl_slideshow_images_free(gl_object *obj_obj)
{
	gl_slideshow_images *obj = (gl_slideshow_images *)obj_obj;
	
	if (obj->data._sequence) {
		((gl_object *)obj->data._sequence)->f->unref((gl_object *)obj->data._sequence);
		obj->data._sequence = NULL;
	}
	
	if (obj->data._current_selection) {
		((gl_object *)obj->data._current_selection)->f->unref((gl_object *)obj->data._current_selection);
		obj->data._current_selection = NULL;
	}
	
	gl_object_free_org_global(obj_obj);
}
