//
//  gl-config-section.h
//  Photoframe
//
//  Created by Martijn Vernooij on 11/03/2017.
//
//

#ifndef gl_config_section_h
#define gl_config_section_h

#include <stdio.h>

#include "infrastructure/gl-object.h"
#include "config/gl-config-value.h"

typedef struct gl_config_section gl_config_section;

typedef struct gl_config_section_funcs {
	gl_object_funcs p;
	gl_config_value *(*get_value) (gl_config_section *obj, const char *title);
	const char *(*get_title) (gl_config_section *obj);
	
	void (*_append_value) (gl_config_section *obj, gl_config_value *value);
} gl_config_section_funcs;

typedef struct gl_config_section_data {
	gl_object_data p;
	
	char *_title;
	gl_config_value *first_child;
	gl_config_section *next_sibling;
} gl_config_section_data;

struct gl_config_section {
	gl_config_section_funcs *f;
	gl_config_section_data data;
};

void gl_config_section_setup();
gl_config_section *gl_config_section_init(gl_config_section *obj);
gl_config_section *gl_config_section_new();
gl_config_section *gl_config_section_new_with_title(char *title);



#endif /* gl_config_section_h */
