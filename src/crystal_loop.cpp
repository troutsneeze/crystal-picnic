#include <cctype>
#include <cstdio>

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "abilities.h"
#include "animation.h"
#include "animation_set.h"
#include "config.h"
#include "crystal_loop.h"
#include "engine.h"
#include "game_specific_globals.h"
#include "general.h"
#include "graphics.h"
#include "player.h"
#include "widgets.h"
#include "wrap.h"

#if defined GOOGLEPLAY || defined AMAZON
#include "android.h"
#endif

#ifdef GAMECENTER
#include "gamecenter.h"
#endif

#ifdef STEAMWORKS
#include "steamworks.h"
#endif

bool Crystal_Button::available()
{
	int tier;
	Abilities::get_tier(*abilities, &tier, NULL, NULL);
	if (number <= tier) {
		return true;
	}
	return false;
}

bool Crystal_Button::filled()
{
	if (id == "hp") {
		if (abilities->hp > number) {
			return true;
		}
	}
	else if (id == "mp") {
		if (abilities->mp > number) {
			return true;
		}
	}
	else {
		if (abilities->abilities[atoi(id.c_str())] > number) {
			return true;
		}
	}
	return false;
}

std::string Crystal_Button::get_description(std::string player_name)
{
	if (id == "hp") {
		return t("PLUS10_MAXHP");
	}
	else if (id == "mp") {
		return t("PLUS5_MAXMP");
	}
	else {
		if (player_name == "egbert") {
			if (id == "0") {
				return t("SLASH");
			}
			else if (id == "1") {
				return t("THROW");
			}
			else {
				return t("ICE");
			}
		}
		else if (player_name == "frogbert") {
			if (id == "0") {
				return t("KICK");
			}
			else if (id == "1") {
				return t("PLANT");
			}
			else {
				return t("FIRE");
			}
		}
		else { // bisou
			if (id == "0") {
				return t("ROLL");
			}
			else if (id == "1") {
				return t("BURROW");
			}
			else {
				return t("HEAL");
			}
		}
	}
}

bool Crystal_Button::acceptsFocus()
{
	return available();
}

void Crystal_Button::draw(int abs_x, int abs_y)
{
	Graphics::quick_draw(slot_bmp->bitmap, abs_x, abs_y, 0);
	if (available()) {
		Graphics::quick_draw(available_bmp->bitmap, abs_x, abs_y, 0);
	}
	if (filled()) {
		Graphics::quick_draw(crystal_bmp->bitmap, abs_x, abs_y, 0);
	}
}

Crystal_Button::Crystal_Button(int x, int y, std::string id, int number, Abilities::Abilities *abilities) :
	W_Button(x, y, 18, 18)
{
	this->id = id;
	this->number = number;
	this->abilities = abilities;

	crystal_bmp = resource_manager->reference_bitmap("misc_graphics/interface/crystal_menu/crystal.png");
	slot_bmp = resource_manager->reference_bitmap("misc_graphics/interface/crystal_menu/slot.png");
	available_bmp = resource_manager->reference_bitmap("misc_graphics/interface/crystal_menu/available.png");
}

Crystal_Button::~Crystal_Button() {
	resource_manager->release_bitmap("misc_graphics/interface/crystal_menu/crystal.png");
	resource_manager->release_bitmap("misc_graphics/interface/crystal_menu/slot.png");
	resource_manager->release_bitmap("misc_graphics/interface/crystal_menu/available.png");
}

//--

