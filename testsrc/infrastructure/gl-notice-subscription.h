//
//  gl-notice-subscription.h
//  Photoframe
//
//  Created by Martijn Vernooij on 10/01/2017.
//
//

#ifndef gl_notice_subscription_h
#define gl_notice_subscription_h

#include <stdio.h>
#include "infrastructure/gl-notice.h"

typedef struct gl_notice_subscription gl_notice_subscription;

typedef struct gl_notice_subscription_funcs {
	gl_object_funcs p;
	void (*run_action) (gl_notice_subscription *obj);
} gl_notice_subscription_funcs;

typedef struct gl_notice_subscription_data {
	gl_object_data p;
	
	gl_notice_subscription *siblingL;
	gl_notice_subscription *siblingR;
	gl_notice *owner;
	
	/* Note that removing this member during the action should work, but not removing other members */
	void (*action) (void *target, gl_notice_subscription *notice_subscription, void *action_data);
	void *target;
	void *action_data;
} gl_notice_subscription_data;

struct gl_notice_subscription {
	gl_notice_subscription_funcs *f;
	gl_notice_subscription_data data;
};

void gl_notice_subscription_setup();
gl_notice_subscription *gl_notice_subscription_init(gl_notice_subscription *obj);
gl_notice_subscription *gl_notice_subscription_new();

#endif /* gl_notice_subscription_h */
