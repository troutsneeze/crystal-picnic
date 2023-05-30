#include <allegro5/allegro.h>

#include "dirty.h"
#include "engine.h"

void backup_dirty_bitmaps()
{
#ifdef ALLEGRO_ANDROID
	if (engine == 0 || engine->get_display() == 0) {
		return;
	}
	engine->stop_timers();
	al_backup_dirty_bitmaps(engine->get_display());
	engine->start_timers();
#elif defined ALLEGRO_WINDOWS
	if (al_get_display_flags(engine->get_display()) & ALLEGRO_OPENGL) {
		return;
	}
	engine->stop_timers();
	al_backup_dirty_bitmaps(engine->get_display());
	engine->start_timers();
#endif
}
