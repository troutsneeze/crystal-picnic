#include <cstdio>
#include <algorithm>

#include <allegro5/allegro.h>

#include "animation.h"
#include "bitmap.h"
#include "frame.h"
#include "general.h"
#include "graphics.h"
#include "resource_manager.h"
#include "shaders.h"
#include "wrap.h"

void Animation::draw_scaled(int sx, int sy, int sw, int sh, int dx, int dy,
	int dw, int dh, int flags)
{
	Wrap::Bitmap *bmp = shared->frames[current_frame]->get_bitmap()->get_bitmap();
	
	Graphics::quick_draw(bmp->bitmap, sx, sy, sw, sh, dx, dy, dw, dh, flags);
}

void Animation::draw(int x, int y, int flags)
{
	draw_tinted(al_map_rgb(0xff, 0xff, 0xff), x, y, flags);
}

void Animation::draw_tinted_rotated(ALLEGRO_COLOR tint, int x, int y, int flags)
{
	if (!shared->rotating) {
		draw_tinted(tint, x, y, flags);
		return;
	}

	Bitmap *b = shared->frames[current_frame]->get_bitmap();
	Wrap::Bitmap *bmp = b->get_bitmap();
	Graphics::quick_draw(
		bmp->bitmap,
		tint,
		shared->rotation_center.x, shared->rotation_center.y,
		x+shared->rotation_center.x, y+shared->rotation_center.y,
		(flags & ALLEGRO_FLIP_HORIZONTAL) ? -angle : angle,
		flags
	);
}

void Animation::draw_add_tinted_rotated(ALLEGRO_COLOR tint, float p, int x, int y, int flags)
{
	if (!shared->rotating) {
		draw_add_tinted(tint, p, x, y, flags);
		return;
	}

	Bitmap *b = shared->frames[current_frame]->get_bitmap();
	Wrap::Bitmap *bmp = b->get_bitmap();
	Wrap::Shader *tinter = Graphics::get_add_tint_shader();
	Shader::use(tinter);
	al_set_shader_float("p", p);
	al_set_shader_float("color_r", tint.r);
	al_set_shader_float("color_g", tint.g);
	al_set_shader_float("color_b", tint.b);
	Graphics::quick_draw(
		bmp->bitmap,
		al_map_rgb(0xff, 0xff, 0xff),
		shared->rotation_center.x, shared->rotation_center.y,
		x+shared->rotation_center.x, y+shared->rotation_center.y,
		(flags & ALLEGRO_FLIP_HORIZONTAL) ? -angle : angle,
		flags
	);
	Shader::use(NULL);
}

void Animation::draw_rotated(int x, int y, int flags)
{
	draw_tinted_rotated(al_map_rgb_f(1, 1, 1), x, y, flags);
}

void Animation::draw_tinted(ALLEGRO_COLOR tint, int x, int y, int flags)
{
	Bitmap *b = shared->frames[current_frame]->get_bitmap();
	b->draw_region_tinted(tint, 0, 0, b->get_width(), b->get_height(),
		x, y, flags);
}

void Animation::draw_add_tinted(ALLEGRO_COLOR tint, float p, int x, int y, int flags)
{
	Bitmap *b = shared->frames[current_frame]->get_bitmap();
	ALLEGRO_BITMAP *bmp = b->get_bitmap()->bitmap;
	Wrap::Shader *tinter = Graphics::get_add_tint_shader();
	Shader::use(tinter);
	al_set_shader_float("p", p);
	al_set_shader_float("color_r", tint.r);
	al_set_shader_float("color_g", tint.g);
	al_set_shader_float("color_b", tint.b);
	Graphics::quick_draw(bmp, 0, 0, b->get_width(), b->get_height(), x, y, flags);
	Shader::use(NULL);
}

void Animation::set_looping(bool l)
{
	shared->looping = l;
}

void Animation::set_loop_mode(Loop_Mode m)
{
	shared->loop_mode = m;
}

int Animation::get_length()
{
	int l = 0;

	for (int i = 0; i < shared->num_frames; i++) {
		Frame *f = shared->frames[i];
		l += f->get_delay();
	}
	
	if (shared->loop_mode == LOOP_PINGPONG) {
		l = l + (l - shared->frames[shared->num_frames-1]->get_delay());
	}

	return l;
}

/* Returns false on fail.
 * frame is not copied so should not be destroyed.
 */
bool Animation::add_frame(Frame *frame)
{
	try {
		shared->frames.push_back(frame);
		shared->num_frames++;
	}
	catch (std::bad_alloc a) {
		return false;
	}
	return true;
}

