#include <cmath>
#include <cstdio>
#include <algorithm>

#include <allegro5/allegro.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>

#ifdef ALLEGRO_WINDOWS
#include <allegro5/allegro_direct3d.h>
#endif

#include "animation.h"
#include "animation_set.h"
#include "bitmap.h"
#include "camera.h"
#include "config.h"
#include "crystalpicnic.h"
#include "engine.h"
#include "frame.h"
#include "general.h"
#include "graphics.h"
#include "shaders.h"
#include "wrap.h"

static Wrap::Shader *shadow_shader = NULL;
static Wrap::Shader *yellow_glow_shader = NULL;
static Wrap::Shader *add_tint_shader = NULL;
static Wrap::Shader *tint_shader = NULL;

static Animation_Set *gauge_anim;
static Animation_Set *thick_gauge_anim;

namespace Graphics {

const float SWIPE_SLANT = 100;
const float SWIPE_WIDTH = 500;

static ALLEGRO_VERTEX *ttf_vertex_cache;
static int ttf_cache_size = 0;
static ALLEGRO_BITMAP *ttf_cache_bitmap;
static bool ttf_caching;
static int ttf_vcount = 0;

static ALLEGRO_VERTEX *vertex_cache;
static int cache_size = 0;
static ALLEGRO_BITMAP *cache_bitmap;
static bool caching;
static int vcount = 0;

static ALLEGRO_BITMAP *real_bitmap(ALLEGRO_BITMAP *bitmap)
{
	if (bitmap == 0) {
		return 0;
	}
	ALLEGRO_BITMAP *parent = bitmap;
	while (al_is_sub_bitmap(parent)) {
		parent = al_get_parent_bitmap(parent);
	}
	return parent;
}

static void ttf_quick_resize_cache(int num_verts)
{
	if (ttf_vcount + num_verts <= ttf_cache_size) {
		return;
	}

	int resize_amount = ((num_verts / 256) + 1) * 256;

	ttf_cache_size += resize_amount;

	ttf_vertex_cache = (ALLEGRO_VERTEX *)realloc(ttf_vertex_cache, ttf_cache_size * sizeof(ALLEGRO_VERTEX));
}

static void ttf_quick_flush()
{
	if (ttf_vcount > 0) {
		bool korean = cfg.language == "Korean";
		ALLEGRO_TRANSFORM backup;
		if (korean) {
			ALLEGRO_TRANSFORM t;
			al_copy_transform(&backup, al_get_current_transform());
			al_identity_transform(&t);
			al_compose_transform(&t, &General::korean_transform);
			al_use_transform(&t);
		}
		al_draw_prim(ttf_vertex_cache, 0, ttf_cache_bitmap, 0, ttf_vcount, ALLEGRO_PRIM_TRIANGLE_LIST);
		if (korean) {
			al_use_transform(&backup);
		}
		ttf_vcount = 0;
	}
	ttf_cache_bitmap = 0;
}

void ttf_quick(bool onoff)
{
	if (ttf_caching == onoff) {
		return;
	}

	ttf_caching = onoff;

	if (onoff == false) {
		ttf_quick_flush();
	}
}

bool ttf_is_quick()
{
	return ttf_caching;
}

void draw_ustr(const ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x, float y, int flags, const ALLEGRO_USTR *ustr)
{
	int pos = 0;
	int codepoint;
	int prev_codepoint = -1;
	ALLEGRO_GLYPH glyph;
	
	bool korean = cfg.language == "Korean";

	if (korean) {
		x *= General::font_scale;
		y *= General::font_scale;
	}

	int len = al_ustr_length(ustr);

	if (len == 0) {
		return;
	}
	
	ttf_quick_resize_cache(6 * len);

	if (flags & ALLEGRO_ALIGN_CENTRE) {
		int w = al_get_ustr_width(font, ustr); // font_scale already applied above, no need here
		x -= w / 2;
	}
	else if (flags & ALLEGRO_ALIGN_RIGHT) {
		int w = al_get_ustr_width(font, ustr); // font_scale already applied above, no need here
		x -= w;
	}

	while ((codepoint = al_ustr_get(ustr, pos)) >= 0) {
		if (al_get_glyph(font, prev_codepoint, codepoint, &glyph)) {
			if (glyph.bitmap) {
				ALLEGRO_BITMAP *real = real_bitmap(glyph.bitmap);
				if (ttf_cache_bitmap != real) {
					ttf_quick_flush();
					ttf_cache_bitmap = real;
				}

				ALLEGRO_VERTEX *v = &ttf_vertex_cache[ttf_vcount];
				
				int u1 = al_get_bitmap_x(glyph.bitmap) + glyph.x;
				int v1 = al_get_bitmap_y(glyph.bitmap) + glyph.y;
				int u2 = u1 + glyph.w;
				int v2 = v1 + glyph.h;

				float dx = x + glyph.kerning + glyph.offset_x;
				float dy = y + glyph.offset_y;

				v[0].x = dx;
				v[0].y = dy;
				v[0].z = 0;
				v[0].u = u1;
				v[0].v = v1;
				v[0].color = color;

				v[1].x = dx+glyph.w;
				v[1].y = dy;
				v[1].z = 0;
				v[1].u = u2;
				v[1].v = v1;
				v[1].color = color;
				
				v[2].x = dx+glyph.w;
				v[2].y = dy+glyph.h;
				v[2].z = 0;
				v[2].u = u2;
				v[2].v = v2;
				v[2].color = color;

				v[3].x = dx;
				v[3].y = dy;
				v[3].z = 0;
				v[3].u = u1;
				v[3].v = v1;
				v[3].color = color;

				v[4].x = dx+glyph.w;
				v[4].y = dy+glyph.h;
				v[4].z = 0;
				v[4].u = u2;
				v[4].v = v2;
				v[4].color = color;

				v[5].x = dx;
				v[5].y = dy+glyph.h;
				v[5].z = 0;
				v[5].u = u1;
				v[5].v = v2;
				v[5].color = color;

				ttf_vcount += 6;
			}

			x += glyph.advance;
		}

		prev_codepoint = codepoint;

		al_ustr_next(ustr, &pos);
	}

	if (ttf_caching == false) {
		ttf_quick_flush();
	}
}

void ttf_quick_shutdown()
{
	free(ttf_vertex_cache);
	ttf_vertex_cache = 0;
	ttf_cache_size = 0;
}

static void quick_resize_cache(int num_verts)
{
	if (vcount + num_verts <= cache_size) {
		return;
	}

	int resize_amount = ((num_verts / 256) + 1) * 256;

	cache_size += resize_amount;

	vertex_cache = (ALLEGRO_VERTEX *)realloc(vertex_cache, cache_size * sizeof(ALLEGRO_VERTEX));
}

static void quick_flush()
{
	if (vcount > 0) {
		al_draw_prim(vertex_cache, 0, cache_bitmap, 0, vcount, ALLEGRO_PRIM_TRIANGLE_LIST);
		vcount = 0;
	}
	cache_bitmap = 0;
}

void quick(bool onoff)
{
	if (caching == onoff) {
		return;
	}

	caching = onoff;

	if (onoff == false) {
		quick_flush();
	}
}

bool is_quick_on()
{
	return caching;
}

void quick_draw(ALLEGRO_BITMAP *bitmap, float sx, float sy, float sw, float sh, ALLEGRO_COLOR tint, float cx, float cy, float dx, float dy, float xscale, float yscale, float angle, int flags)
{
	ALLEGRO_BITMAP *real = real_bitmap(bitmap);
	if (cache_bitmap != real) {
		quick_flush();
		cache_bitmap = real;
	}

	quick_resize_cache(6);

	ALLEGRO_VERTEX *v = &vertex_cache[vcount];

	sx += al_get_bitmap_x(bitmap);
	sy += al_get_bitmap_y(bitmap);
	int u1 = sx;
	int u2 = sx + sw;
	int v1 = sy;
	int v2 = sy + sh;

	if (flags & ALLEGRO_FLIP_HORIZONTAL) {
		int tmp = u1;
		u1 = u2;
		u2 = tmp;
	}
	if (flags & ALLEGRO_FLIP_VERTICAL) {
		int tmp = v1;
		v1 = v2;
		v2 = tmp;
	}

	ALLEGRO_TRANSFORM t;
	al_identity_transform(&t);
	al_translate_transform(&t, -cx, -cy);
	al_scale_transform(&t, xscale, yscale);
	al_rotate_transform(&t, angle);
	al_translate_transform(&t, dx, dy);

	float x1 = 0;
	float y1 = 0;
	float x2 = sw;
	float y2 = 0;
	float x3 = sw;
	float y3 = sh;
	float x4 = 0;
	float y4 = sh;

	al_transform_coordinates(&t, &x1, &y1);
	al_transform_coordinates(&t, &x2, &y2);
	al_transform_coordinates(&t, &x3, &y3);
	al_transform_coordinates(&t, &x4, &y4);

	v[0].x = x1;
	v[0].y = y1;
	v[0].z = 0;
	v[0].u = u1;
	v[0].v = v1;
	v[0].color = tint;

	v[1].x = x2;
	v[1].y = y2;
	v[1].z = 0;
	v[1].u = u2;
	v[1].v = v1;
	v[1].color = tint;
	
	v[2].x = x3;
	v[2].y = y3;
	v[2].z = 0;
	v[2].u = u2;
	v[2].v = v2;
	v[2].color = tint;

	v[3].x = x1;
	v[3].y = y1;
	v[3].z = 0;
	v[3].u = u1;
	v[3].v = v1;
	v[3].color = tint;

	v[4].x = x3;
	v[4].y = y3;
	v[4].z = 0;
	v[4].u = u2;
	v[4].v = v2;
	v[4].color = tint;

	v[5].x = x4;
	v[5].y = y4;
	v[5].z = 0;
	v[5].u = u1;
	v[5].v = v2;
	v[5].color = tint;

	vcount += 6;

	if (caching == false) {
		quick_flush();
	}
}

void quick_draw(ALLEGRO_BITMAP *bitmap, float sx, float sy, float sw, float sh, float cx, float cy, float dx, float dy, float xscale, float yscale, float angle, int flags)
{
	quick_draw(bitmap, sx, sy, sw, sh, al_map_rgb(255, 255, 255), cx, cy, dx, dy, xscale, yscale, angle, flags);
}

void quick_draw(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float cx, float cy, float dx, float dy, float xscale, float yscale, float angle, int flags)
{
	quick_draw(bitmap, 0, 0, al_get_bitmap_width(bitmap), al_get_bitmap_height(bitmap), tint, cx, cy, dx, dy, xscale, yscale, angle, flags);
}

void quick_draw(ALLEGRO_BITMAP *bitmap, float cx, float cy, float dx, float dy, float xscale, float yscale, float angle, int flags)
{
	quick_draw(bitmap, 0, 0, al_get_bitmap_width(bitmap), al_get_bitmap_height(bitmap), al_map_rgb(255, 255, 255), cx, cy, dx, dy, xscale, yscale, angle, flags);
}

void quick_draw(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float cx, float cy, float dx, float dy, float angle, int flags)
{
	quick_draw(bitmap, 0, 0, al_get_bitmap_width(bitmap), al_get_bitmap_height(bitmap), tint, cx, cy, dx, dy, 1.0f, 1.0f, angle, flags);
}

void quick_draw(ALLEGRO_BITMAP *bitmap, float cx, float cy, float dx, float dy, float angle, int flags)
{
	quick_draw(bitmap, 0, 0, al_get_bitmap_width(bitmap), al_get_bitmap_height(bitmap), al_map_rgb(255, 255, 255), cx, cy, dx, dy, 1.0f, 1.0f, angle, flags);
}

void quick_draw(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
	ALLEGRO_BITMAP *real = real_bitmap(bitmap);
	if (cache_bitmap != real) {
		quick_flush();
		cache_bitmap = real;
	}

	quick_resize_cache(6);

	ALLEGRO_VERTEX *v = &vertex_cache[vcount];
	
	sx += al_get_bitmap_x(bitmap);
	sy += al_get_bitmap_y(bitmap);
	int u1 = sx;
	int u2 = sx + sw;
	int v1 = sy;
	int v2 = sy + sh;

	if (flags & ALLEGRO_FLIP_HORIZONTAL) {
		int tmp = u1;
		u1 = u2;
		u2 = tmp;
	}
	if (flags & ALLEGRO_FLIP_VERTICAL) {
		int tmp = v1;
		v1 = v2;
		v2 = tmp;
	}

	v[0].x = dx;
	v[0].y = dy;
	v[0].z = 0;
	v[0].u = u1;
	v[0].v = v1;
	v[0].color = tint;

	v[1].x = dx+dw;
	v[1].y = dy;
	v[1].z = 0;
	v[1].u = u2;
	v[1].v = v1;
	v[1].color = tint;
	
	v[2].x = dx+dw;
	v[2].y = dy+dh;
	v[2].z = 0;
	v[2].u = u2;
	v[2].v = v2;
	v[2].color = tint;

	v[3].x = dx;
	v[3].y = dy;
	v[3].z = 0;
	v[3].u = u1;
	v[3].v = v1;
	v[3].color = tint;

	v[4].x = dx+dw;
	v[4].y = dy+dh;
	v[4].z = 0;
	v[4].u = u2;
	v[4].v = v2;
	v[4].color = tint;

	v[5].x = dx;
	v[5].y = dy+dh;
	v[5].z = 0;
	v[5].u = u1;
	v[5].v = v2;
	v[5].color = tint;

	vcount += 6;

	if (caching == false) {
		quick_flush();
	}
}

void quick_draw(ALLEGRO_BITMAP *bitmap, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
	quick_draw(bitmap, al_map_rgb(255, 255, 255), sx, sy, sw, sh, dx, dy, dw, dh, flags);
}

void quick_draw(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float sx, float sy, float sw, float sh, float dx, float dy, int flags)
{
	quick_draw(bitmap, tint, sx, sy, sw, sh, dx, dy, sw, sh, flags);
}

void quick_draw(ALLEGRO_BITMAP *bitmap, float sx, float sy, float sw, float sh, float dx, float dy, int flags)
{
	quick_draw(bitmap, al_map_rgb(255, 255, 255), sx, sy, sw, sh, dx, dy, sw, sh, flags);
}

void quick_draw(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float dx, float dy, int flags)
{
	quick_draw(bitmap, tint, 0, 0, al_get_bitmap_width(bitmap), al_get_bitmap_height(bitmap), dx, dy, al_get_bitmap_width(bitmap), al_get_bitmap_height(bitmap), flags);
}

void quick_draw(ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags)
{
	quick_draw(bitmap, al_map_rgb(255, 255, 255), 0, 0, al_get_bitmap_width(bitmap), al_get_bitmap_height(bitmap), dx, dy, al_get_bitmap_width(bitmap), al_get_bitmap_height(bitmap), flags);
}

void quick_cache(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, int flags)
{
	ALLEGRO_BITMAP *real = real_bitmap(bitmap);
	if (cache_bitmap != real) {
		quick_flush();
		cache_bitmap = real;
	}

	quick_resize_cache(6);

	ALLEGRO_VERTEX *v = &vertex_cache[vcount];

	int u1 = al_get_bitmap_x(bitmap);
	int u2 = u1 + al_get_bitmap_width(bitmap);
	int v1 = al_get_bitmap_y(bitmap);
	int v2 = v1 + al_get_bitmap_height(bitmap);

	if (flags & ALLEGRO_FLIP_HORIZONTAL) {
		int tmp = u1;
		u1 = u2;
		u2 = tmp;
	}
	if (flags & ALLEGRO_FLIP_VERTICAL) {
		int tmp = v1;
		v1 = v2;
		v2 = tmp;
	}

	v[0].x = x1;
	v[0].y = y1;
	v[0].z = 0;
	v[0].u = u1;
	v[0].v = v1;
	v[0].color = tint;

	v[1].x = x2;
	v[1].y = y2;
	v[1].z = 0;
	v[1].u = u2;
	v[1].v = v1;
	v[1].color = tint;
	
	v[2].x = x3;
	v[2].y = y3;
	v[2].z = 0;
	v[2].u = u2;
	v[2].v = v2;
	v[2].color = tint;

	v[3].x = x1;
	v[3].y = y1;
	v[3].z = 0;
	v[3].u = u1;
	v[3].v = v1;
	v[3].color = tint;

	v[4].x = x3;
	v[4].y = y3;
	v[4].z = 0;
	v[4].u = u2;
	v[4].v = v2;
	v[4].color = tint;

	v[5].x = x4;
	v[5].y = y4;
	v[5].z = 0;
	v[5].u = u1;
	v[5].v = v2;
	v[5].color = tint;

	vcount += 6;

	if (caching == false) {
		quick_flush();
	}
}

void quick_shutdown()
{
	free(vertex_cache);
	vertex_cache = 0;
	cache_size = 0;
}

void side_swipe_in(ALLEGRO_COLOR color, float percent)
{
	float x = percent * (cfg.screen_w+SWIPE_WIDTH+SWIPE_SLANT) - (SWIPE_WIDTH+SWIPE_SLANT);
	unsigned char r, g, b;
	ALLEGRO_VERTEX verts[6*256];
	int vcount = 0;

	al_unmap_rgb(color, &r, &g, &b);

	for (int i = 0; i < 256; i++) {
		float ratio = (255-i)/255.0;
		float this_w = 1.2345 * sinf(ratio*(M_PI/2));

		ALLEGRO_COLOR col = al_map_rgba(r, g, b, i);

		verts[vcount].x = x+SWIPE_SLANT;
		verts[vcount].y = cfg.screen_h;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;
		verts[vcount].x = x;
		verts[vcount].y = 0;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;
		verts[vcount].x = x+SWIPE_SLANT+this_w;
		verts[vcount].y = cfg.screen_h;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;
		verts[vcount].x = x;
		verts[vcount].y = 0;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;
		verts[vcount].x = x+SWIPE_SLANT+this_w;
		verts[vcount].y = cfg.screen_h;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;
		verts[vcount].x = x+this_w;
		verts[vcount].y = 0;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;

		x += this_w;
	}

	al_draw_prim(verts, 0, 0, 0, vcount, ALLEGRO_PRIM_TRIANGLE_LIST);

	al_draw_filled_triangle(
		x, 0,
		x+SWIPE_SLANT, 0,
		x+SWIPE_SLANT, cfg.screen_h,
		al_map_rgb(r, g, b)
	);
	al_draw_filled_rectangle(
		x+SWIPE_SLANT, 0,
		al_get_display_width(engine->get_display()), al_get_display_height(engine->get_display()),
		al_map_rgb(r, g, b)
	);
}

void side_swipe_out(ALLEGRO_COLOR color, float percent)
{
	float x = percent * (cfg.screen_w+SWIPE_WIDTH+SWIPE_SLANT);
	unsigned char r, g, b;
	ALLEGRO_VERTEX verts[6*256];
	int vcount = 0;

	al_unmap_rgb(color, &r, &g, &b);

	for (int i = 0; i < 256; i++) {
		float ratio = (255-i)/255.0;
		float this_w = 1.2345 * sinf(ratio*(M_PI/2));

		ALLEGRO_COLOR col = al_map_rgba(r, g, b, i);

		verts[vcount].x = x-SWIPE_SLANT;
		verts[vcount].y = 0;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;
		verts[vcount].x = x;
		verts[vcount].y = cfg.screen_h;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;
		verts[vcount].x = x-SWIPE_SLANT-this_w;
		verts[vcount].y = 0;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;
		verts[vcount].x = x;
		verts[vcount].y = cfg.screen_h;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;
		verts[vcount].x = x-SWIPE_SLANT-this_w;
		verts[vcount].y = 0;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;
		verts[vcount].x = x-this_w;
		verts[vcount].y = cfg.screen_h;
		verts[vcount].z = 0;
		verts[vcount].color = col;
		vcount++;

		x -= this_w;
	}

	al_draw_prim(verts, 0, 0, 0, vcount, ALLEGRO_PRIM_TRIANGLE_LIST);

	al_draw_filled_triangle(
		x-SWIPE_SLANT, 0,
		x-SWIPE_SLANT, cfg.screen_h,
		x, cfg.screen_h,
		al_map_rgb(r, g, b)
	);
	al_draw_filled_rectangle(
		0, 0,
		x-SWIPE_SLANT, cfg.screen_h,
		al_map_rgb(r, g, b)
	);
}

void turn_bitmap(Wrap::Bitmap *bitmap, float angle)
{
	ALLEGRO_TRANSFORM t, backup, backup2;
	al_copy_transform(&backup, al_get_current_projection_transform());
	al_copy_transform(&backup2, al_get_current_transform());

	al_identity_transform(&t);

	float x;
	float y;
	if (cfg.screen_w > cfg.screen_h) {
		x = 1.0f / 2.0f;
		y = (float)cfg.screen_h / cfg.screen_w / 2.0f;
	}
	else {
		x = (float)cfg.screen_w / cfg.screen_h / 2.0f;
		y = 1.0f / 2.0f;
	}

	al_scale_transform_3d(&t, 10.0f, 10.0f, 1.0f);
	al_rotate_transform_3d(&t, 0.0f, 1.0f, 0.0f, angle);
	al_translate_transform_3d(&t, 0, 0, -10.0f);


	al_perspective_transform(
		&t,
		-x,
		-y,
		1,
		x,
		y,
		1000
	);

	al_use_projection_transform(&t);

	al_identity_transform(&t);
	al_use_transform(&t);

	int low, high;

	if (angle >= M_PI/2) {
		low = cfg.screen_w;
		high = 0;
	}
	else {
		low = 0;
		high = cfg.screen_w;
	}

	float x1, x2;
	float y1, y2;
	x1 = -x;
	y1 = -y;
	x2 = x;
	y2 = y;

	ALLEGRO_VERTEX v[4];
	v[0].x = x1;
	v[0].y = y2;
	v[0].z = 0;
	v[0].u = low;
	v[0].v = cfg.screen_h;
	v[0].color = al_map_rgb(0xff, 0xff, 0xff);
	v[1].x = x1;
	v[1].y = y1;
	v[1].z = 0;
	v[1].u = low;
	v[1].v = 0;
	v[1].color = al_map_rgb(0xff, 0xff, 0xff);
	v[2].x = x2;
	v[2].y = y2;
	v[2].z = 0;
	v[2].u = high;
	v[2].v = cfg.screen_h;
	v[2].color = al_map_rgb(0xff, 0xff, 0xff);
	v[3].x = x2;
	v[3].y = y1;
	v[3].z = 0;
	v[3].u = high;
	v[3].v = 0;
	v[3].color = al_map_rgb(0xff, 0xff, 0xff);

	al_draw_prim(v, 0, bitmap->bitmap, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);

	al_use_projection_transform(&backup);
	al_use_transform(&backup2);
}

void draw_bitmap_shadow(Wrap::Bitmap *bmp, int x, int y)
{
	if (shadow_shader == NULL) {
		shadow_shader = Shader::get("shadow");
	}
	Wrap::Bitmap *work = engine->get_work_bitmap();
	ALLEGRO_BITMAP *old_target2 = al_get_target_bitmap();
	al_set_target_bitmap(work->bitmap);
	al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
	quick_draw(bmp->bitmap, 5, 5, 0);
	al_set_target_bitmap(old_target2);
	int w = al_get_bitmap_width(work->bitmap);
	int h = al_get_bitmap_height(work->bitmap);
	int w2 = al_get_bitmap_width(bmp->bitmap);
	int h2 = al_get_bitmap_height(bmp->bitmap);
	Shader::use(shadow_shader);
	al_set_shader_sampler("bmp", work->bitmap, 0);
	al_set_shader_float("bmp_w", w);
	al_set_shader_float("bmp_h", h);
	quick_draw(work->bitmap, 0, 0, w2+10, h2+10, x-5, y-5, 0);
	Shader::use(NULL);
}

void draw_bitmap_shadow_region_no_intermediate(Wrap::Bitmap *bmp, int sx, int sy, int sw, int sh, int dx, int dy)
{
	if (shadow_shader == NULL) {
		shadow_shader = Shader::get("shadow");
	}
	int w = al_get_bitmap_width(bmp->bitmap);
	int h = al_get_bitmap_height(bmp->bitmap);
	Shader::use(shadow_shader);
	al_set_shader_sampler("bmp", bmp->bitmap, 0);
	al_set_shader_float("bmp_w", w);
	al_set_shader_float("bmp_h", h);
	quick_draw(bmp->bitmap, sx, sy, sw, sh, dx, dy, 0);
	Shader::use(NULL);
}

void draw_focus_circle(float cx, float cy, float radius, ALLEGRO_COLOR color)
{
	ALLEGRO_BITMAP *target = al_get_target_bitmap();
	int target_w = al_get_bitmap_width(target);
	float scale = (float)target_w / cfg.screen_w;

	ALLEGRO_TRANSFORM t, backup;
	al_copy_transform(&backup, al_get_current_transform());
	al_identity_transform(&t);
	al_use_transform(&t);

	double time = fmod(al_get_time(), 1.0);
	if (time > 0.5) time = 1.0 - time;

	al_draw_circle(cx*scale, cy*scale, radius*scale+scale*time, color, scale);

	al_use_transform(&backup);
}

// ox, oy are offset of the camera, like area->top which is needed to keep line segments from moving when screen is panned
void draw_stippled_line(float x1, float y1, float x2, float y2, float ox, float oy, ALLEGRO_COLOR color, float thickness, float run1, float run2)
{
	float len_x = x2 - x1;
	float len_y = y2 - y1;
	float len = sqrtf(len_x*len_x + len_y*len_y);
	float a = atan2(len_y, len_x);

	for (float done = 0.0f; done < len; done += run1+run2) {
		float run = MIN(done, run1);
		float start_x = x1 + cosf(a) * done;
		float start_y = y1 + sinf(a) * done;
		float end_x = start_x + cosf(a) * run;
		float end_y = start_y + sinf(a) * run;
		al_draw_line(start_x-ox, start_y-oy, end_x-ox, end_y-oy, color, thickness);
	}
}

void draw_info_box(float topright_x, float topright_y, int width, int height, std::string text)
{
	Wrap::Bitmap *work = engine->get_work_bitmap();
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
	al_set_target_bitmap(work->bitmap);
	al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
	al_draw_filled_rectangle(
		5, 5,
		5+width, 5+height,
		al_map_rgb(0x00, 0x00, 0x00)
	);
	al_set_target_bitmap(old_target);
	draw_bitmap_shadow_region_no_intermediate(work, 0, 0, width+10, height+10, topright_x-width-5, topright_y-5);

	al_draw_filled_rectangle(
		topright_x-width,
		topright_y,
		topright_x,
		topright_y+height,
		al_map_rgb(0xd2, 0xb4, 0x8c)
	);

	ALLEGRO_COLOR dark = change_brightness(al_map_rgb(0xd2, 0xb4, 0x8c), 0.5f);
	al_draw_rectangle(
		topright_x-width+0.5f,
		topright_y+0.5f,
		topright_x-0.5f,
		topright_y+height-0.5f,
		dark,
		1
	);

	General::draw_wrapped_text(text, al_map_rgb(0x00, 0x00, 0x00), topright_x-width+2, topright_y+2, width-4);
}

#define TINT_COLOR(c1, c2) \
	c1.r *= c2.r; \
	c1.g *= c2.g; \
	c1.b *= c2.b; \
	c1.a *= c2.a;

void draw_gauge(ALLEGRO_COLOR tint, int x, int y, int width, bool thick, float percent, ALLEGRO_COLOR gauge_hilight_color, ALLEGRO_COLOR color)
{
	TINT_COLOR(gauge_hilight_color, tint)
	TINT_COLOR(color, tint)

	if (gauge_anim == NULL) {
		gauge_anim = new Animation_Set();
		gauge_anim->load("misc_graphics/interface/gauge");
		thick_gauge_anim = new Animation_Set();
		thick_gauge_anim->load("misc_graphics/interface/thick_gauge");
	}

	// draw color
	int startx = x+2;
	int endx = x+width-2;
	int length = (endx-startx) * percent;

	al_draw_filled_rectangle(startx, y+1, endx, y+3+(thick ? 1 : 0), al_map_rgb(0x00, 0x00, 0x00));

	if (length > 0) {
		if (percent > 0 && length == 0) length = 1;
		ALLEGRO_COLOR darker = color;
		darker.r = MAX(0, darker.r-0.25f);
		darker.g = MAX(0, darker.g-0.25f);
		darker.b = MAX(0, darker.b-0.25f);
		al_draw_filled_rectangle(startx, y+1, startx+length, y+2, darker);
		al_draw_filled_rectangle(startx, y+2, startx+length, y+3+(thick ? 1 : 0), color);
	}

	Animation_Set *anim_set;

	if (thick) {
		anim_set = thick_gauge_anim;
	}
	else {
		anim_set = gauge_anim;
	}

	ALLEGRO_BITMAP *left, *right, *middle;

	anim_set->set_sub_animation("left");
	left = anim_set->get_current_animation()->get_current_frame()->get_bitmap()->get_bitmap()->bitmap;
	anim_set->set_sub_animation("right");
	right = anim_set->get_current_animation()->get_current_frame()->get_bitmap()->get_bitmap()->bitmap;
	anim_set->set_sub_animation("middle");
	middle = anim_set->get_current_animation()->get_current_frame()->get_bitmap()->get_bitmap()->bitmap;

	bool was_quick = is_quick_on();
	quick(true);

	quick_draw(left, gauge_hilight_color, x, y, 0);
	quick_draw(
		middle,
		gauge_hilight_color,
		0, 0, al_get_bitmap_width(middle), al_get_bitmap_height(middle),
		x+al_get_bitmap_width(left), y,
		width-al_get_bitmap_width(left)-al_get_bitmap_width(right),
		al_get_bitmap_height(middle),
		0
	);
	quick_draw(right, gauge_hilight_color, x+width-al_get_bitmap_width(right), y, 0);

	quick(was_quick);
}

ALLEGRO_COLOR change_brightness(ALLEGRO_COLOR in, float amount)
{
	ALLEGRO_COLOR bright = al_map_rgb_f(
		MAX(0.0f, MIN(1.0f, in.r * amount)),
		MAX(0.0f, MIN(1.0f, in.g * amount)),
		MAX(0.0f, MIN(1.0f, in.b * amount)) 
	);

	return bright;
}

void init()
{
	yellow_glow_shader = Shader::get("yellow_glow");
	add_tint_shader = Shader::get("add_tint");
	tint_shader = Shader::get("tint");
}

void shutdown()
{
	if (shadow_shader) {
		Shader::destroy(shadow_shader);
	}
	if (yellow_glow_shader) {
		Shader::destroy(yellow_glow_shader);
	}
	if (add_tint_shader) {
		Shader::destroy(add_tint_shader);
	}
	if (tint_shader) {
		Shader::destroy(tint_shader);
	}

	if (gauge_anim) {
		delete gauge_anim;
		gauge_anim = NULL;
		delete thick_gauge_anim;
		thick_gauge_anim = NULL;
	}

	ttf_quick_shutdown();
	quick_shutdown();
}

void capture_target(ALLEGRO_BITMAP *tmp)
{
	ALLEGRO_BITMAP *target = al_get_target_bitmap();

#if defined ALLEGRO_ANDROID || defined ALLEGRO_IPHONE || defined ALLEGRO_RASPBERRYPI
	const int MAX_SIZE = 512;

	int w = al_get_bitmap_width(target);
	int h = al_get_bitmap_height(target);

	int sz_w = ceil((float)w / MAX_SIZE);
	int sz_h = ceil((float)h / MAX_SIZE);

	for (int yy = 0; yy < sz_h; yy++) {
		for (int xx = 0; xx < sz_w; xx++) {
			int ww = MIN(MAX_SIZE, w-(xx*MAX_SIZE));
			int hh = MIN(MAX_SIZE, h-(yy*MAX_SIZE));
			ALLEGRO_LOCKED_REGION *lr1 = al_lock_bitmap_region(
				tmp,
				xx*MAX_SIZE, yy*MAX_SIZE, ww, hh,
				al_get_bitmap_format(target), ALLEGRO_LOCK_WRITEONLY
			);
			ALLEGRO_LOCKED_REGION *lr2 = al_lock_bitmap_region(
				target,
				xx*MAX_SIZE, yy*MAX_SIZE, ww, hh,
				al_get_bitmap_format(target), ALLEGRO_LOCK_READONLY
			);
			int pixel_size = al_get_pixel_size(al_get_bitmap_format(target));
			for (int y = 0; y < hh; y++) {
				uint8_t *d1 = (uint8_t *)lr1->data + lr1->pitch * y;
				uint8_t *d2 = (uint8_t *)lr2->data + lr2->pitch * y;
				memcpy(d1, d2, pixel_size*ww);
			}
			al_unlock_bitmap(tmp);
			al_unlock_bitmap(target);
		}
	}
#else
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();

	al_set_target_bitmap(tmp);

	al_clear_to_color(al_map_rgb_f(0, 0, 0));

	al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

	quick_draw(target, 0, 0, 0);

	al_set_target_bitmap(old_target);
#endif
}

void draw_tinted_bitmap_region_depth_yellow_glow(
	Wrap::Bitmap *bitmap,
	ALLEGRO_COLOR tint,
	float sx, float sy,
	float sw, float sh,
	float dx, float dy,
	int flags,
	float depth,
	int r1, int g1, int b1,
	int r2, int g2, int b2)
{
	float p = fmod(al_get_time(), 2.0);
	if (p >= 1.0f) {
		p = 1.0f - (p - 1.0f);
	}

	Shader::use(yellow_glow_shader);
	al_set_shader_float("p", p);
	float color1[3] = {
		(float)r1/255.0f,
		(float)g1/255.0f,
		(float)b1/255.0f,
	};
	float color2[3] = {
		(float)r2/255.0f,
		(float)g2/255.0f,
		(float)b2/255.0f,
	};

	al_set_shader_float("color1_r", color1[0]);
	al_set_shader_float("color1_g", color1[1]);
	al_set_shader_float("color1_b", color1[2]);
	al_set_shader_float("color2_r", color2[0]);
	al_set_shader_float("color2_g", color2[1]);
	al_set_shader_float("color2_b", color2[2]);

	quick_draw(bitmap->bitmap, tint, sx, sy, sw, sh, dx, dy, flags);
	Shader::use(NULL);
}

void get_texture_size(ALLEGRO_BITMAP *bmp, int *outw, int *outh)
{
	if (al_get_display_flags(engine->get_display()) & ALLEGRO_OPENGL) {
		al_get_opengl_texture_size(bmp, outw, outh);
	}
#ifdef ALLEGRO_WINDOWS
	else {
		al_get_d3d_texture_size(bmp, outw, outh);
	}
#endif
}

Wrap::Shader *get_add_tint_shader()
{
	return add_tint_shader;
}

Wrap::Shader *get_tint_shader()
{
	return tint_shader;
}

void draw_gradient_rectangle(float x1, float y1, float x2, float y2, ALLEGRO_COLOR x1y1, ALLEGRO_COLOR x2y1, ALLEGRO_COLOR x2y2, ALLEGRO_COLOR x1y2)
{
	ALLEGRO_VERTEX v[4];

	v[0].x = x1;
	v[0].y = y2;
	v[0].z = 0;
	v[0].color = x1y2;
	v[1].x = x1;
	v[1].y = y1;
	v[1].z = 0;
	v[1].color = x1y1;
	v[2].x = x2;
	v[2].y = y1;
	v[2].z = 0;
	v[2].color = x2y1;
	v[3].x = x2;
	v[3].y = y2;
	v[3].z = 0;
	v[3].color = x2y2;

	al_draw_prim(v, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
}

ALLEGRO_BITMAP *clone_target()
{
	ALLEGRO_BITMAP *target = al_get_target_bitmap();

	int format = al_get_new_bitmap_format();
	al_set_new_bitmap_format(General::noalpha_bmp_format);
	ALLEGRO_BITMAP *tmp = al_create_bitmap(
		al_get_bitmap_width(target),
		al_get_bitmap_height(target)
	);
	al_set_new_bitmap_format(format);

#if defined ALLEGRO_ANDROID || defined ALLEGRO_IPHONE
	const int MAX_SIZE = 512;

	int w = al_get_bitmap_width(target);
	int h = al_get_bitmap_height(target);

	int sz_w = ceil((float)w / MAX_SIZE);
	int sz_h = ceil((float)h / MAX_SIZE);

	for (int yy = 0; yy < sz_h; yy++) {
		for (int xx = 0; xx < sz_w; xx++) {
			int ww = MIN(MAX_SIZE, w-(xx*MAX_SIZE));
			int hh = MIN(MAX_SIZE, h-(yy*MAX_SIZE));
			ALLEGRO_LOCKED_REGION *lr1 = al_lock_bitmap_region(
				tmp,
				xx*MAX_SIZE, yy*MAX_SIZE, ww, hh,
				al_get_bitmap_format(target), ALLEGRO_LOCK_WRITEONLY
			);
			ALLEGRO_LOCKED_REGION *lr2 = al_lock_bitmap_region(
				target,
				xx*MAX_SIZE, yy*MAX_SIZE, ww, hh,
				al_get_bitmap_format(target), ALLEGRO_LOCK_READONLY
			);
			int pixel_size = al_get_pixel_size(al_get_bitmap_format(target));
			for (int y = 0; y < hh; y++) {
				uint8_t *d1 = (uint8_t *)lr1->data + lr1->pitch * y;
				uint8_t *d2 = (uint8_t *)lr2->data + lr2->pitch * y;
				memcpy(d1, d2, pixel_size*ww);
			}
			al_unlock_bitmap(tmp);
			al_unlock_bitmap(target);
		}
	}
#else
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();

	al_set_target_bitmap(tmp);

	al_clear_to_color(al_map_rgb_f(0, 0, 0));

	ALLEGRO_STATE state;
	al_store_state(&state, ALLEGRO_STATE_BLENDER);

	al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

	quick_draw(target, 0, 0, 0);

	al_restore_state(&state);

	al_set_target_bitmap(old_target);
#endif

	return tmp;
}

} // end namespace Graphics
