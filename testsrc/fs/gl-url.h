//
//  gl-url.h
//  Photoframe
//
//  Created by Martijn Vernooij on 14/02/2017.
//
//

#ifndef gl_url_h
#define gl_url_h

#include <stdio.h>

#include "infrastructure/gl-object.h"

typedef struct gl_url gl_url;

typedef struct gl_url_funcs {
	gl_object_funcs p;
	int (*decode) (gl_url *obj, const char *urlString);
	int (*decode_scheme) (gl_url *obj, const char *urlString);
	
	int (*url_escape) (gl_url *obj, const char *input, char **output);
	int (*url_unescape) (gl_url *obj, const char *input, char **output);
} gl_url_funcs;

typedef struct gl_url_data {
	gl_object_data p;
	
	char *scheme;
	char *username;
	char *password;
	char *host;
	unsigned int port;
	char *path;
	char *arguments;
	char *fragment;
} gl_url_data;

struct gl_url {
	gl_url_funcs *f;
	gl_url_data data;
};

void gl_url_setup();
gl_url *gl_url_init(gl_url *obj);
gl_url *gl_url_new();


#endif /* gl_url_h */
