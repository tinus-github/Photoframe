//
//  gl-label-renderer.c
//  Photoframe
//
//  Created by Martijn Vernooij on 05/01/2017.
//
//

#include "labels/gl-label-renderer.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <assert.h>
#include <string.h>
#include "hb-ft.h"
#include <limits.h>

#include "infrastructure/gl-object.h"
#include "gl-tile.h"
#include "config/gl-configuration.h"

#ifdef __APPLE__
#define FONT_FILE "/Library/Fonts/Arial Unicode.ttf"
#else
#define FONT_FILE "/home/pi/lib/share/font/DejaVuSerif.ttf"
#endif

#define LABEL_HEIGHT 64
#define LABEL_BASELINE LABEL_HEIGHT

static gl_tile *gl_label_renderer_render(gl_label_renderer *obj,
					 int32_t windowX, int32_t windowY, int32_t windowWidth, int32_t windowHeight);
static void gl_label_renderer_layout(gl_label_renderer *obj);
static void gl_label_renderer_free(gl_object *obj_obj);

static struct gl_label_renderer_funcs gl_label_renderer_funcs_global = {
	.layout = &gl_label_renderer_layout,
	.render = &gl_label_renderer_render
};

static void (*gl_object_free_org_global) (gl_object *obj);

struct rendering_data {
	FT_Library library;
	FT_Face face;
	
	hb_language_t hb_language;
	hb_font_t *hb_font;
};

static struct rendering_data global_rendering_data;

typedef struct gl_label_renderer_rect {
	int x;
	int y;
	int width;
	int height;
} gl_label_renderer_rect;

static void gl_label_renderer_blit_preclipped(unsigned char* dest, unsigned int dest_stride,
					      unsigned char* src,  unsigned int src_stride,
					      unsigned int src_start_x, unsigned int src_end_x,
					      unsigned int src_start_y, unsigned int src_end_y,
					      int offset_x, int offset_y)
{
	unsigned int counter_y;
	unsigned int counter_x;
	int this_dst_x;
	int this_dst_y;
	unsigned int this_src_index;
	unsigned int this_dst_index;
	
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

static void gl_label_renderer_blit(unsigned char *dest, gl_label_renderer_rect *dest_rect,
				   unsigned char *src, gl_label_renderer_rect *src_rect,
				   int offset_x, int offset_y)
{
	int total_offset_x = offset_x;
	int total_offset_y = offset_y;
	
	total_offset_x += src_rect->x;
	total_offset_y += src_rect->y;
	total_offset_x -= dest_rect->x;
	total_offset_y -= dest_rect->y;
	
	int blit_start_x = 0;
	int blit_end_x = src_rect->width;
	int blit_start_y = 0;
	int blit_end_y = src_rect->height;
	
	if (total_offset_x < 0) {
		blit_start_x = -total_offset_x;
	}
	if (total_offset_y < 0) {
		blit_start_y = -total_offset_y;
	}
	
	if ((blit_end_x + total_offset_x) > dest_rect->width) {
		blit_end_x = dest_rect->width - total_offset_x;
	}
	if ((blit_end_y + total_offset_y) > dest_rect->height) {
		blit_end_y = dest_rect->height - total_offset_y;
	}
	
	if ((blit_end_x <= blit_start_x) ||
	    (blit_end_y <= blit_start_y)) {
		return;
	}
	
	gl_label_renderer_blit_preclipped(dest, dest_rect->width,
				 src, src_rect->width,
				 blit_start_x, blit_end_x,
				 blit_start_y, blit_end_y,
				 total_offset_x, total_offset_y);
}

static void gl_label_renderer_render_character(gl_label_renderer *obj,
					       uint32_t glyph_index,
					       int32_t x, int32_t y,
					       unsigned char* bitmap, gl_label_renderer_rect *bitmap_rect)
{
	FT_Error errorret;
	
	assert (!(errorret = FT_Load_Glyph(global_rendering_data.face, glyph_index, FT_LOAD_DEFAULT)));
	
	FT_GlyphSlot glyph = global_rendering_data.face->glyph;
	if (glyph->format != FT_GLYPH_FORMAT_BITMAP) {
		assert (!(errorret = FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL)));
	}
	
	gl_label_renderer_rect src_rect_stack;
	gl_label_renderer_rect *src_rect = &src_rect_stack;
	src_rect_stack.x = glyph->bitmap_left;
	src_rect_stack.y = -glyph->bitmap_top;
	src_rect_stack.width = glyph->bitmap.width;
	src_rect_stack.height = glyph->bitmap.rows;
	
	gl_label_renderer_blit(bitmap, bitmap_rect, glyph->bitmap.buffer, src_rect, x, y);
}