bool Crystal_Loop::init()
{
	if (inited) {
		return true;
	}
	Loop::init();

	engine->load_sample("sfx/spend_crystal.ogg");

	for (size_t i = 0; i < players.size(); i++) {
		Wrap::Bitmap *b = Wrap::load_bitmap("misc_graphics/interface/" + players[i]->get_name() + "_normal_icon.png");
		player_bmps.push_back(b);
	}

	tgui::setNewWidgetParent(NULL);

	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < /*3*/1; y++) {
			//ability_buttons[x][y] = new Crystal_Button(10 + x * 25, 60 - y * 25, General::itos(x), y, &abilities);
			ability_buttons[x][y] = new Crystal_Button(30 + x * 35, 60 - /*y*/1 * 25, General::itos(x), y, &abilities);
			tgui::addWidget(ability_buttons[x][y]);
		}
	}

	for (int i = 0; i < /*8*/1; i++) {
		//int x = i % 4;
		//int y = i / 4;
		//hp_buttons[i] = new Crystal_Button((cfg.screen_w-10-80) + 20 * x, 10 + 20 * y, "hp", i, &abilities);
		hp_buttons[i] = new Crystal_Button(30 + (i+3) * 35, 60 - /*y*/1 * 25, "hp", i, &abilities);
		tgui::addWidget(hp_buttons[i]);
	}

	for (int i = 0; i < /*8*/1; i++) {
		//int x = i % 4;
		//int y = i / 4;
		//mp_buttons[i] = new Crystal_Button((cfg.screen_w-10-80) + 20 * x, 65 + 20 * y, "mp", i, &abilities);
		mp_buttons[i] = new Crystal_Button(30 + (i+4) * 35, 60 - /*y*/1 * 25, "mp", i, &abilities);
		tgui::addWidget(mp_buttons[i]);
	}

	return_button = new W_Button("misc_graphics/interface/fat_red_button.png", t("RETURN"));
	return_button->setX(5);
	return_button->setY(cfg.screen_h-5-20-return_button->getHeight()/2);
	tgui::addWidget(return_button);

	int bh = ability_buttons[0][0]->getHeight();
	int h = (cfg.screen_h-45) - (60+bh);
	int x = 10;
	int y = (60+bh) + h/2 - 16;

	player_button = new W_Button(x, y, 32, 32);
	tgui::addWidget(player_button);

	tgui::setFocus(return_button);

	return true;
}

void Crystal_Loop::top()
{
}

bool Crystal_Loop::handle_event(ALLEGRO_EVENT *event)
{
	if (event->type == ALLEGRO_EVENT_KEY_DOWN) {
		if (
			event->keyboard.keycode == ALLEGRO_KEY_ESCAPE
#if defined ALLEGRO_ANDROID
			|| event->keyboard.keycode == ALLEGRO_KEY_BUTTON_B
			|| event->keyboard.keycode == ALLEGRO_KEY_BACK
			|| event->keyboard.keycode == ALLEGRO_KEY_SELECT
#endif
			) {
			std::vector<Loop *> loops;
			loops.push_back(this);
			engine->fade_out(loops);
			engine->unblock_mini_loop();
			return true;
		}
	}
	else if (event->type == ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN) {
		if (event->joystick.button == cfg.joy_ability[2]) {
			std::vector<Loop *> loops;
			loops.push_back(this);
			engine->fade_out(loops);
			engine->unblock_mini_loop();
			return true;
		}
	}
	return false;
}

bool Crystal_Loop::logic()
{
	tgui::TGUIWidget *w = tgui::update();

	if (w == return_button) {
		std::vector<Loop *> loops;
		loops.push_back(this);
		engine->fade_out(loops);
		engine->unblock_mini_loop();
	}
	else if (w == player_button) {
		next_player();
	}
	else {
		bool done = false;
		std::vector<Loop *> loops;
		loops.push_back(this);
		for (int x = 0; x < 3; x++) {
			for (int y = 0; y < /*3*/1; y++) {
				if (w == ability_buttons[x][y]) {
					if (Game_Specific_Globals::crystals <= 0) {
						engine->play_sample("sfx/error.ogg");
					}
					else if (!static_cast<Crystal_Button *>(w)->filled()) {
						std::vector<std::string> v;
						v.push_back(t("REALLY_SPEND_CRYSTAL"));
						if (engine->yes_no_prompt(v, &loops)) {
							engine->play_sample("sfx/spend_crystal.ogg");
							Game_Specific_Globals::crystals--;
							players[current_player]->get_battle_attributes().abilities.abilities[x]++;
							copy_abilities();
#if defined STEAMWORKS || defined GOOGLEPLAY || defined GAMECENTER || defined AMAZON
							achieve("crystal");
#endif
						}
					}
					done = true;
					break;
				}
			}
		}
		if (!done) {
			for (int i = 0; i < /*8*/1; i++) {
				if (w == hp_buttons[i]) {
					if (Game_Specific_Globals::crystals <= 0) {
						engine->play_sample("sfx/error.ogg");
					}
					else if (!static_cast<Crystal_Button *>(w)->filled()) {
						std::vector<std::string> v;
						v.push_back(t("REALLY_SPEND_CRYSTAL"));
						if (engine->yes_no_prompt(v, &loops)) {
							engine->play_sample("sfx/spend_crystal.ogg");
							Game_Specific_Globals::crystals--;
							players[current_player]->get_battle_attributes().abilities.hp++;
							players[current_player]->get_battle_attributes().max_hp += 10 * cfg.difficulty_mult();
							copy_abilities();
#if defined STEAMWORKS || defined GOOGLEPLAY || defined GAMECENTER || defined AMAZON
							achieve("crystal");
#endif
						}
					}
				}
				else if (w == mp_buttons[i]) {
					if (Game_Specific_Globals::crystals <= 0) {
						engine->play_sample("sfx/error.ogg");
					}
					else if (!static_cast<Crystal_Button *>(w)->filled()) {
						std::vector<std::string> v;
						v.push_back(t("REALLY_SPEND_CRYSTAL"));
						if (engine->yes_no_prompt(v, &loops)) {
							engine->play_sample("sfx/spend_crystal.ogg");
							Game_Specific_Globals::crystals--;
							players[current_player]->get_battle_attributes().abilities.mp++;
							players[current_player]->get_battle_attributes().max_mp += 5;
							copy_abilities();
#if defined STEAMWORKS || defined GOOGLEPLAY || defined GAMECENTER || defined AMAZON
							achieve("crystal");
#endif
						}
					}
				}
			}
		}
	}

#if defined STEAMWORKS || defined GOOGLEPLAY || defined GAMECENTER || defined AMAZON
	bool all_full = true;
	for (int i = 0; i < 3; i++) {
		if (
			players[i]->get_battle_attributes().abilities.abilities[0] != 1 ||
			players[i]->get_battle_attributes().abilities.abilities[1] != 1 ||
			players[i]->get_battle_attributes().abilities.abilities[2] != 1 ||
			players[i]->get_battle_attributes().abilities.hp != 1 ||
			players[i]->get_battle_attributes().abilities.mp != 1
		) {
			all_full = false;
			break;
		}
	}
	if (all_full) {
		achieve("crystals");
	}
#endif

	return false;
}

