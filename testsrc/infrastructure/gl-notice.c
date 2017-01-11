//
//  gl-notice.c
//  Photoframe
//
//  Created by Martijn Vernooij on 10/01/2017.
//
//

#include "infrastructure/gl-notice.h"
#include "infrastructure/gl-notice-subscription.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_notice_subscribe(gl_notice *obj, gl_notice_subscription *sub);
static void gl_notice_cancel_subscription(gl_notice *obj, gl_notice_subscription *sub);
static void gl_notice_fire(gl_notice *obj);
static void gl_notice_remove_subscriptions(gl_notice *obj);
static void gl_notice_free(gl_object *obj_obj);


static struct gl_notice_funcs gl_notice_funcs_global = {
	.subscribe = &gl_notice_subscribe,
	.cancel_subscription = &gl_notice_cancel_subscription,
	.fire = &gl_notice_fire
};

static void gl_notice_subscribe(gl_notice *obj, gl_notice_subscription *sub)
{
	assert (!sub->data.owner);
	
	sub->data.owner = obj;
	
	gl_notice_subscription *head = obj->data.firstChild;
	gl_notice_subscription *last_child = head->data.siblingL;
	sub->data.siblingL = last_child;
	last_child->data.siblingR = sub;
	sub->data.siblingR = head;
	head->data.siblingL = sub;
	
	((gl_object *)sub)->f->ref((gl_object *)sub);
}

static void gl_notice_cancel_subscription(gl_notice *obj, gl_notice_subscription *sub)
{
	assert (sub->data.owner == obj);
	
	gl_notice_subscription *siblingR = sub->data.siblingR;
	gl_notice_subscription *siblingL = sub->data.siblingL;
	
	siblingR->data.siblingL = sub->data.siblingL;
	siblingL->data.siblingR = sub->data.siblingR;

	sub->data.owner = NULL;
	((gl_object *)sub)->f->unref((gl_object *)sub);
}

static void gl_notice_fire(gl_notice *obj)
{
	gl_notice_subscription *head = obj->data.firstChild;
	gl_notice_subscription *current_child = head->data.siblingR;
	gl_notice_subscription *next_child = current_child->data.siblingR;

	while (current_child != head) {
		assert (current_child->data.owner = obj);
		
		current_child->f->run_action(current_child);
		
		current_child = next_child;
		next_child = current_child->data.siblingR;
	}
	if (!obj->data.repeats) {
		gl_notice_remove_subscriptions(obj);
	}
}

static void gl_notice_remove_subscriptions(gl_notice *obj)
{
	gl_notice_subscription *head = obj->data.firstChild;
	gl_notice_subscription *current_child = head->data.siblingR;
	gl_notice_subscription *next_child = current_child->data.siblingR;

	while (current_child != head) {
		assert (current_child->data.owner = obj);
		obj->f->cancel_subscription(obj, current_child);
		
		current_child = next_child;
		next_child = current_child->data.siblingR;
	}
}

void gl_notice_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_notice_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
	
	gl_object_free_org_global = gl_notice_funcs_global.p.free;
	gl_notice_funcs_global.p.free = &gl_notice_free;
}

gl_notice *gl_notice_init(gl_notice *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_notice_funcs_global;
	
	gl_notice_subscription *head = gl_notice_subscription_new();
	obj->data.firstChild = head;
	head->data.siblingL = head;
	head->data.siblingR = head;
	
	return obj;
}

gl_notice *gl_notice_new()
{
	gl_notice *ret = calloc(1, sizeof(gl_notice));
	
	return gl_notice_init(ret);
}

static void gl_notice_free(gl_object *obj_obj)
{
	gl_notice *obj = (gl_notice *)obj_obj;
	
	gl_notice_remove_subscriptions(obj);
	
	((gl_object *)obj->data.firstChild)->f->unref((gl_object *)obj->data.firstChild);
	gl_object_free_org_global(obj_obj);
}
