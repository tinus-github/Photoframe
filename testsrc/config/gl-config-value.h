//
//  gl-config-value.h
//  Photoframe
//
//  Created by Martijn Vernooij on 11/03/2017.
//
//

#ifndef gl_config_value_h
#define gl_config_value_h

#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "infrastructure/gl-object.h"

#define GL_CONFIG_VALUE_SELECTION_NOT_FOUND INT_MAX

typedef enum {
	gl_config_value_type_none = 0,
	gl_config_value_type_string,
	gl_config_value_type_int
} gl_config_value_type;

typedef struct {
	const char *name;
	int32_t value;
} gl_config_value_selection;

typedef struct gl_config_value gl_config_value;

typedef struct gl_config_value_funcs {
	gl_object_funcs p;
	gl_config_value_type (*get_type) (gl_config_value *obj);
	const char *(*get_value_string) (gl_config_value *obj);
	int32_t (*get_value_int) (gl_config_value *obj);
	const char*(*get_title) (gl_config_value *obj);
	int32_t (*get_value_string_selection) (gl_config_value *obj, gl_config_value_selection *options);
} gl_config_value_funcs;

typedef struct gl_config_value_data {
	gl_object_data p;
	
	gl_config_value_type _type;
	char *_value_string;
	int32_t _value_int;
	char *_title;
	
	gl_config_value *next_sibling;
} gl_config_value_data;

struct gl_config_value {
	gl_config_value_funcs *f;
	gl_config_value_data data;
};

void gl_config_value_setup();
gl_config_value *gl_config_value_init(gl_config_value *obj);
gl_config_value *gl_config_value_new();
gl_config_value *gl_config_value_new_with_title_string(char *title, char *value);
gl_config_value *gl_config_value_new_with_title_int(char *title, int32_t value);


#endif /* gl_config_value_h */
