//
//  gl-label.c
//  Photoframe
//
//  Created by Martijn Vernooij on 02/01/2017.
//
//

#include "gl-label.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <assert.h>
#include <string.h>
#include <hb.h>

#include "gl-texture.h"

#define FONT_FILE "/home/pi/lib/share/font/DejaVuSerif.ttf"
#define LABEL_HEIGHT 64

static void gl_label_free(gl_object *obj);
static void gl_label_render(gl_label *obj);
static void gl_label_layout(gl_label *obj);

static struct gl_label_funcs gl_label_funcs_global = {
	.render = &gl_label_render
};

struct rendering_data {
	FT_Library library;
	FT_Face face;
	
	hb_language_t hb_language;
	hb_font_t *hb_font;
};

typedef struct gl_label_rect {
	int x;
	int y;
	int width;
	int height;
} gl_label_rect;

struct rendering_data global_rendering_data;

static void (*gl_object_free_org_global) (gl_object *obj);

/* returns 0 if the rect is empty */
static unsigned int gl_label_clip_rect(gl_label_rect *rect, gl_label_rect *mask)
{
	unsigned int difference;
	int ret = 1;
	if ((rect->x >= (mask->x + mask->width)) ||
	    (rect->y >= (mask->y + mask->height)) ||
	    ((rect->x + rect->width) <= mask->x) ||
	    ((rect->y + rect->height) <= mask->y)) {
		ret = 0;
	}
	
	if (rect->x < mask->x) {
		difference = mask->x - rect->x;
		rect->x += difference;
		rect->width -= difference;
	}
	if (rect->y < mask->y) {
		difference = mask->y - rect->y;
		rect->y += difference;
		rect->width -= difference;
	}
	
	if ((rect->x + rect->width) > (mask->x + mask->width)) {
		difference = rect->x + rect->width - (mask->x + mask->width);
		rect->width -= difference;
	}
	if ((rect->y + rect->height) > (mask->y + mask->height)) {
		difference = rect->y + rect->height - (mask->y + mask->height);
		rect->height -= difference;
	}
	
	return ret;
}

static void gl_label_blit(unsigned char *dest, gl_label_rect *dest_rect,
			  unsigned char *src, gl_label_rect *src_rect,
			  int offset_x, int offset_y)
{
	gl_label_rect dst_clipped_rect_stack;
	
	gl_label_rect *dst_clipped_rect = &dst_clipped_rect_stack;
	memcpy(dst_clipped_rect, src_rect, sizeof(gl_label_rect));
	
	dst_clipped_rect->x += offset_x;
	dst_clipped_rect->y += offset_y;
	
	unsigned int work_to_do = gl_label_clip_rect(dst_clipped_rect, dest_rect);
	if (!work_to_do) {
		return;
	}
	
	int src_offset_x = (dst_clipped_rect->x - offset_x) - src_rect->x;
	int src_offset_y = (dst_clipped_rect->y - offset_y) - src_rect->y;
	
	int x2 = dst_clipped_rect->x + dst_clipped_rect->width;
	int y2 = dst_clipped_rect->y + dst_clipped_rect->height;
	
	int counter_x;
	int counter_y;
	int this_dst_x;
	int this_dst_y;
	int this_src_x;
	int this_src_y;
	int this_dst_index;
	int this_src_index;
	
	for (counter_y = dst_clipped_rect->y; counter_y < y2; counter_y++) {
		this_dst_y = counter_y - dest_rect->y;
		this_src_y = (counter_y - src_offset_y) - src_rect->y;
		for (counter_x = dst_clipped_rect->x; counter_x < x2; counter_x++) {
			this_dst_x = counter_x - dest_rect->x;
			this_src_x = (counter_x - src_offset_x) - src_rect->x;
			this_dst_index = this_dst_x + (dest_rect->width * this_dst_y);
			this_src_index = this_src_x + (src_rect->width * this_src_y);
			switch (src[this_src_index]) {
				case 0:
					break;
				case 255:
					dest[this_dst_index] = 255;
					break;
				default:
					dest[this_dst_index] = src[this_src_index] +
								(((255 - src[this_src_index]) * dest[this_dst_index]) / 255);
			}
		}
	}
}

