#include <cctype>
#include <algorithm>

#include <allegro5/allegro.h>

#include <vector>

#include "battle_transition_in.h"
#include "bitmap.h"
#include "config.h"
#include "dirty.h"
#include "engine.h"
#include "frame.h"
#include "general.h"
#include "general_types.h"
#include "graphics.h"
#include "wrap.h"

struct BMP {
	int x, y;
	int num;
};

#define BLOCK_SIZE (cfg.screen_w / 64)
#define NUM_BLOCKS (cfg.screen_w / BLOCK_SIZE) * (cfg.screen_h / BLOCK_SIZE)

static int last_w = -1;
static int last_h = -1;
static bool **used;
static ALLEGRO_BITMAP *bmp, *bmp2;
static int num_to_draw;
static int *random_data;
static std::vector<BMP> bmps;

void cleanup_battle_transition_in()
{
	if (used) {
		for (int i = 0; i < cfg.screen_h / BLOCK_SIZE; i++) {
			delete[] used[i];
		}

		delete[] used;

		used = 0;
	}

	delete[] random_data;

	random_data = 0;
}

static void find(int num, int w, int h)
{
	int x = General::rand() % (w / BLOCK_SIZE);
	int y = General::rand() % (h / BLOCK_SIZE);

	while (used[y][x]) {
		x++;
		if (x == (w / BLOCK_SIZE)) {
			x = 0;
			y++;
		}
		if (y == (h / BLOCK_SIZE)) {
			y = 0;
		}
	}

	used[y][x] = true;

	random_data[num*3+0] = x;
	random_data[num*3+1] = y;
	random_data[num*3+2] = num;
}

static void draw(std::vector<BMP> &v)
{
	int count = 0;
	for (size_t i = 0; i < v.size(); i++) {
		BMP &b = v[i];
		if (b.num < num_to_draw) {
			count++;
			Graphics::quick_draw(
				bmp2,
				b.x*BLOCK_SIZE,
				b.y*BLOCK_SIZE,
				BLOCK_SIZE, BLOCK_SIZE,
				b.x*BLOCK_SIZE,
				b.y*BLOCK_SIZE,
				BLOCK_SIZE, BLOCK_SIZE,
				0
			);
		}
	}
}

void battle_transition_in(Wrap::Bitmap *start_bmp, Wrap::Bitmap *end_bmp)
{
	int num = NUM_BLOCKS;
	int w = cfg.screen_w;
	int h = cfg.screen_h;

	Wrap::Bitmap *buf = Wrap::create_bitmap_no_preserve(cfg.screen_w, cfg.screen_h);

	bmp = start_bmp->bitmap;
	bmp2 = end_bmp->bitmap;

	if (last_w != w || last_h != h) {
		last_w = w;
		last_h = h;

		cleanup_battle_transition_in();

		used = new bool *[h / BLOCK_SIZE];
		for (int i = 0; i < h / BLOCK_SIZE; i++) {
			used[i] = new bool[w / BLOCK_SIZE];
			memset(used[i], 0, sizeof(bool) * (w / BLOCK_SIZE));
		}

		random_data = new int[w*h*3];

		for (int i = 0; i < num; i++) {
			find(i, w, h);
		}

		bmps.clear();

		for (int i = 0; i < num; i++) {
			BMP b;
			b.x = random_data[i * 3 + 0];
			b.y = random_data[i * 3 + 1];
			b.num = random_data[i * 3 + 2];
			bmps.push_back(b);
		}
	}

	double length = 0.5f;
	num_to_draw = 0;

	double start = al_get_time();

	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();

	while (true) {
		al_set_target_bitmap(buf->bitmap);

		Graphics::quick_draw(bmp, 0, 0, 0);

		Graphics::quick_draw(bmp, 0, 0, 0);
		bool was_quick = Graphics::is_quick_on();
		Graphics::quick(true);
		draw(bmps);
		Graphics::quick(was_quick);

		al_set_target_bitmap(old_target);

		engine->draw_to_display(buf->bitmap);

		backup_dirty_bitmaps();

		al_flip_display();

		if (num_to_draw >= NUM_BLOCKS) {
			break;
		}

		double now = al_get_time();
		double elapsed = now - start;
		double percent = elapsed / length;

		if (percent > 1.0f) {
			percent = 1.0f;
		}

		num_to_draw = NUM_BLOCKS * percent;

		al_rest(1.0 / 60.0);
	}

	Wrap::destroy_bitmap(buf);
}