/* Returns how many frames passed
 * Can go backwards.
 * Step is in milliseconds.
 */
int Animation::update(int step)
{
	angle += shared->angle_inc;

	int passed = 0;

	if (step < 0) {
		count += step;
		while (count < 0) {
			passed++;
			current_frame -= increment;
			wrap();
			int thisDelay = shared->frames[current_frame]->get_delay();
			count += thisDelay;
			if (thisDelay <= 0)
				break;
		}
	}
	else {
		count += step;
		int thisDelay = shared->frames[current_frame]->get_delay();
		while (count >= thisDelay) {
			if ((!shared->looping && ((shared->loop_mode == LOOP_NORMAL && current_frame == shared->num_frames-1) || (shared->loop_mode == LOOP_PINGPONG && increment == -1 && current_frame == 0))) ||
					(shared->num_frames == 1))
				break;
			count -= thisDelay;
			if (thisDelay <= 0)
				break;
			passed++;
			if (!(shared->looping == false && shared->loop_mode == LOOP_PINGPONG && increment == -1 && current_frame <= 0))
				current_frame+=increment;
			wrap();
			thisDelay = shared->frames[current_frame]->get_delay();
		}
	}

	return passed;
}

int Animation::update(void)
{
	return update(General::LOGIC_MILLIS);
}

void Animation::set_frame(int frame)
{
	current_frame = frame;
}

void Animation::reset(void)
{
	current_frame = 0;
	count = 0;
	increment = 1;
}

void Animation::set_tags(std::vector<std::string> tags)
{
	shared->tags = tags;

	// set up things for rotating animations
	for (unsigned int i = 0; i < shared->tags.size(); i++) {
		std::string s = shared->tags[i];
		if (s.substr(0, 12) == "rotate_speed") {
			shared->angle_inc = atof(s.substr(13).c_str());
			shared->rotating = true;
		}
		else if (s.substr(0, 13) == "rotate_center") {
			int xofs = s.find(' ') + 1;
			int xend = s.find(' ', xofs);
			int yofs = xend+1;
			int yend = s.find(' ', yofs);
			std::string xS = s.substr(xofs, xend-xofs);
			std::string yS = s.substr(yofs, yend-yofs);
			float xx = atof(xS.c_str());
			float yy = atof(yS.c_str());
			shared->rotation_center = General::Point<float>(xx, yy);
			shared->rotating = true;
		}
	}
}

Animation::Animation(std::string name, Shared *shared) :
	shared(shared)
{
	current_frame = 0;
	count = 0;
	increment = 1;
	angle = 0;
}

/* Frames are destroyed
 */
Animation::~Animation(void)
{
	resource_manager->release_animation(shared->id);
}

void Animation::wrap(void)
{
	if (shared->loop_mode == LOOP_NORMAL) {
		if (shared->looping) {
			if (current_frame < 0) {
				current_frame = shared->num_frames-1;
			}
			else if (current_frame >= shared->num_frames) {
				current_frame = (shared->loop_start > 0) ? shared->loop_start : 0;
			}
		}
		else {
			if (current_frame >= shared->num_frames) {
				current_frame = shared->num_frames-1;
			}
		}
	}
	else if (shared->loop_mode == LOOP_PINGPONG) {
		if (shared->looping) {
			if (current_frame < 0) {
				current_frame = shared->num_frames == 1 ? 0 : 1;
				increment = 1;
			}
			else if (current_frame >= shared->num_frames) {
				current_frame = shared->num_frames == 1 ? shared->num_frames-1 : shared->num_frames-2;
				increment = -1;
			}
		}
		else {
			if (current_frame >= shared->num_frames) {
				current_frame = shared->num_frames-1;
				increment = -1;
			}
			else if (current_frame < 0 && increment < 0) {
				current_frame = 0;
			}
		}
		
	}
}

void Animation::set_loop_start(int loop_start)
{
	shared->loop_start = loop_start;
}

bool Animation::is_finished()
{
	if (((!shared->looping && ((shared->loop_mode != LOOP_PINGPONG && current_frame == shared->num_frames-1 &&
	count >= shared->frames[current_frame]->get_delay()) ||
	((shared->loop_mode == LOOP_PINGPONG) && (increment == -1) &&
		(current_frame == 0) && (count >= shared->frames[0]->get_delay())))) /*||
	(num_frames == 1 && count >= 100)*/))
		return true;
	return false;
}
