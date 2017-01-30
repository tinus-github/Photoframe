//
//  gl-label-scroller-segment.c
//  Photoframe
//
//  Created by Martijn Vernooij on 06/01/2017.
//
//

#include "labels/gl-label-scroller-segment.h"

#include "gl-renderloop-member.h"
#include "gl-renderloop.h"
#include "gl-notice.h"
#include "gl-notice-subscription.h"

#define SEGMENT_WIDTH 512

static void gl_label_scroller_segment_free(gl_object *obj);
static void gl_label_scroller_segment_render(gl_label_scroller_segment *obj);
static void gl_label_scroller_segment_layout(gl_label_scroller_segment *obj);
gl_label_scroller_segment *gl_label_scroller_segment_dup(gl_label_scroller_segment *obj);

static gl_label_scroller_segment_child_data *gl_label_scroller_segment_append_child(
										    gl_label_scroller_segment *obj, gl_tile *tile,
										    uint32_t tile_index);
static void gl_label_scroller_segment_remove_child(
						    gl_label_scroller_segment *obj,
						    gl_label_scroller_segment_child_data *childData);

static struct gl_label_scroller_segment_funcs gl_label_scroller_segment_funcs_global = {
	.render = &gl_label_scroller_segment_render,
	.layout = &gl_label_scroller_segment_layout,
	.dup = &gl_label_scroller_segment_dup
};

static void (*gl_object_free_org_global) (gl_object *obj);

static void gl_label_scroller_segment_apply_outline(void *target, gl_renderloop_member *renderloop_member, void *extra_data)
{
	gl_label_scroller_segment *obj = (gl_label_scroller_segment *)target;
	gl_tile *tile = (gl_tile *)extra_data;
	
	gl_texture *texture = tile->data.texture;
	texture->f->apply_outline(texture);
	
	((gl_object *)renderloop_member)->f->unref((gl_object *)renderloop_member);
	((gl_object *)tile)->f->unref((gl_object *)tile);
}

static void gl_label_scroller_segment_prepare_outline(gl_label_scroller_segment *obj, gl_tile *tile)
{
	gl_renderloop_member *renderloop_member = gl_renderloop_member_new();
	renderloop_member->data.target = obj;
	renderloop_member->data.action = &gl_label_scroller_segment_apply_outline;
	renderloop_member->data.action_data = tile;
	renderloop_member->data.loadPriority = gl_renderloop_member_priority_high;
	
	gl_renderloop *renderloop = gl_renderloop_get_global_renderloop();
	renderloop->f->append_child(renderloop, gl_renderloop_phase_load, renderloop_member);
	((gl_object *)renderloop_member)->f->ref((gl_object *)renderloop_member);
	((gl_object *)tile)->f->ref((gl_object *)tile);
}

static void gl_label_scroller_segment_tile_loaded(void *target,
						  gl_notice_subscription *subscription,
						  void *data)
{
	gl_label_scroller_segment *obj = (gl_label_scroller_segment *)target;
	gl_tile *tile = (gl_tile *)data;
	
	gl_label_scroller_segment_prepare_outline(obj, tile);
}

static gl_tile *gl_label_scroller_segment_render_tile(gl_label_scroller_segment *obj, uint32_t tile_index)
{
	gl_label_renderer *renderer = obj->data.renderer;
	gl_tile *tile;
	gl_shape *tile_shape;
	
	tile = renderer->f->render(renderer,
				   tile_index * SEGMENT_WIDTH, 0,
				   SEGMENT_WIDTH,
				   obj->data.height);
	tile_shape = (gl_shape *)tile;
	tile_shape->data.objectX = tile_index * SEGMENT_WIDTH;
	
	gl_texture *texture = tile->data.texture;
	if (texture->data.loadState == gl_texture_loadstate_done) {
		gl_label_scroller_segment_prepare_outline(obj, tile);
	} else {
		gl_notice_subscription *subscription = gl_notice_subscription_new();
		subscription->data.action = &gl_label_scroller_segment_tile_loaded;
		subscription->data.target = obj;
		subscription->data.action_data = tile;
		
		texture->data.loadedNotice->f->subscribe(texture->data.loadedNotice, subscription);
	}
	
	return tile;
}

static void gl_label_scroller_segment_render(gl_label_scroller_segment *obj)
{
	gl_label_renderer *renderer = obj->data.renderer;

	int32_t tile_index = obj->data.exposedSectionLeft / SEGMENT_WIDTH;
	int32_t final_tile_index = (obj->data.exposedSectionLeft + obj->data.exposedSectionWidth) / SEGMENT_WIDTH;
	
	if (tile_index < 0) {
		tile_index = 0;
	}
	
	int32_t counter;
	
	gl_tile *tile;
	gl_label_scroller_segment_child_data *currentChildData = obj->data.childDataHead;
	currentChildData = currentChildData->siblingR;
	gl_label_scroller_segment_child_data *nextChildData = currentChildData->siblingR;
	int found;
	
	for (counter = tile_index; counter <= final_tile_index; counter++) {
		if ((counter * SEGMENT_WIDTH) > renderer->data.totalWidth) {
			break;
		}
		found = 0;
		
		while (currentChildData != obj->data.childDataHead) {
			if (currentChildData->tileIndex == counter) {
				currentChildData = nextChildData;
				nextChildData = currentChildData->siblingR;
				found = 1;
				break;
			}
			gl_label_scroller_segment_remove_child(obj, currentChildData);
			currentChildData = nextChildData;
			nextChildData = currentChildData->siblingR;
		}
		if (!found) {
			tile = gl_label_scroller_segment_render_tile(obj, counter);
			gl_label_scroller_segment_append_child(obj, tile, counter);
		}
	}
}

