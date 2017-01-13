//
//  gl-workqueue.c
//  Photoframe
//
//  Created by Martijn Vernooij on 13/01/2017.
//
//

#include "infrastructure/gl-workqueue.h"
#include "infrastructure/gl-workqueue-job.h"

static struct gl_workqueue_funcs gl_workqueue_funcs_global = {
	.append_job = &gl_workqueue_append_job
};

void gl_workqueue_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_workqueue_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
}

static gl_workqueue *gl_workqueue_init(gl_workqueue *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_workqueue_funcs_global;
	
	
	pthread_mutex_init(&obj->data.queueMutex, NULL);
	pthread_cond_init(&obj->data.workAvailable);
	
	gl_workqueue_job *head = gl_workqueue_job_new();
	head->data.siblingL = head;
	head->data.siblingR = head;
	obj->data.queuedJobs = head;
	
	head = gl_workqueue_job_new();
	head->data.siblingL = head;
	head->data.siblingR = head;
	obj->data.doneJobs = head;
	
	return obj;
}

gl_workqueue *gl_workqueue_new()
{
	gl_workqueue *ret = calloc(1, sizeof(gl_workqueue));
	
	return gl_workqueue_init(ret);
}
