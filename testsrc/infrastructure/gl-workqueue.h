//
//  gl-workqueue.h
//  Photoframe
//
//  Created by Martijn Vernooij on 13/01/2017.
//
//

#ifndef gl_workqueue_h
#define gl_workqueue_h

#include <stdio.h>
#include <pthread.h>

#include "infrastructure/gl-object.h"

typedef struct gl_workqueue gl_workqueue;
typedef struct gl_workqueue_job gl_workqueue_job;

typedef struct gl_workqueue_funcs {
	gl_object_funcs p;
	void (*append_job) (gl_workqueue *obj, gl_workqueue_job *child);
	gl_workqueue *(start) (gl_workqueue *obj);
} gl_workqueue_funcs;

typedef struct gl_workqueue_data {
	gl_object_data p;
	
	int schedulingPolicy;
	
	pthread_mutex_t queueMutex;
	
	pthread_cond_t workAvailable;
	
	gl_workqueue_job *queuedJobs;
	gl_workqueue_job *doneJobs;
} gl_workqueue_data;

struct gl_workqueue {
	gl_workqueue_funcs *f;
	gl_workqueue_data data;
};

void gl_workqueue_setup();
gl_workqueue *gl_workqueue_init(gl_workqueue *obj);
gl_workqueue *gl_workqueue_new();

#endif /* gl_workqueue_h */