static void gl_label_render_character(gl_label *obj, uint32_t codepoint, int32_t x, int32_t y, unsigned char* bitmap)
{
	FT_Error errorret;
	
	unsigned int glyph_index = FT_Get_Char_Index(global_rendering_data.face, codepoint);
	assert (!(errorret = FT_Load_Glyph(global_rendering_data.face, glyph_index, FT_LOAD_DEFAULT)));
	
	FT_GlyphSlot glyph = global_rendering_data.face->glyph;
	if (glyph->format != FT_GLYPH_FORMAT_BITMAP) {
		assert (!(errorret = FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL)));
	}
	
	
	gl_label_rect dst_rect_stack;
	gl_label_rect *dst_rect = &dst_rect_stack;
	dst_rect_stack.x = obj->data.windowX;
	dst_rect_stack.y = obj->data.windowY;
	dst_rect_stack.width = obj->data.windowWidth;
	dst_rect_stack.height = obj->data.windowHeight;
	
	gl_label_rect src_rect_stack;
	gl_label_rect *src_rect = &src_rect_stack;
	src_rect_stack.x = glyph->bitmap_left;
	src_rect_stack.y = glyph->bitmap_top;
	src_rect_stack.width = glyph->bitmap.width;
	src_rect_stack.height = glyph->bitmap.rows;
	
	gl_label_blit(bitmap, dst_rect, glyph->bitmap.buffer, src_rect, x, y);
}

static void gl_label_render(gl_label *obj)
{
	gl_label_layout(obj);

	unsigned char *bitmap = calloc(1, obj->data.windowWidth * obj->data.windowHeight);
	
	uint32_t counter;
	for (counter = 0; counter < obj->data.numGlyphs; counter++) {
		gl_label_glyph_data *glyphdata = obj->data.glyphData[counter];
		gl_label_render_character(obj, glyphdata->codepoint, glyphdata->x, glyphdata->y, bitmap)
	}
	
	gl_texture *texture = gl_texture_new();
	texture->f->load_image_monochrome(texture, bitmap, obj->data.windowWidth, obj->data.windowHeight);
	
	texture->data.dataType = gl_texture_data_type_alpha;
	
	obj->data.tile = gl_tile_new();
	gl_tile *tile  = obj->data.tile;
	tile->f->set_texture(tile, texture);
}

static void gl_label_layout(gl_label *obj)
{
	hb_buffer_t *buf = hb_buffer_create();
	hb_buffer_add_utf8(buf, text, strlen(obj->data.text), 0, strlen(obj->data.text));
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_language(global_rendering_data.hb_language);
	
	hb_shape(global_rendering_data.hb_font,
		 buf,
		 NULL,
		 0);
	
	hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &obj->data.numGlyphs);
	hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &obj->data.numGlyphs);
	
	uint32_t counter;
	int32_t cursurX = 0;
	int32_t cursorY = 0;
	obj->data.glyphData = calloc(obj->data.numGlyphs, sizeof(gl_label_glyph_data));)
	
	for(counter = 0; counter < obj->data.numGlyphs; counter++) {
		obj->data.glyphData[counter].x = cursorX + glyph_pos[counter].x_offset;
		obj->data.glyphData[counter].y = cursorY + glyph_pos[counter].y_offset;
		cursorX += glyph_pos[counter].x_advance;
		cursorY += glyph_pos[counter].y_advance;
		
		obj->data.glyphData[counter].codepoint = glyph_info[counter].codepoint;
	}
}

static void gl_label_setup_freetype()
{
	FT_Error errorret;
	assert (!(errorret = FT_Init_FreeType(&global_rendering_data.library)));
	
	assert (!(errorret = FT_New_Face(global_rendering_data.library, FONT_FILE, 0, &global_rendering_data.face)));

	assert (!(errorret = FT_Set_Char_Size(global_rendering_data.face, 0, LABEL_HEIGHT*64, 72,72)));
}
			       
static void gl_label_setup_harfbuzz()
{
	global_rendering_data.hb_language = hb_language_from_string("en", strlen("en"));
	global_rendering_data.hb_font = hb_ft_font_create(global_rendering_data.face);
}

void gl_label_setup()
{
	gl_shape *parent = gl_shape_new();
	gl_object *parent_obj = (gl_object *)parent;
	memcpy(&gl_label_funcs_global.p, parent->f, sizeof(gl_shape_funcs));
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_label_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_label_free;
	
	parent_obj->f->free(parent_obj);
	
	gl_label_setup_freetype();
	gl_label_setup_harfbuzz();
}

gl_label *gl_label_init(gl_label *obj)
{
	gl_shape_init((gl_shape *)obj);
	
	obj->f = &gl_label_funcs_global;
	
	return obj;
}

gl_label *gl_label_new()
{
	gl_label *ret = calloc(1, sizeof(gl_label));
	
	return gl_label_init(ret);
}

void gl_label_free(gl_object *obj_obj)
{
	gl_label *obj = (gl_label *)obj_obj;
	free (obj->data.text);
	
	gl_object_free_org_global(obj_obj);
}
