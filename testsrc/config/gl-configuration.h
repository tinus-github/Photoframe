//
//  gl-configuration.h
//  Photoframe
//
//  Created by Martijn Vernooij on 11/03/2017.
//
//

#ifndef gl_configuration_h
#define gl_configuration_h

#include <stdio.h>

#include "infrastructure/gl-object.h"
#include "config/gl-config-section.h"

typedef struct gl_configuration gl_configuration;

typedef struct gl_configuration_funcs {
	gl_object_funcs p;
	int (*load) (gl_configuration *obj);
	gl_config_section *(*get_section) (gl_configuration *obj, const char* title);
} gl_configuration_funcs;

typedef struct gl_configuration_data {
	gl_object_data p;
	
	gl_config_section *first_section;
	
	char *_config_filename;
} gl_configuration_data;

struct gl_configuration {
	gl_configuration_funcs *f;
	gl_configuration_data data;
};

void gl_configuration_setup();
gl_configuration *gl_configuration_init(gl_configuration *obj);
gl_configuration *gl_configuration_new();
gl_configuration *gl_configuration_new_from_file(const char *filename);
gl_configuration *gl_configuration_get_global_configuration();
gl_config_value *gl_configuration_get_value_for_path(const char* path);


#endif /* gl_configuration_h */
