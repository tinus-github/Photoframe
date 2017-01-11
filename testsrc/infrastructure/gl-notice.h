//
//  gl-notice.h
//  Photoframe
//
//  Created by Martijn Vernooij on 10/01/2017.
//
//

#ifndef gl_notice_h
#define gl_notice_h

#include "infrastructure/gl-object.h"
#include <stdio.h>

typedef struct gl_notice gl_notice;
typedef struct gl_notice_subscription gl_notice_subscription;

typedef struct gl_notice_funcs {
	gl_object_funcs p;
	void (*subscribe) (gl_notice *obj, gl_notice_subscription *subscription);
	void (*cancel_subscription) (gl_notice *obj, gl_notice_subscription *subscription);
	void (*fire) (gl_notice *obj);
} gl_notice_funcs;

typedef struct gl_notice_data {
	gl_object_data p;
	
	gl_notice_subscription *firstChild;
	
	int repeats;
} gl_notice_data;

struct gl_notice {
	gl_notice_funcs *f;
	gl_notice_data data;
};

void gl_notice_setup();
gl_notice *gl_notice_init(gl_notice *obj);
gl_notice *gl_notice_new();


#endif /* gl_notice_h */
