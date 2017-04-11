//
//  gl-qrcode-data.h
//  Photoframe
//
//  Created by Martijn Vernooij on 11/04/2017.
//
//

#ifndef gl_qrcode_data_h
#define gl_qrcode_data_h

#include <stdio.h>

#include "infrastructure/gl-object.h"

typedef enum {
	gl_qrcode_data_level_l = 1,
	gl_qrcode_data_level_m,
	gl_qrcode_data_level_q,
	gl_qrcode_data_level_h
} gl_qrcode_data_level;

typedef struct gl_qrcode_data gl_qrcode_data;

typedef struct gl_qrcode_data_funcs {
	gl_object_funcs p;
	void (*set_string) (gl_qrcode_data *obj, const char *string);
	int (*render) (gl_qrcode_data *obj);
	void (*set_minimum_version) (gl_qrcode_data *obj, int version);
	void (*set_level) (gl_qrcode_data *obj, gl_qrcode_data_level level);
	unsigned int (*get_width) (gl_qrcode_data *obj);
	const unsigned char *(*get_output) (gl_qrcode_data *obj);
} gl_qrcode_data_funcs;

typedef struct gl_qrcode_data_data {
	gl_object_data p;
	
	int _version; // in qrcode lingo version indicates the size of the output image
	gl_qrcode_data_level _level; // amount of redundancy
	
	char * _string;
	unsigned char * _output;
	unsigned int _width;
} gl_qrcode_data_data;

struct gl_qrcode_data {
	gl_qrcode_data_funcs *f;
	gl_qrcode_data_data data;
};

void gl_qrcode_data_setup();
gl_qrcode_data *gl_qrcode_data_init(gl_qrcode_data *obj);
gl_qrcode_data *gl_qrcode_data_new();

#endif /* gl_qrcode_data_h */
