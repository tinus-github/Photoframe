//
//  gl-workqueue-job.h
//  Photoframe
//
//  Created by Martijn Vernooij on 13/01/2017.
//
//

#ifndef gl_workqueue_job_h
#define gl_workqueue_job_h

#include <stdio.h>

#include "gl-object.h"
#include "gl-notice.h"

typedef struct gl_workqueue_job gl_workqueue_job;

typedef struct gl_workqueue_job_funcs {
	gl_object_funcs p;
	void (*run) (gl_workqueue_job *obj);
} gl_workqueue_job_funcs;

typedef struct gl_workqueue_job_data {
	gl_object_data p;
	
	void *target;
	void *(*action) (void *target, void *extraData);
	void *extraData;
	
	void *jobReturn;
	
	gl_workqueue_job *siblingL;
	gl_workqueue_job *siblingR;
	
	gl_notice *doneNotice;
} gl_workqueue_job_data;

struct gl_workqueue_job {
	gl_workqueue_job_funcs *f;
	gl_workqueue_job_data data;
};

void gl_workqueue_job_setup();
gl_workqueue_job *gl_workqueue_job_init(gl_workqueue_job *obj);
gl_workqueue_job *gl_workqueue_job_new();


#endif /* gl_workqueue_job_h */
