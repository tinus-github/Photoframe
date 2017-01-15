
/*
 * code stolen from openGL-RPi-tutorial-master/encode_OGL/
 * and from OpenGL® ES 2.0 Programming Guide
 */

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <bcm_host.h>

#include "loadimage.h"
#include "gl-texture.h"
#include "gl-tile.h"
#include "gl-container-2d.h"
#include "gl-stage.h"
#include "gl-tiled-image.h"
#include "gl-renderloop.h"
#include "egl-driver.h"
#include "gl-value-animation.h"
#include "gl-value-animation-easing.h"
#include "labels/gl-label-scroller.h"
#include "infrastructure/gl-workqueue.h"
#include "infrastructure/gl-workqueue-job.h"
#include "infrastructure/gl-notice-subscription.h"

#include "../lib/linmath/linmath.h"

// from esUtil.h
#define TRUE 1
#define FALSE 0

struct render_data {
	const char *filename;
	
	unsigned char *image;
	int width;
	int height;
	
	unsigned int orientation;
};

struct image_display_data {
	gl_container_2d *container_2d;
};

void *image_render_job(void *target, void *extra_data)
{
	struct render_data *renderDataP = (struct render_data *)target;
	
	renderDataP->image = loadJPEG(renderDataP->filename,
				      1920,1080,
				      &renderDataP->width,
				      &renderDataP->height,
				      &renderDataP->orientation);
	
	fprintf(stderr, "Image is %d x %d\n", renderDataP->width, renderDataP->height);
	return NULL;
}

void use_image(void *target, gl_notice_subscription *subscription, void *extra_data)
{
	struct image_display_data *displayDataP = (struct image_display_data *)target;
	struct render_data *renderDataP = (struct render_data *)extra_data;

	gl_container *main_container_2d_container = (gl_container *)displayDataP->container_2d;
	
	gl_tiled_image *tiled_image = gl_tiled_image_new();
	
	tiled_image->f->load_image(tiled_image, renderDataP->image,
				   renderDataP->width, renderDataP->height,
				   renderDataP->orientation, 128);
	main_container_2d_container->f->append_child(main_container_2d_container, (gl_shape *)tiled_image);
}

int main(int argc, char *argv[])
{
	struct render_data renderData;
	struct image_display_data displayData;
	
	int width, height;
	unsigned int orientation;
	
	struct timeval t1, t2;
	struct timezone tz;
	float deltatime;
	
	unsigned char *image;
	
	//image = esLoadTGA("jan.tga", &width, &height);
	gettimeofday ( &t1 , &tz );
	
	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return -1;
	}
	
	egl_driver_setup();
	egl_driver_init();
	
	renderData.filename = argv[1];
	
	gl_workqueue *workqueue = gl_workqueue_new();
	workqueue->f->start(workqueue);
	
	gl_workqueue_job *job = gl_workqueue_job_new();
	job->data.target = &renderData;
	job->data.action = &image_render_job;
	
	workqueue->f->append_job(workqueue, job);
	
	
#if 0
	if (image == NULL) {
		fprintf(stderr, "No such image\n");
		exit(1);
	}
	fprintf(stderr, "Image is %d x %d\n", width, height);
	gettimeofday(&t2, &tz);
	deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
	
	printf("Image loaded in %1.4f seconds\n", deltatime);
#endif
	
//	gl_tiled_image *tiled_image = gl_tiled_image_new();
	gl_container_2d *main_container_2d = gl_container_2d_new();
	gl_container *main_container_2d_container = (gl_container *)main_container_2d;
	gl_shape *main_container_2d_shape = (gl_shape *)main_container_2d;

//	tiled_image->f->load_image(tiled_image, image, width, height, orientation, 128);
//	main_container_2d_container->f->append_child(main_container_2d_container, (gl_shape *)tiled_image);
	
	gl_label_scroller *scroller = gl_label_scroller_new();
	gl_shape *scroller_shape = (gl_shape *)scroller;
	scroller_shape->data.objectHeight = 160;
	scroller_shape->data.objectWidth = 1920;
	scroller->data.text = "AVAVAVABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	scroller->f->start(scroller);
	main_container_2d_container->f->append_child(main_container_2d_container, scroller_shape);

	main_container_2d_shape->data.objectX = 0.0;
	main_container_2d->data.scaleH = 1.0;
	main_container_2d->data.scaleV = 1.0;
	
	gl_stage *global_stage = gl_stage_get_global_stage();
	global_stage->f->set_shape(global_stage, (gl_shape *)main_container_2d);

	displayData.container_2d = main_container_2d;
	gl_notice_subscription *sub = gl_notice_subscription_new();
	sub->data.target = &displayData;
	sub->data.action_data = &renderData;
	sub->data.action = &use_image;
	job->data.doneNotice->f->subscribe(job->data.doneNotice, sub);
	
	workqueue->f->append_job(workqueue, job);
	
	gl_renderloop_loop();
	
	return 0; // not reached
}
