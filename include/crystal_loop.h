#ifndef CRYSTAL_LOOP_H
#define CRYSTAL_LOOP_H

#include <string>
#include <vector>

#include <allegro5/allegro.h>

#include "abilities.h"
#include "graphics.h"
#include "loop.h"
#include "resource_manager.h"
#include "widgets.h"

class Player;

namespace Wrap {
	class Bitmap;
}

class Crystal_Button : public W_Button {
public:
	bool available();

	bool filled();

	std::string get_description(std::string player_name);

	bool acceptsFocus();

	virtual void draw(int abs_x, int abs_y);

	Crystal_Button(int x, int y, std::string id, int number, Abilities::Abilities *abilities);

	virtual ~Crystal_Button();

private:
	std::string id;
	int number;
	Abilities::Abilities *abilities;

	Wrap::Bitmap *crystal_bmp;
	Wrap::Bitmap *slot_bmp;
	Wrap::Bitmap *available_bmp;
};

class Crystal_Loop : public Loop {
public:
	// Loop interface
	bool init();
	void top();
	bool handle_event(ALLEGRO_EVENT *event);
	bool logic();
	void draw();

	Crystal_Loop(std::vector<Player *> players, int start_player);
	virtual ~Crystal_Loop();

private:
	void copy_abilities();
	void next_player();

	Crystal_Button *ability_buttons[3][3];
	Crystal_Button *hp_buttons[8];
	Crystal_Button *mp_buttons[8];

	W_Button *player_button;

	Abilities::Abilities abilities;

	std::vector<Player *> players;
	std::vector<Wrap::Bitmap *> player_bmps;

	W_Button *return_button;

	int current_player;
};

#endif // CRYSTAL_LOOP_H
