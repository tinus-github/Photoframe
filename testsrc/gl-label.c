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
#include <hb-ft.h>

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

static void gl_label_blit_preclipped(unsigned char* dest, unsigned int dest_stride,
				     unsigned char* src,  unsigned int src_stride,
				     unsigned int src_start_x, unsigned int src_end_x,
				     unsigned int src_start_y, unsigned int src_end_y,
				     int offset_x, int offset_y)
{
	unsigned int counter_y;
	unsigned int counter_x;
	
	for (counter_y = src_start_y; counter_y < src_end_y; counter_y++) {
		this_dst_y = offset_y + counter_y;
		for (counter_x = src_start_x; counter_x < src_end_x; counter_x++) {
			this_dst_x = offset_x + counter_x;
			this_dst_index = this_dst_x + (dest_stride * this_dst_y);
			this_src_index = counter_x + (src_stride * counter_y);
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

static void gl_label_blit(unsigned char *dest, gl_label_rect *dest_rect,
			  unsigned char *src, gl_label_rect *src_rect,
			  int offset_x, int offset_y)
{
	int total_offset_x = offset_x;
	int total_offset_y = offset_y;
	
	total_offset_x += src_rect->x;
	total_offset_y += src_rect->y;
	total_offset_x -= dst_rect->x;
	total_offset_y -= dst_rect->y;
	
	int blit_start_x = 0;
	int blit_end_x = src_rect->width;
	int blit_start_y = 0;
	int blit_end_y = src_rect->height;
	
	if (offset_x < 0) {
		blit_start_x = -offset_x;
	}
	if (offset_y < 0) {
		blit_start_y = -offset_y;
	}
	
	if ((blit_end_x + offset_x) > dst_rect->width) {
		blit_end_x = dst_rect->width - offset_x;
	}
	if ((blit_end_y + offset_y) > dst_rect->height) {
		blit_end_y = dst_rect->height - offset_y;
	}
	
	if ((blit_end_x <= blit_start_x) ||
	    (blit_end_y <= blit_start_y)) {
		return;
	}
	
	gl_label_blit_preclipped(dest, dest_rect->width,
				 src, src_rect->width,
				 blit_start_x, blit_end_x,
				 blit_start_y, blit_end_y,
				 total_offset_x, total_offset_y);
}

static void gl_label_render_character(gl_label *obj, uint32_t glyph_index, int32_t x, int32_t y, unsigned char* bitmap)
{
	FT_Error errorret;
	
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
		gl_label_glyph_data *glyphdata = &obj->data.glyphData[counter];
		gl_label_render_character(obj, glyphdata->codepoint, glyphdata->x / 64, glyphdata->y / 64, bitmap);
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
	hb_buffer_add_utf8(buf, obj->data.text, strlen(obj->data.text), 0, strlen(obj->data.text));
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_language(buf, global_rendering_data.hb_language);
	
	hb_shape(global_rendering_data.hb_font,
		 buf,
		 NULL,
		 0);
	
	hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &obj->data.numGlyphs);
	hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &obj->data.numGlyphs);
	
	uint32_t counter;
	int32_t cursorX = 0;
	int32_t cursorY = 0;
	obj->data.glyphData = calloc(obj->data.numGlyphs, sizeof(gl_label_glyph_data));
	
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
	
	int counter;
	for(counter = 0; counter < global_rendering_data.face->num_charmaps; counter++) {
		if (((global_rendering_data.face->charmaps[counter]->platform_id == 0)
		     && (global_rendering_data.face->charmaps[counter]->encoding_id == 3))
		    ||
		    ((global_rendering_data.face->charmaps[counter]->platform_id == 3)
		     && (global_rendering_data.face->charmaps[counter]->encoding_id == 1))) {
			    FT_Set_Charmap(global_rendering_data.face, global_rendering_data.face->charmaps[counter]);
		    }
	}
}

static void gl_label_setup_harfbuzz()
{
	global_rendering_data.hb_language = hb_language_from_string("en", strlen("en"));
	global_rendering_data.hb_font = hb_ft_font_create(global_rendering_data.face, NULL);
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
