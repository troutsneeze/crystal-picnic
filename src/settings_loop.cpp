#include <cstdio>

#include <allegro5/allegro.h>

#include <tgui2.hpp>

#include "audio_config_loop.h"
#include "config.h"
#include "crystalpicnic.h"
#include "engine.h"
#include "general.h"
#include "language_config_loop.h"
#include "input_config_loop.h"
#include "settings_loop.h"
#include "widgets.h"
#include "video_config_loop.h"

#if defined ALLEGRO_IPHONE && defined ADMOB
#include "apple.h"
#endif

bool Settings_Loop::init()
{
	if (inited) {
		return true;
	}
	Loop::init();

	audio_button = new W_Translated_Button("CONFIG_AUDIO");
	video_button = new W_Translated_Button("CONFIG_VIDEO");
	keyboard_button = new W_Translated_Button("CONFIG_KEYBOARD");
	gamepad_button = new W_Translated_Button("CONFIG_GAMEPAD");
	language_button = new W_Translated_Button("CONFIG_LANGUAGE");
	restore_button = new W_Translated_Button("RESTORE_PURCHASES");
#if !defined ALLEGRO_RASPBERRYPI
	licenses_button = new W_Translated_Button("THIRD_PARTY");
#endif

	return_button = new W_Button("misc_graphics/interface/fat_red_button.png", t("RETURN"));

	int maxw = audio_button->getWidth();
	maxw = MAX(maxw, video_button->getWidth());
	maxw = MAX(maxw, keyboard_button->getWidth());
	maxw = MAX(maxw, gamepad_button->getWidth());
	maxw = MAX(maxw, language_button->getWidth());
#if !defined ALLEGRO_RASPBERRYPI
	maxw = MAX(maxw, licenses_button->getWidth());
#endif

#ifdef ALLEGRO_IPHONE
	tgui::TGUIWidget *w[] = {
		audio_button,
		video_button,
		language_button,
		licenses_button,
		restore_button,
	};
	int nw;
#ifdef ADMOB
	if (engine->get_purchased()) {
#else
	if (true) {
#endif
		nw = 4;
	}
	else {
		nw = 5;
	}
#elif defined ALLEGRO_RASPBERRYPI
	tgui::TGUIWidget *w[] = {
		audio_button,
		video_button,
		keyboard_button,
		gamepad_button,
		language_button,
	};
	int nw = 5;
#else
	tgui::TGUIWidget *w[] = {
		audio_button,
		video_button,
		keyboard_button,
		gamepad_button,
		language_button,
		licenses_button,
	};
	int nw = 6;
#endif

	for (int i = 0; i < nw; i++) {
		w[i]->setX(cfg.screen_w/2-maxw/2);
		w[i]->setY(cfg.screen_h/2-((General::get_font_line_height(General::FONT_LIGHT)+4)*(nw+2)+5)/2+(General::get_font_line_height(General::FONT_LIGHT)+4)*i);
		tgui::addWidget(w[i]);
	}

	return_button->setX(cfg.screen_w/2-return_button->getWidth()/2);
	return_button->setY(cfg.screen_h/2-((General::get_font_line_height(General::FONT_LIGHT)+4)*(nw+2)+5)/2+(General::get_font_line_height(General::FONT_LIGHT)+4)*(nw)+5);

	tgui::addWidget(return_button);

	tgui::setFocus(return_button);

	return true;
}

void Settings_Loop::top()
{
}

bool Settings_Loop::handle_event(ALLEGRO_EVENT *event)
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

bool Settings_Loop::logic()
{
	tgui::TGUIWidget *w = tgui::update();

	if (w == audio_button) {
		std::vector<Loop *> this_loop;
		this_loop.push_back(this);
		engine->fade_out(this_loop);
		Audio_Config_Loop *l = new Audio_Config_Loop();
		tgui::hide();
		tgui::push(); // popped in ~Audio_Config_Loop()
		std::vector<Loop *> loops;
		l->init();
		loops.push_back(l);
		engine->fade_in(loops);
		engine->do_blocking_mini_loop(loops, NULL);
		if (!engine->get_done()) {
			engine->fade_in(this_loop);
		}
	}
	else if (w == video_button) {
		std::vector<Loop *> this_loop;
		this_loop.push_back(this);
		engine->fade_out(this_loop);
		Video_Config_Loop *l = new Video_Config_Loop();
		tgui::hide();
		tgui::push(); // popped in ~Video_Config_Loop()
		std::vector<Loop *> loops;
		l->init();
		loops.push_back(l);
		engine->fade_in(loops);
		engine->do_blocking_mini_loop(loops, NULL);
		if (restart_game) {
			engine->unblock_mini_loop();
			return true;
		}
		if (!engine->get_done()) {
			engine->fade_in(this_loop);
		}
	}
	else if (w == keyboard_button) {
		std::vector<Loop *> this_loop;
		this_loop.push_back(this);
		engine->fade_out(this_loop);
		Input_Config_Loop *l = new Input_Config_Loop(true);
		tgui::hide();
		tgui::push(); // popped in ~Input_Config_Loop()
		std::vector<Loop *> loops;
		l->init();
		loops.push_back(l);
		engine->fade_in(loops);
		engine->do_blocking_mini_loop(loops, NULL);
		if (!engine->get_done()) {
			engine->fade_in(this_loop);
		}
	}
	else if (w == gamepad_button) {
		std::vector<Loop *> this_loop;
		this_loop.push_back(this);
		engine->fade_out(this_loop);
		Input_Config_Loop *l = new Input_Config_Loop(false);
		tgui::hide();
		tgui::push(); // popped in ~Input_Config_Loop()
		std::vector<Loop *> loops;
		l->init();
		loops.push_back(l);
		engine->fade_in(loops);
		engine->do_blocking_mini_loop(loops, NULL);
		if (!engine->get_done()) {
			engine->fade_in(this_loop);
		}
	}
	else if (w == language_button) {
		std::vector<Loop *> this_loop;
		this_loop.push_back(this);
		engine->fade_out(this_loop);
		Language_Config_Loop *l = new Language_Config_Loop();
		tgui::hide();
		tgui::push(); // popped in ~Language_Config_Loop()
		std::vector<Loop *> loops;
		l->init();
		loops.push_back(l);
		engine->fade_in(loops);
		engine->do_blocking_mini_loop(loops, NULL);
		// Change return button
		return_button->set_text(t("RETURN"));
		if (!engine->get_done()) {
			engine->fade_in(this_loop);
		}
	}
#if defined ALLEGRO_IPHONE && defined ADMOB
	else if (w == restore_button) {
		restore_purchases();
		std::vector<std::string> v;
		if (isPurchased()) {
			engine->set_purchased(true);
			cfg.save();
			tgui::setFocus(return_button);
			restore_button->remove();
			v.push_back(t("RESTORED1"));
			v.push_back(t("RESTORED2"));
		}
		else {
			v.push_back(t("COULDNT_RESTORE1"));
			v.push_back(t("COULDNT_RESTORE2"));
		}
		engine->notify(v);
	}
#endif
#if !defined ALLEGRO_RASPBERRYPI
	else if (w == licenses_button) {
		General::show_license();
	}
#endif
	else if (w == return_button) {
		std::vector<Loop *> loops;
		loops.push_back(this);
		engine->fade_out(loops);
		engine->unblock_mini_loop();
		return true;
	}

	return false;
}

void Settings_Loop::draw()
{
	al_clear_to_color(General::UI_GREEN);

	tgui::draw();
}

Settings_Loop::Settings_Loop()
{
}

Settings_Loop::~Settings_Loop()
{
	audio_button->remove();
	delete audio_button;
	video_button->remove();
	delete video_button;
	keyboard_button->remove();
	delete keyboard_button;
	gamepad_button->remove();
	delete gamepad_button;
	language_button->remove();
	delete language_button;
	restore_button->remove();
	delete restore_button;
	return_button->remove();
	delete return_button;

	tgui::pop(); // pushed beforehand
	tgui::unhide();
}

