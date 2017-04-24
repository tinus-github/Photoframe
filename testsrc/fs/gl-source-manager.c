//
//  gl-source-manager.c
//  Photoframe
//
//  Created by Martijn Vernooij on 18/04/2017.
//
//

#include "fs/gl-source-manager.h"
#include "fs/gl-tree-cache-directory-ordered.h"
#include "fs/gl-tree-cache-directory-reloading.h"
#include "config/gl-configuration.h"
#include <string.h>
#include <stdlib.h>

static gl_source_manager *global_source_manager;

static gl_tree_cache_directory *gl_source_manager_get_source(gl_source_manager *obj, const char *name);

static struct gl_source_manager_funcs gl_source_manager_funcs_global = {
	.get_source = &gl_source_manager_get_source
};

static gl_tree_cache_directory *gl_source_manager_get_source(gl_source_manager *obj, const char *name)
{
	gl_tree_cache_directory *ret;
	// first, try looking it up in the list
	
	gl_source_manager_entry *current_entry = obj->data.first_entry;
	
	while (current_entry) {
		if (!strcasecmp(current_entry->name, name)) {
			ret = current_entry->source;
			((gl_object *)ret)->f->ref((gl_object *)ret);
			
			return ret;
		}
		current_entry = current_entry->next;
	}
	
	gl_configuration *global_config = gl_configuration_get_global_configuration();
	gl_config_section *section = global_config->f->get_section(global_config, name);
	
	if (!section) {
		return NULL;
	}
	
	
	gl_config_value *value = section->f->get_value(section, "url");
	if (!value) {
		return NULL;
	}
	const char *url = value->f->get_value_string(value);
	if (!url) { // TODO: show error?
		return NULL;
	}
	
	ret = (gl_tree_cache_directory *)gl_tree_cache_directory_reloading_new();
	ret->f->load(ret, url);
	
	gl_source_manager_entry *new_entry = calloc(1, sizeof(gl_source_manager_entry));
	new_entry->name = strdup(name);
	new_entry->source = ret;
	new_entry->next = obj->data.first_entry;
	obj->data.first_entry = new_entry;
	
	return ret;
}

void gl_source_manager_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_source_manager_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	((gl_object *)parent)->f->free((gl_object *)parent);
}

gl_source_manager *gl_source_manager_init(gl_source_manager *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_source_manager_funcs_global;
	
	return obj;
}

gl_source_manager *gl_source_manager_new()
{
	gl_source_manager *ret = calloc(1, sizeof(gl_source_manager));
	
	if (!global_source_manager) {
		global_source_manager = ret;
	}
	
	return gl_source_manager_init(ret);
}

gl_source_manager *gl_source_manager_get_global_manager()
{
	if (!global_source_manager) {
		gl_source_manager_new();
	}
	
	return global_source_manager;
}