void Crystal_Loop::draw()
{
	al_clear_to_color(General::UI_GREEN);

	Graphics::draw_gradient_rectangle(10, 30, 20, 60, al_map_rgba_f(0, 0, 0, 0), Graphics::change_brightness(General::UI_GREEN, 0.65f), Graphics::change_brightness(General::UI_GREEN, 0.65f), al_map_rgba_f(0, 0, 0, 0));
	al_draw_filled_rectangle(20, 30, 205, 60, Graphics::change_brightness(General::UI_GREEN, 0.65f));

	/*
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			al_draw_line(ability_buttons[i][j]->getX()+9, ability_buttons[i][j]->getY()+9, ability_buttons[i][j+1]->getX()+9, ability_buttons[i][j+1]->getY()+9, Graphics::change_brightness(General::UI_GREEN, 0.65f), 4);
		}
	}

	Graphics::draw_gradient_rectangle((cfg.screen_w-10-80)-50, 5, (cfg.screen_w-10-80)-40, 5+20, al_map_rgba_f(0, 0, 0, 0), Graphics::change_brightness(General::UI_GREEN, 0.65f), Graphics::change_brightness(General::UI_GREEN, 0.65f), al_map_rgba_f(0, 0, 0, 0));
	al_draw_filled_rectangle((cfg.screen_w-10-80)-40, 5, (cfg.screen_w-10-80)+20*4+5, 5+20, Graphics::change_brightness(General::UI_GREEN, 0.65f));
	al_draw_filled_rectangle((cfg.screen_w-10-80)-5, 5+20, (cfg.screen_w-10-80)+20*4+5, 5+20*2+10, Graphics::change_brightness(General::UI_GREEN, 0.65f));
	
	Graphics::draw_gradient_rectangle((cfg.screen_w-10-80)-50, 60, (cfg.screen_w-10-80)-40, 60+20, al_map_rgba_f(0, 0, 0, 0), Graphics::change_brightness(General::UI_GREEN, 0.65f), Graphics::change_brightness(General::UI_GREEN, 0.65f), al_map_rgba_f(0, 0, 0, 0));
	al_draw_filled_rectangle((cfg.screen_w-10-80)-40, 60, (cfg.screen_w-10-80)+20*4+5, 60+20, Graphics::change_brightness(General::UI_GREEN, 0.65f));
	al_draw_filled_rectangle((cfg.screen_w-10-80)-5, 60+20, (cfg.screen_w-10-80)+20*4+5, 60+20*2+10, Graphics::change_brightness(General::UI_GREEN, 0.65f));

	General::draw_text(t("HP"), al_map_rgb(0xff, 0xff, 0xff), (cfg.screen_w-10-80)-20, 15-General::get_font_line_height(General::FONT_HEAVY)/2, ALLEGRO_ALIGN_CENTRE, General::FONT_HEAVY);
	General::draw_text(t("MP"), al_map_rgb(0xff, 0xff, 0xff), (cfg.screen_w-10-80)-20, 70-General::get_font_line_height(General::FONT_HEAVY)/2, ALLEGRO_ALIGN_CENTRE, General::FONT_HEAVY);
	*/

	/*
	General::draw_speech_window(SPEECH_NORMAL, 5, cfg.screen_h-5-40, 120, 40, false, al_map_rgb(0xff, 0xff, 0xff), 1.0f);
	tgui::TGUIWidget *w = tgui::getFocussedWidget();
	if (w) {
		Crystal_Button *b = dynamic_cast<Crystal_Button *>(w);
		if (b) {
			General::draw_text(b->get_description(players[current_player]->get_name()), 20, cfg.screen_h-25-General::get_font_line_height(General::FONT_LIGHT)/2, 0);
		}
	}
	*/
	bool ttf_was_quick = Graphics::ttf_is_quick();
	Graphics::ttf_quick(true);
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < /*3*/1; y++) {
			std::string t = ability_buttons[x][y]->get_description(players[current_player]->get_name());
			General::draw_text(t, ability_buttons[x][y]->getX()+ability_buttons[x][y]->getWidth()/2-General::get_text_width(General::FONT_LIGHT, t)/2, x % 2 == 0 ? ability_buttons[x][y]->getY()-General::get_font_line_height(General::FONT_LIGHT)-2 : ability_buttons[x][y]->getY()+ability_buttons[x][y]->getHeight()+2, 0);
		}
	}

	for (int i = 0; i < /*8*/1; i++) {
		std::string t = hp_buttons[i]->get_description(players[current_player]->get_name());
		General::draw_text(t, hp_buttons[i]->getX()+hp_buttons[i]->getWidth()/2-General::get_text_width(General::FONT_LIGHT, t)/2, hp_buttons[i]->getY()+hp_buttons[i]->getHeight()+2, 0);
	}

	for (int i = 0; i < /*8*/1; i++) {
		std::string t = mp_buttons[i]->get_description(players[current_player]->get_name());
		General::draw_text(t, mp_buttons[i]->getX()+mp_buttons[i]->getWidth()/2-General::get_text_width(General::FONT_LIGHT, t)/2, mp_buttons[i]->getY()-General::get_font_line_height(General::FONT_LIGHT)-2, 0);
	}


	int tier;
	Abilities::get_tier(abilities, &tier, NULL, NULL);
	//General::draw_text(std::string(t("TIER_LABEL")) + " " + General::itos(tier+1), al_map_rgb(0xff, 0xff, 0xff), return_button->getX()+return_button->getWidth()+5, return_button->getY()+return_button->getHeight()/2-General::get_font_line_height(General::FONT_LIGHT), 0, General::FONT_LIGHT);
	General::draw_text(std::string(t("CRYSTALS_LABEL")) + " " + General::itos(Game_Specific_Globals::crystals), al_map_rgb(0xff, 0xff, 0xff), return_button->getX()+return_button->getWidth()+5, return_button->getY()+return_button->getHeight()/2-General::get_font_line_height(General::FONT_LIGHT)/2, 0, General::FONT_LIGHT);
	Graphics::ttf_quick(ttf_was_quick);

	Graphics::quick_draw(
		player_bmps[current_player]->bitmap,
		player_button->getX()+player_button->getWidth()/2-al_get_bitmap_width(player_bmps[current_player]->bitmap)/2,
		player_button->getY()+player_button->getHeight()/2-al_get_bitmap_height(player_bmps[current_player]->bitmap)/2,
		0
	);

	tgui::draw();
}

Crystal_Loop::Crystal_Loop(std::vector<Player *> players, int start_player) :
	players(players),
	current_player(start_player)
{
	copy_abilities();
}

Crystal_Loop::~Crystal_Loop()
{
	engine->destroy_sample("sfx/spend_crystal.ogg");

	for (size_t i = 0; i < player_bmps.size(); i++) {
		Wrap::destroy_bitmap(player_bmps[i]);
	}

	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < /*3*/1; y++) {
			ability_buttons[x][y]->remove();
			delete ability_buttons[x][y];
		}
	}

	for (int i = 0; i < /*8*/1; i++) {
		hp_buttons[i]->remove();
		delete hp_buttons[i];
	}

	for (int i = 0; i < /*8*/1; i++) {
		mp_buttons[i]->remove();
		delete mp_buttons[i];
	}

	tgui::pop(); // pushed in Main_Menu_Loop
	tgui::unhide();
}

void Crystal_Loop::copy_abilities()
{
	memcpy(&abilities, &players[current_player]->get_battle_attributes().abilities, sizeof(Abilities::Abilities));
}

void Crystal_Loop::next_player()
{
	current_player++;
	current_player %= players.size();
	copy_abilities();
}

