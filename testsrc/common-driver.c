//
//  common-driver.c
//  Photoframe
//
//  Created by Martijn Vernooij on 05/02/2017.
//
//

#include "common-driver.h"

#include "gl-texture.h"
#include "gl-shape.h"
#include "gl-tile.h"
#include "gl-stage.h"
#include "gl-container.h"
#include "gl-container-2d.h"
#include "gl-tiled-image.h"
#include "gl-renderloop-member.h"
#include "gl-renderloop.h"
#include "gl-value-animation.h"
#include "gl-value-animation-easing.h"
#include "labels/gl-label.h"
#include "labels/gl-label-scroller-segment.h"
#include "labels/gl-label-scroller.h"
#include "gl-rectangle.h"
#include "gl-bitmap.h"
#include "infrastructure/gl-notice.h"
#include "infrastructure/gl-notice-subscription.h"
#include "infrastructure/gl-workqueue.h"
#include "infrastructure/gl-workqueue-job.h"
#include "gl-image.h"
#include "gl-framed-shape.h"
#include "slideshow/gl-slide.h"
#include "slideshow/gl-slide-image.h"
#include "slideshow/gl-slideshow.h"
#include "images/gl-bitmap-scaler.h"

#ifdef __APPLE__
#include "macosx/GLWindow-C.h"
#else
#include "egl-driver.h"
#endif


void gl_objects_setup()
{
	gl_texture_setup();
	gl_shape_setup();
	gl_tile_setup();
	gl_rectangle_setup();
	gl_stage_setup();
	gl_container_setup();
	gl_container_2d_setup();
	gl_tiled_image_setup();
	gl_renderloop_member_setup();
	gl_renderloop_setup();
	gl_notice_setup();
	gl_notice_subscription_setup();
	gl_value_animation_setup();
	gl_value_animation_easing_setup();
	gl_label_setup();
	gl_label_scroller_segment_setup();
	gl_label_scroller_setup();
	gl_bitmap_setup();
	gl_workqueue_setup();
	gl_workqueue_job_setup();
	gl_image_setup();
	gl_framed_shape_setup();
	gl_slide_setup();
	gl_slide_image_setup();
	gl_slideshow_setup();
	gl_bitmap_scaler_setup();
}

GLuint driver_load_program ( const GLchar *vertShaderSrc, const GLchar *fragShaderSrc )
{
#ifdef __APPLE__
	return gl_driver_load_program(vertShaderSrc, fragShaderSrc);
#else
	return egl_driver_load_program(vertShaderSrc, fragShaderSrc);
#endif
}

