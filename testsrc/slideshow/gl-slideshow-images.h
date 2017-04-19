//
//  gl-slideshow-images.h
//  Photoframe
//
//  Created by Martijn Vernooij on 17/04/2017.
//
//

#ifndef gl_slideshow_images_h
#define gl_slideshow_images_h

#include "slideshow/gl-slideshow.h"
#include "fs/gl-tree-cache-directory.h"
#include "slideshow/gl-sequence.h"
#include <stdio.h>

typedef enum {
	gl_slideshow_images_selection_type_all_recursive,
	gl_slideshow_images_selection_type_all,
	gl_slideshow_images_selection_type_onedir
} gl_slideshow_images_selection_type;

typedef struct gl_slideshow_images gl_slideshow_images;

typedef struct gl_slideshow_images_funcs {
	gl_slideshow_funcs p;
} gl_slideshow_images_funcs;

typedef struct gl_slideshow_images_data {
	gl_slideshow_data p;
	
	gl_tree_cache_directory *_source_dir;
	gl_sequence *_sequence;
	gl_slideshow_images_selection_type _selection_type;
	
	gl_tree_cache_directory *_current_selection;
} gl_slideshow_images_data;

struct gl_slideshow_images {
	gl_slideshow_images_funcs *f;
	gl_slideshow_images_data data;
};

void gl_slideshow_images_setup();
gl_slideshow_images *gl_slideshow_images_init(gl_slideshow_images *obj);
gl_slideshow_images *gl_slideshow_images_new();


#endif /* gl_slideshow_images_h */
