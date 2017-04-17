//
//  gl-config-value.c
//  Photoframe
//
//  Created by Martijn Vernooij on 11/03/2017.
//
//

#include "config/gl-config-value.h"
#include <string.h>
#include <stdlib.h>

static gl_config_value_type gl_config_value_get_type(gl_config_value *obj);
static const char *gl_config_value_get_title(gl_config_value *obj);
static const char *gl_config_value_get_value_string(gl_config_value *obj);
static int32_t gl_config_get_value_string_selection(gl_config_value *obj, gl_config_value_selection *options);
static int gl_config_value_get_value_int(gl_config_value *obj);
static void gl_config_value_free(gl_object *obj_obj);

static struct gl_config_value_funcs gl_config_value_funcs_global = {
	.get_type = &gl_config_value_get_type,
	.get_title = &gl_config_value_get_title,
	.get_value_string = &gl_config_value_get_value_string,
	.get_value_int = &gl_config_value_get_value_int,
	.get_value_string_selection = &gl_config_get_value_string_selection,
};

static void (*gl_object_free_org_global) (gl_object *obj);

static gl_config_value_type gl_config_value_get_type(gl_config_value *obj)
{
	return obj->data._type;
}

static const char *gl_config_value_get_title(gl_config_value *obj)
{
	return obj->data._title;
}

static const char *gl_config_value_get_value_string(gl_config_value *obj)
{
	if (obj->data._type != gl_config_value_type_string) {
		return NULL;
	}
	return obj->data._value_string;
}

static int32_t gl_config_value_get_value_int(gl_config_value *obj)
{
	if (obj->data._type != gl_config_value_type_int) {
		return 0;
	}
	return obj->data._value_int;
}

static int32_t gl_config_get_value_string_selection(gl_config_value *obj, gl_config_value_selection *options)
{
	if (obj->data._type != gl_config_value_type_string) {
		return GL_CONFIG_VALUE_SELECTION_NOT_FOUND;
	}
	
	const char *value = obj->data._value_string;
	
	while (options) {
		if (!strcasecmp(value, options->name)) {
			return options->value;
		}
		options++;
	}
	
	return GL_CONFIG_VALUE_SELECTION_NOT_FOUND;
}

void gl_config_value_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_config_value_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_config_value_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_config_value_free;
}

static void gl_config_value_free(gl_object *obj_obj)
{
	gl_config_value *obj = (gl_config_value *)obj_obj;
	
	if (obj->f->get_type(obj) == gl_config_value_type_string) {
		free(obj->data._value_string);
		obj->data._value_string = NULL;
	}
	
	return gl_object_free_org_global(obj_obj);
}

gl_config_value *gl_config_value_init(gl_config_value *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_config_value_funcs_global;
	
	return obj;
}

gl_config_value *gl_config_value_new()
{
	gl_config_value *ret = calloc(1, sizeof(gl_config_value));
	
	return gl_config_value_init(ret);
}

gl_config_value *gl_config_value_new_with_title_string(char *title, char *value)
{
	gl_config_value *ret = gl_config_value_new();
	
	if (!ret) {
		return ret;
	}
	
	ret->data._title = title;
	ret->data._type = gl_config_value_type_string;
	ret->data._value_string = value;
	
	return ret;
}

gl_config_value *gl_config_value_new_with_title_int(char *title, int32_t value)
{
	gl_config_value *ret = gl_config_value_new();
	
	if (!ret) {
		return ret;
	}
	
	ret->data._title = title;
	ret->data._type = gl_config_value_type_int;
	ret->data._value_int = value;
	
	return ret;
}
