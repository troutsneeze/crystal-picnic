#ifndef CRYSTALPICNIC_H
#define CRYSTALPICNIC_H

#include "error.h"
#include "general_types.h"
#include "main.h"

// This must be included for magic main to work
#include <allegro5/allegro.h>

#ifndef M_PI
#define M_PI ALLEGRO_PI
#endif

#ifdef ALLEGRO_WINDOWS
#define mkdir(a, b) _mkdir(a)
#endif

extern bool restart_game;

#endif