static void gl_label_scroller_segment_layout(gl_label_scroller_segment *obj)
{
	gl_label_renderer *renderer = obj->data.renderer;
	renderer->data.text = obj->data.text;
	
	renderer->f->layout(renderer);
	
	obj->data.textWidth = renderer->data.totalWidth;
}

static gl_label_scroller_segment_child_data *gl_label_scroller_segment_append_child(
										    gl_label_scroller_segment *obj, gl_tile *tile,
										    uint32_t tile_index)
{
	gl_label_scroller_segment_child_data *childData = calloc(1, sizeof(gl_label_scroller_segment_child_data));
	childData->tileIndex = tile_index;
	childData->tile = tile;
	gl_label_scroller_segment_child_data *head = obj->data.childDataHead;
	childData->siblingL = head->siblingL;
	head->siblingL->siblingR = childData;
	head->siblingL = childData;
	childData->siblingR = head;
	
	gl_container *obj_container = (gl_container *)obj;
	obj_container->f->append_child(obj_container, (gl_shape *)tile);
	
	return childData;
}

static void gl_label_scroller_segment_remove_child(
						    gl_label_scroller_segment *obj,
						    gl_label_scroller_segment_child_data *childData) {
	gl_label_scroller_segment_child_data *siblingL;
	gl_label_scroller_segment_child_data *siblingR;
	
	siblingL = childData->siblingL;
	siblingR = childData->siblingR;
	siblingL->siblingR = childData->siblingR;
	siblingR->siblingL = childData->siblingL;
	
	gl_shape *tile_shape = (gl_shape *)childData->tile;
	gl_container *container = tile_shape->data.container;
	container->f->remove_child(container, tile_shape);
	free (childData);
}

void gl_label_scroller_segment_setup()
{
	gl_container_2d *parent = gl_container_2d_new();
	gl_object *parent_obj = (gl_object *)parent;
	memcpy(&gl_label_scroller_segment_funcs_global.p, parent->f, sizeof(gl_container_2d_funcs));
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_label_scroller_segment_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_label_scroller_segment_free;
	
	parent_obj->f->free(parent_obj);
}

gl_label_scroller_segment *gl_label_scroller_segment_init(gl_label_scroller_segment *obj)
{
	gl_container_2d_init((gl_container_2d *)obj);
	
	obj->f = &gl_label_scroller_segment_funcs_global;
	
	gl_label_scroller_segment_child_data *data_head = calloc(1, sizeof(gl_label_scroller_segment_child_data));
	data_head->siblingL = data_head;
	data_head->siblingR = data_head;
	obj->data.childDataHead = data_head;
	
	return obj;
}

gl_label_scroller_segment *gl_label_scroller_segment_dup(gl_label_scroller_segment *obj)
{
	gl_label_scroller_segment *ret = gl_label_scroller_segment_new();
	
	if (obj->data.text) {
		ret->data.text = strdup(obj->data.text);
	}
	ret->data.width = obj->data.width;
	ret->data.height = obj->data.height;
	ret->data.textWidth = obj->data.textWidth;
	ret->data.exposedSectionLeft = obj->data.exposedSectionLeft;
	ret->data.exposedSectionWidth = obj->data.exposedSectionWidth;
	
	gl_object *renderer_obj = (gl_object *)ret->data.renderer;
	renderer_obj->f->unref(renderer_obj);
	ret->data.renderer = obj->data.renderer;
	renderer_obj = (gl_object *)ret->data.renderer;
	renderer_obj->f->ref(renderer_obj);
	
	return ret;
}

gl_label_scroller_segment *gl_label_scroller_segment_new()
{
	gl_label_scroller_segment *ret = calloc(1, sizeof(gl_label_scroller_segment));
	
	ret->data.renderer = gl_label_renderer_new();
	
	return gl_label_scroller_segment_init(ret);
}

static void gl_label_scroller_segment_free(gl_object *obj_obj)
{
	gl_label_scroller_segment *obj = (gl_label_scroller_segment *)obj_obj;
	free (obj->data.text);
	
	gl_object *renderer_obj = (gl_object *)obj->data.renderer;
	renderer_obj->f->unref(renderer_obj);
	
	gl_label_scroller_segment_child_data *dataPtr = obj->data.childDataHead;
	gl_label_scroller_segment_child_data *dataPtrNext;
	do {
		dataPtrNext = dataPtr->siblingR;
		free(dataPtr);
		dataPtr = dataPtrNext;
	} while (dataPtr != obj->data.childDataHead);
	
	gl_object_free_org_global(obj_obj);
}
