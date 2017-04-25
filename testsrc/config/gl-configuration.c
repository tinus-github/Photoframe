//
//  gl-configuration.c
//  Photoframe
//
//  Created by Martijn Vernooij on 11/03/2017.
//
//

#include "config/gl-configuration.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <assert.h>
#include "minIni.h"


static gl_configuration *global_configuration;

static int gl_configuration_load(gl_configuration *obj);
static gl_config_section *gl_configuration_get_section(gl_configuration *obj, const char* title);
static void gl_configuration_free(gl_object *obj_obj);

static struct gl_configuration_funcs gl_configuration_funcs_global = {
	.load = &gl_configuration_load,
	.get_section = &gl_configuration_get_section
};

static void (*gl_object_free_org_global) (gl_object *obj);

static int gl_configuration_load_callback(const mTCHAR *Section, const mTCHAR *Key, const mTCHAR *Value, const void *UserData)
{
	gl_configuration *obj = (gl_configuration *)UserData;
	
	gl_config_section *firstSection = obj->data.first_section;
	
	if ((!firstSection) ||
	    ((!Section) && (firstSection->f->get_title(firstSection))) ||
	    (Section && strcmp(Section, firstSection->f->get_title(firstSection)))) {
		if (Section) {
			char *privSectionTitle = strdup(Section);
			obj->data.first_section = gl_config_section_new_with_title(privSectionTitle);
		} else {
			obj->data.first_section = gl_config_section_new();
		}
		obj->data.first_section->data.next_sibling = firstSection;
		firstSection = obj->data.first_section;
	}
	
	char *endInt;
	gl_config_value *value;
	
	char *keyCopy = strdup(Key);
	if (!keyCopy) {
		return 0;
	}
	
	errno = 0;
	intmax_t intValue = strtoimax(Value, &endInt, 10);

	if (errno || (intValue > INT32_MAX) || (intValue < INT32_MIN) || (*endInt != '\0')) {
		char *valueCopy = strdup(Value);
		if (!valueCopy) {
			free(keyCopy);
			return 0;
		}
		value = gl_config_value_new_with_title_string(keyCopy, valueCopy);
	} else {
		int32_t int32_value = (int32_t)intValue;
		value = gl_config_value_new_with_title_int(keyCopy, int32_value);
	}
	
	firstSection->f->_append_value(firstSection, value);
	
	return 1;
}


static int gl_configuration_load(gl_configuration *obj)
{
	errno = 0;
	if (!ini_browse(&gl_configuration_load_callback, obj, obj->data._config_filename)) {
		if (errno) {
			return errno;
		}
		return EINVAL;
	}
	return 0;
}

static gl_config_section *gl_configuration_get_section(gl_configuration *obj, const char* title)
{
	gl_config_section *child = obj->data.first_section;
	
	while (child) {
		if (!title) {
			if (!child->f->get_title(child)) {
				return child;
			}
		} else {
			if (!strcmp(title, child->f->get_title(child))) {
				return child;
			}
		}
		child = child->data.next_sibling;
	}
	
	return NULL;
}

void gl_configuration_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_configuration_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_configuration_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_configuration_free;
}

gl_configuration *gl_configuration_init(gl_configuration *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_configuration_funcs_global;
	
	return obj;
}

gl_configuration *gl_configuration_new()
{
	gl_configuration *ret = calloc(1, sizeof(gl_configuration));
	
	if (!global_configuration) {
		global_configuration = ret;
	}
	
	return gl_configuration_init(ret);
}

gl_configuration *gl_configuration_new_from_file(const char *filename)
{
	gl_configuration *ret = gl_configuration_new();
	if (!ret) {
		return ret;
	}
	
	ret->data._config_filename = strdup(filename);
	
	return ret;
}

static void gl_configuration_free(gl_object *obj_obj)
{
	gl_configuration *obj = (gl_configuration *)obj_obj;
	
	gl_config_section *child = obj->data.first_section;
	gl_config_section *next_child;
	
	while (child) {
		next_child = child->data.next_sibling;
		((gl_object *)child)->f->unref((gl_object *)child);
		child = next_child;
	}
	
	obj->data.first_section = NULL;
	
	free (obj->data._config_filename);
	obj->data._config_filename = NULL;
	
	return gl_object_free_org_global(obj_obj);
}

gl_configuration *gl_configuration_get_global_configuration()
{
	return global_configuration;
}

gl_config_value *gl_configuration_get_value_for_path(const char* path)
{
	gl_configuration *cf = gl_configuration_get_global_configuration();
	gl_config_value *value = NULL;
	char *sectionTitle = NULL;
	
	assert (path);
	
	char *sectionTitleEnd = strchr(path, '/');
	const char *valueTitleStart = NULL;
	if (sectionTitleEnd) {
		sectionTitle = strndup(path, sectionTitleEnd-path);
		valueTitleStart = sectionTitleEnd + 1;
	} else {
		sectionTitle = strdup("");
		valueTitleStart = path;
	}
	
	if (cf) {
		gl_config_section *section = cf->f->get_section(cf, sectionTitle);
		if (section) {
			value = section->f->get_value(section, valueTitleStart);
		}
	}
	
	free (sectionTitle);

	return value;
}
