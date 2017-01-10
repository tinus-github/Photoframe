//
//  gl-notice-subscription.c
//  Photoframe
//
//  Created by Martijn Vernooij on 10/01/2017.
//
//

#include "infrastructure/gl-notice-subscription.h"

static void gl_notice_subscription_run_action(gl_notice_subscription *obj);

static struct gl_notice_subscription_funcs gl_notice_subscription_funcs_global = {
	.run_action = &gl_notice_subscription_run_action
};

// doesn't matter if this is called multiple times
void gl_notice_subscription_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_notice_subscription_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

gl_notice_subscription *gl_notice_subscription_init(gl_notice_subscription *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_notice_subscription_funcs_global;
	
	return obj;
}

gl_notice_subscription *gl_notice_subscription_new()
{
	gl_notice_subscription *ret = calloc(1, sizeof(gl_notice_subscription));
	
	return gl_notice_subscription_init(ret);
}

static void gl_notice_subscription_run_action(gl_notice_subscription *obj)
{
	obj->data.action(obj->data.target, obj, obj->data.action_data);
}
