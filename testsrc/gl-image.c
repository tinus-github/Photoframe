//
//  gl-image.c
//  Photoframe
//
//  Created by Martijn Vernooij on 16/01/2017.
//
//

#include "gl-image.h"
#include <string.h>
#include <stdlib.h>

#include "infrastructure/gl-workqueue-job.h"
#include "infrastructure/gl-workqueue.h"
#include "gl-stage.h"
#include "images/loadimage.h"
#include "images/loadimage-jpg.h"
#include "images/loadimage-png.h"
#include "images/loadimage-bmp.h"
#include "fs/gl-stream.h"
#include "fs/gl-stream-rewindable.h"
#include "infrastructure/gl-notice-subscription.h"
#include "infrastructure/gl-notice.h"

#define GL_IMAGE_TILE_HEIGHT 128

static void gl_image_free(gl_object *obj_obj);
static void gl_image_load_file(gl_image *obj, const char* filename);

static struct gl_image_funcs gl_image_funcs_global = {
	.load_file = &gl_image_load_file
};

static void (*gl_object_free_org_global) (gl_object *obj);
static void *gl_image_render_job(void *target, void *extra_data);
static void gl_image_loading_completed(void *target, gl_notice_subscription *subscription, void *extra_data);
static void gl_image_transfer_completed(void *target, gl_notice_subscription *subscription, void *extra_data);

static int gl_image_setup_done = 0;

static gl_workqueue *loading_workqueue;

static void gl_image_setup_workqueue()
{
	loading_workqueue = gl_workqueue_new();
	loading_workqueue->f->start(loading_workqueue);
}

void gl_image_setup()
{
	if (gl_image_setup_done) {
		return;
	}
	gl_tiled_image *parent = gl_tiled_image_new();
	memcpy(&gl_image_funcs_global.p, parent->f, sizeof(gl_tiled_image_funcs));
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_image_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_image_free;
	
	((gl_object *)parent)->f->free((gl_object *)parent);
	
	gl_image_setup_workqueue();
	
	gl_image_setup_done = 1;
}

static void gl_image_load_file(gl_image *obj, const char* urlString)
{
	((gl_object *)obj)->f->ref((gl_object *)obj);
	obj->data._urlString = strdup(urlString);
	
	gl_workqueue_job *job = gl_workqueue_job_new();
	job->data.target = obj;
	job->data.action = &gl_image_render_job;
	
	gl_notice_subscription *sub = gl_notice_subscription_new();
	sub->data.target = obj;
	sub->data.action = gl_image_loading_completed;
	sub->data.action_data = job;
	((gl_object *)job)->f->ref((gl_object *)job);
	
	job->data.doneNotice->f->subscribe(job->data.doneNotice, sub);
	
	loading_workqueue->f->append_job(loading_workqueue, job);
}

#define SIGNATURE_SIZE 11

static void *gl_image_render_job(void *target, void *extra_data)
{
	gl_image *obj = (gl_image *)target;
	
	gl_stream *stream = (gl_stream *)gl_stream_new_for_url(obj->data._urlString);
	if (!stream) {
		return NULL;
	}
	
	gl_stream_rewindable *stream_rewindable = gl_stream_rewindable_new();
	stream_rewindable->f->set_stream(stream_rewindable, stream);
	((gl_stream *)stream_rewindable)->f->open((gl_stream *)stream_rewindable);
	
	unsigned char signature[SIGNATURE_SIZE];
	size_t num_read = ((gl_stream *)stream_rewindable)->f->read((gl_stream *)stream_rewindable, signature, SIGNATURE_SIZE);
	
	printf("Loading file %s\n", obj->data._urlString);
	
	if (num_read != SIGNATURE_SIZE) {
		((gl_object *)stream_rewindable)->f->unref((gl_object *)stream_rewindable);
		return NULL;
	}
	
	loadImageFunction f = functionForLoadingImage(signature);
	if (!f) {
		((gl_object *)stream_rewindable)->f->unref((gl_object *)stream_rewindable);
		return NULL;
	}
	
	stream_rewindable->f->rewind(stream_rewindable, signature, SIGNATURE_SIZE);
	
	void *ret = f((gl_stream *)stream_rewindable,
			    obj->data.desiredWidth,
			    obj->data.desiredHeight,
			    &obj->data._width,
			    &obj->data._height,
			    &obj->data._orientation
			    );
	
	((gl_object *)stream_rewindable)->f->unref((gl_object *)stream_rewindable);
	return ret;
}

static void gl_image_loading_completed(void *target, gl_notice_subscription *subscription, void *extra_data)
{
	gl_image *obj = (gl_image *)target;
	gl_tiled_image *obj_tiled = (gl_tiled_image *)obj;
	gl_workqueue_job *job = (gl_workqueue_job *)extra_data;
	
	free(obj->data._urlString);
	obj->data._urlString = NULL;
	
	unsigned char* bitmap = job->data.jobReturn;
	((gl_object *)job)->f->unref((gl_object *)job);
	
	if (bitmap) {
		gl_notice_subscription *sub = gl_notice_subscription_new();
		sub->data.target = obj;
		sub->data.action = &gl_image_transfer_completed;
		obj_tiled->data.loadedNotice->f->subscribe(obj_tiled->data.loadedNotice, sub);
	
		obj_tiled->f->load_image(obj_tiled, bitmap,
					 obj->data._width, obj->data._height,
					 obj->data._orientation,
					 GL_IMAGE_TILE_HEIGHT);
	} else {
		obj->data.failedNotice->f->fire(obj->data.failedNotice);
		((gl_object *)obj)->f->unref((gl_object *)obj);
	}
}

static void gl_image_transfer_completed(void *target, gl_notice_subscription *subscription, void *extra_data)
{
	gl_image *obj = (gl_image *)target;
	
	obj->data.readyNotice->f->fire(obj->data.readyNotice);
	((gl_object *)obj)->f->unref((gl_object *)obj);
}

gl_image *gl_image_init(gl_image *obj)
{
	gl_tiled_image_init((gl_tiled_image *)obj);
	
	obj->f = &gl_image_funcs_global;
	
	obj->data.readyNotice = gl_notice_new();
	obj->data.failedNotice = gl_notice_new();
	
	gl_stage *main_stage = gl_stage_get_global_stage();
	obj->data.desiredWidth = main_stage->data.width;
	obj->data.desiredHeight = main_stage->data.height;
	
	return obj;
}

gl_image *gl_image_new()
{
	gl_image *ret = calloc(1, sizeof(gl_image));
	
	return gl_image_init(ret);
}

static void gl_image_free(gl_object *obj_obj)
{
	gl_image *obj = (gl_image *)obj_obj;
	
	free(obj->data._urlString);
	obj->data._urlString = NULL;
	
	((gl_object *)obj->data.readyNotice)->f->unref((gl_object *)obj->data.readyNotice);
	((gl_object *)obj->data.failedNotice)->f->unref((gl_object *)obj->data.failedNotice);
	
	gl_object_free_org_global(obj_obj);
}