static gl_tile *gl_label_renderer_render(gl_label_renderer *obj,
					 int32_t windowX, int32_t windowY, int32_t windowWidth, int32_t windowHeight)
{
	//gl_label_renderer_layout(obj);
	gl_bitmap *bitmap = gl_bitmap_new();
	bitmap->data.bitmap = calloc(1, windowWidth * windowHeight);
	
	gl_label_renderer_rect bitmap_rect_stack;
	gl_label_renderer_rect *bitmap_rect = &bitmap_rect_stack;
	bitmap_rect->x = windowX;
	bitmap_rect->y = windowY;
	bitmap_rect->width = windowWidth;
	bitmap_rect->height = windowHeight;
	
	FT_Face face = global_rendering_data.face;
	
	uint32_t counter;
	uint32_t x_ppem = face->size->metrics.x_ppem;
	long maxwidth_l = ((face->bbox.xMax - face->bbox.xMin) * x_ppem) / face->units_per_EM;
	uint32_t maxwidth = (uint32_t)maxwidth_l; // INT_MAX should be enough for everyone
	
	int32_t minx = (windowX - maxwidth) * 64;
	int32_t maxx = (windowX + windowWidth + maxwidth) * 64;
	for (counter = 0; counter < obj->data.numGlyphs; counter++) {
		gl_label_renderer_glyph_data *glyphdata = &obj->data.glyphData[counter];
		if (glyphdata->x > maxx) {
			break;
		}
		if (glyphdata->x >= minx ) {
			gl_label_renderer_render_character(obj,
							   glyphdata->codepoint,
							   (glyphdata->x / 64), LABEL_BASELINE + (glyphdata->y / 64),
							   bitmap->data.bitmap, bitmap_rect);
		}
	}
	
	gl_texture *texture = gl_texture_new();
	texture->data.loadPriority = gl_renderloop_member_priority_high;
	texture->f->load_image_monochrome(texture, bitmap, windowWidth, windowHeight);
	
	texture->data.dataType = gl_texture_data_type_alpha;
	
	gl_tile *tile = gl_tile_new();
	tile->f->set_texture(tile, texture);
	
	((gl_object *)bitmap)->f->unref((gl_object *)bitmap);
	
	return tile;
}

static void gl_label_renderer_layout(gl_label_renderer *obj)
{
	hb_buffer_t *buf = hb_buffer_create();
	ssize_t text_length = strlen(obj->data.text);
	int text_length_int; // Harfbuzz chose the wrong type :(
	if (text_length > INT_MAX) {
		text_length_int = INT_MAX;
	} else {
		text_length_int = (int)text_length;
	}
	hb_buffer_add_utf8(buf, obj->data.text, text_length_int, 0, text_length_int);
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
	obj->data.glyphData = calloc(obj->data.numGlyphs, sizeof(gl_label_renderer_glyph_data));
	
	obj->data.totalWidth = 0;
	
	for(counter = 0; counter < obj->data.numGlyphs; counter++) {
		obj->data.glyphData[counter].x = cursorX + glyph_pos[counter].x_offset;
		obj->data.glyphData[counter].y = cursorY - glyph_pos[counter].y_offset;
		cursorX += glyph_pos[counter].x_advance;
		cursorY += glyph_pos[counter].y_advance;
		
		obj->data.glyphData[counter].codepoint = glyph_info[counter].codepoint;
		
		if (cursorX > obj->data.totalWidth) {
			obj->data.totalWidth = cursorX;
		}
	}
	
	obj->data.totalWidth = obj->data.totalWidth / 64;
	
	hb_buffer_destroy(buf);
}

static void gl_label_renderer_setup_freetype()
{
	FT_Error errorret;
	const char *fontFileName = NULL;
	int fontSize = 0;
	
	assert (!(errorret = FT_Init_FreeType(&global_rendering_data.library)));
	
	gl_configuration *cf = gl_configuration_get_global_configuration();
	if (cf) {
		gl_config_section *section = cf->f->get_section(cf, "Labels");
		if (section) {
			gl_config_value *fontFileValue = section->f->get_value(section, "fontFile");
			if (fontFileValue) {
				fontFileName = fontFileValue->f->get_value_string(fontFileValue);
			}
			gl_config_value *fontSizeValue = section->f->get_value(section, "fontSize");
			if (fontSizeValue) {
				fontSize = fontSizeValue->f->get_value_int(fontSizeValue);
			}
			
		}
	}
	if (!fontFileName) {
		fontFileName = FONT_FILE;
	}
	if (!fontSize) {
		fontSize = LABEL_HEIGHT;
	}
	
	assert (!(errorret = FT_New_Face(global_rendering_data.library, fontFileName, 0, &global_rendering_data.face)));
	
	assert (!(errorret = FT_Set_Char_Size(global_rendering_data.face, 0, fontSize*64, 72,72)));
	
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

static void gl_label_renderer_setup_harfbuzz()
{
	global_rendering_data.hb_language = hb_language_from_string("en", strlen("en"));
	global_rendering_data.hb_font = hb_ft_font_create(global_rendering_data.face, NULL);
}

void gl_label_renderer_setup()
{
	gl_object *parent = gl_object_new();
	memcpy(&gl_label_renderer_funcs_global.p, parent->f, sizeof(gl_object_funcs));
	
	gl_object_funcs *obj_funcs_global = (gl_object_funcs *) &gl_label_renderer_funcs_global;
	gl_object_free_org_global = obj_funcs_global->free;
	obj_funcs_global->free = &gl_label_renderer_free;
	
	parent->f->free(parent);
}

gl_label_renderer *gl_label_renderer_init(gl_label_renderer *obj)
{
	gl_object_init((gl_object *)obj);
	
	obj->f = &gl_label_renderer_funcs_global;
	
	if (!global_rendering_data.library) {
		gl_label_renderer_setup_freetype();
		gl_label_renderer_setup_harfbuzz();
	}
	
	return obj;
}

gl_label_renderer *gl_label_renderer_new()
{
	gl_label_renderer *ret = calloc(1, sizeof(gl_label_renderer));
	
	return gl_label_renderer_init(ret);
}

static void gl_label_renderer_free(gl_object *obj_obj)
{
	gl_label_renderer *obj = (gl_label_renderer *)obj_obj;
	free(obj->data.text);
	free(obj->data.glyphData);
	
	gl_object_free_org_global(obj_obj);
}
