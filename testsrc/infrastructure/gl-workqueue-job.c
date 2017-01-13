//
//  gl-workqueue-job.c
//  Photoframe
//
//  Created by Martijn Vernooij on 13/01/2017.
//
//

#include "gl-workqueue-job.h"
#include <stdlib.h>
#include <string.h>

static void gl_workqueue_job_run(gl_workqueue_job *obj);
static void gl_workqueue_job_free(gl_object *obj_obj);

static struct gl_workqueue_job_funcs gl_workqueue_job_funcs_global = {
	.run = &gl_workqueue_job_run
};

static void (*gl_object_free_org_global) (gl_object *obj);

void gl_workqueue_job_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_workqueue_job_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	parent->f->free(parent);
	
	gl_object_free_org_global = gl_workqueue_job_funcs_global.p.free;
	gl_workqueue_job_funcs_global.p.free = &gl_workqueue_job_free;

}

static void gl_workqueue_job_run(gl_workqueue_job *obj)
{
	obj->data.jobReturn = obj->data.action(target, extra_data);
}

static gl_workqueue_job *gl_workqueue_job_init(gl_workqueue_job *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_workqueue_job_funcs_global;
	
	obj->data.doneNotice = gl_notice_new();
	
	return obj;
}

gl_workqueue_job *gl_workqueue_job_new()
{
	gl_workqueue_job *ret = calloc(1, sizeof(gl_workqueue_job));
	
	return gl_workqueue_job_init(ret);
}

static void gl_workqueue_job_free(gl_object *obj_obj)
{
	gl_workqueue_job *obj = (gl_workqueue_job *)obj_obj;
	
	((gl_object *)obj->data.doneNotice)->f->unref((gl_object *)obj->data.doneNotice);
	gl_object_free_org_global(obj_obj);
}
