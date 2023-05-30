#include <cstdio>

#include <allegro5/allegro.h>

#include "bitmap.h"
#include "frame.h"
#include "graphics.h"
#include "wrap.h"

int Bitmap::get_width()
{
	return al_get_bitmap_width(bitmap->bitmap);
}

int Bitmap::get_height()
{
	return al_get_bitmap_height(bitmap->bitmap);
}

Wrap::Bitmap *Bitmap::get_bitmap()
{
	return bitmap;
}

void Bitmap::set(Wrap::Bitmap *bmp)
{
	bitmap = bmp;
}

void Bitmap::draw_region(float sx, float sy, int sw, int sh, float dx, float dy, int flags)
{
	draw_region_tinted(al_map_rgb(0xff, 0xff, 0xff), sx, sy, sw, sh, dx, dy, flags);
}

void Bitmap::draw_region_tinted_depth(ALLEGRO_COLOR tint, float sx, float sy, int sw, int sh, float dx, float dy, int flags, float depth)
{
	// Don't know what this is for
	Graphics::quick_draw(bitmap->bitmap, tint, sx, sy, sw, sh, dx, dy, flags);
}

void Bitmap::draw_region_tinted(ALLEGRO_COLOR tint, float sx, float sy, int sw, int sh, float dx, float dy, int flags)
{
	Graphics::quick_draw(bitmap->bitmap, tint, sx, sy, sw, sh, dx, dy, flags);
}

Bitmap::Bitmap()
{
	bitmap = NULL;
	delete_bitmap = true;
}

Bitmap::Bitmap(bool delete_bitmap)
{
	bitmap = NULL;
	this->delete_bitmap = delete_bitmap;
}

Bitmap::~Bitmap()
{
	if (bitmap && delete_bitmap) {
		Wrap::destroy_bitmap(bitmap);
	}
}

