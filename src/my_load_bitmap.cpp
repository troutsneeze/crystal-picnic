#include <cstdio>

#include <allegro5/allegro.h>

#include "cpa.h"
#include "engine.h"

/* This has to load bitmaps from CPA and disk (for skeled) */
ALLEGRO_BITMAP *my_load_bitmap(std::string filename)
{
	CPA *cpa;
	ALLEGRO_FILE *f;
	if (engine) {
		cpa = engine->get_cpa();
	}
	else {
		cpa = NULL;
	}
	if (cpa) {
		f = cpa->load(filename);
	}
	else {
		f = al_fopen(filename.c_str(), "rb");
	}
	if (!f) {
		return NULL;
	}
	ALLEGRO_BITMAP *bmp;

	engine->stop_timers();

	bmp = al_load_bitmap_f(f, ".png");

	engine->start_timers();

	al_fclose(f);
	return bmp;
}
