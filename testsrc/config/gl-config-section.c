//
//  gl-config-section.c
//  Photoframe
//
//  Created by Martijn Vernooij on 11/03/2017.
//
//

#include "config/gl-config-section.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

static void gl_config_section_free(gl_object *obj_obj);
static const char *gl_config_section_get_title(gl_config_section *obj);
static gl_config_value *gl_config_section_get_value(gl_config_section *obj, const char *title);
static void gl_config_section_append_value(gl_config_section *obj, gl_config_value *value);

static struct gl_config_section_funcs gl_config_section_funcs_global = {
	.get_value = &gl_config_section_get_value,
	.get_title = &gl_config_section_get_title,
	._append_value = &gl_config_section_append_value
};

static void (*gl_object_free_org_global) (gl_object *obj);

static const char *gl_config_section_get_title(gl_config_section *obj)
{
	return obj->data._title;
}

static gl_config_value *gl_config_section_get_value(gl_config_section *obj, const char *title)
{
	gl_config_value *child = obj->data.first_child;
	
	assert (title);
	
	while (child) {
		if (!strcmp(child->f->get_title(child), title)) {
			return child;
		}
		child = child->data.next_sibling;
	}
	
	return NULL;
}

static void gl_config_section_append_value(gl_config_section *obj, gl_config_value *value)
{
	value->data.next_sibling = obj->data.first_child;
	obj->data.first_child = value;
}

void gl_config_section_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_config_section_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_config_section_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_config_section_free;
}

gl_config_section *gl_config_section_init(gl_config_section *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_config_section_funcs_global;
	
	return obj;
}

gl_config_section *gl_config_section_new()
{
	gl_config_section *ret = calloc(1, sizeof(gl_config_section));
	
	return gl_config_section_init(ret);
}

gl_config_section *gl_config_section_new_with_title(char *title)
{
	gl_config_section *ret = gl_config_section_new();
	
	if (!ret) {
		return ret;
	}
	
	ret->data._title = title;
	
	return ret;
}

static void gl_config_section_free(gl_object *obj_obj)
{
	gl_config_section *obj = (gl_config_section *)obj_obj;
	
	gl_config_value *child = obj->data.first_child;
	gl_config_value *next_child;
	while (child) {
		next_child = child->data.next_sibling;
		((gl_object *)child)->f->unref((gl_object *)child);
		child = next_child;
	}
	obj->data.first_child = NULL;
	
	free (obj->data._title);
	obj->data._title = NULL;
	
	return gl_object_free_org_global(obj_obj);
}
