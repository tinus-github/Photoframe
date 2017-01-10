//
//  gl-renderloop-member.h
//  Photoframe
//
//  Created by Martijn Vernooij on 27/12/2016.
//
//

#ifndef gl_renderloop_member_h
#define gl_renderloop_member_h

#include <stdio.h>
#include "gl-renderloop.h"
#include "infrastructure/gl_object.h"

typedef struct gl_renderloop_member gl_renderloop_member;

typedef struct gl_renderloop_member_funcs {
	gl_object_funcs p;
	void (*run_action) (gl_renderloop_member *obj);
} gl_renderloop_member_funcs;

typedef enum {
	gl_renderloop_member_priority_low = 0,
	gl_renderloop_member_priority_high
} gl_renderloop_member_priority;

typedef struct gl_renderloop_member_data {
	gl_object_data p;
	
	gl_renderloop_member *siblingL;
	gl_renderloop_member *siblingR;
	gl_renderloop_phase renderloopPhase;
	gl_renderloop *owner;

	/* Note that removing this member during the action should work, but not removing other members */
	void (*action) (void *target, gl_renderloop_member *renderloop_member, void *action_data);
	void *target;
	void *action_data;
	
	gl_renderloop_member_priority loadPriority; // Only used for the load phase
} gl_renderloop_member_data;

struct gl_renderloop_member {
	gl_renderloop_member_funcs *f;
	gl_renderloop_member_data data;
};

void gl_renderloop_member_setup();
gl_renderloop_member *gl_renderloop_member_init(gl_renderloop_member *obj);
gl_renderloop_member *gl_renderloop_member_new();

#endif /* gl_renderloop_member_h */
