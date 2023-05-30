#include <cctype>
#include <cmath>
#include <cstdio>
#include <algorithm>

#ifndef ALLEGRO_WINDOWS
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#ifdef ALLEGRO_WINDOWS
#include <allegro5/allegro_direct3d.h>
#include <allegro5/allegro_windows.h>
#endif

#ifdef ALLEGRO_ANDROID
#include <allegro5/allegro_opengl.h>
#include "android.h"
#endif

#if defined ALLEGRO_IPHONE || defined ALLEGRO_MACOSX
#include "apple.h"
#endif

#ifdef ALLEGRO_MACOSX
#include <AppKit/AppKit.h>
#endif

#ifdef ALLEGRO_UNIX
#include <allegro5/allegro_x.h>
#endif

#include <tgui2.hpp>

ALLEGRO_DEBUG_CHANNEL("CrystalPicnic")

#include <bass.h>

#include "animation_set.h"
#include "area_manager.h"
#include "area_loop.h"
#include "battle_loop.h"
#include "battle_transition_in.h"
#include "bitmap.h"
#include "collision_detection.h"
#include "config.h"
#include "cpa.h"
#include "crystalpicnic.h"
#include "direct3d.h"
#include "dirty.h"
#include "engine.h"
#include "frame.h"
#include "game_specific_globals.h"
#include "general.h"
#include "graphics.h"
#include "input_config_loop.h"
#include "map_entity.h"
#include "map_loop.h"
#include "music.h"
#include "particle.h"
#include "player.h"
#include "resource_manager.h"
#include "runner_loop.h"
#include "snprintf.h"
#include "sound.h"
#include "video_player.h"
#include "widgets.h"
#include "wrap.h"

#if defined GOOGLEPLAY || defined AMAZON
#include "android.h"
#endif

#ifdef GAMECENTER
#include "gamecenter.h"
#endif

#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include "steamworks.h"
#endif

#if defined __linux__ || defined ALLEGRO_MACOSX || defined ALLEGRO_IPHONE
	#define USE_FULLSCREEN_WINDOW 1
#endif

#if !defined ALLEGRO_ANDROID && !defined ALLEGRO_IPHONE
static Wrap::Bitmap *mouse_cursor;
static ALLEGRO_MOUSE_CURSOR *system_mouse_cursor;
#endif

static Animation_Set *coin0;
static Animation_Set *coin1;
static Animation_Set *coin2;

#if defined ALLEGRO_IPHONE && defined ADMOB
extern "C" {
	int isPurchased_engine()
	{
		return engine->get_purchased();
	}
}
#endif

extern "C" {
	void show_please_connect_dialog(bool is_network_test)
	{
		std::vector<std::string> v;
		v.push_back(t("PLEASE_CONNECT"));
		v.push_back(t("TO_THE_INTERNET"));
		engine->notify(v, &engine->get_loops(), is_network_test, true);
	}
}

bool enable_press_and_hold(bool enable, ALLEGRO_DISPLAY *d)
{
#ifdef _WIN32
#define TABLET_DISABLE_PRESSANDHOLD        0x00000001
#define TABLET_DISABLE_PENTAPFEEDBACK      0x00000008
#define TABLET_DISABLE_PENBARRELFEEDBACK   0x00000010
#define TABLET_DISABLE_TOUCHUIFORCEON      0x00000100
#define TABLET_DISABLE_TOUCHUIFORCEOFF     0x00000200
#define TABLET_DISABLE_TOUCHSWITCH         0x00008000
#define TABLET_DISABLE_FLICKS              0x00010000
#define TABLET_ENABLE_FLICKSONCONTEXT      0x00020000
#define TABLET_ENABLE_FLICKLEARNINGMODE    0x00040000
#define TABLET_DISABLE_SMOOTHSCROLLING     0x00080000
#define TABLET_DISABLE_FLICKFALLBACKKEYS   0x00100000
#define TABLET_ENABLE_MULTITOUCHDATA       0x01000000

	// Toggle the press and hold gesture for the given window
	// See: https://msdn.microsoft.com/en-us/library/ms812373.aspx

	// The atom identifier and Tablet PC atom
	ATOM atomID = 0;
	LPCTSTR tabletAtom = "MicrosoftTabletPenServiceProperty";

	// Get the Tablet PC atom ID
	atomID = GlobalAddAtom(tabletAtom);

	// If getting the ID failed, return false
	if (atomID == 0) {
		return false;
	}

	// Enable or disable the press and hold gesture
	if (enable) {
		// Try to enable press and hold gesture by 
		// clearing the window property, return the result
		return RemoveProp(al_get_win_window_handle(d), tabletAtom) != 0;
	}
	else {
		// Try to disable press and hold gesture by 
		// setting the window property, return the result
		//return SetProp(al_get_win_window_handle(d), tabletAtom, (HANDLE)(TABLET_DISABLE_PRESSANDHOLD | TABLET_DISABLE_PENTAPFEEDBACK | TABLET_DISABLE_PENBARRELFEEDBACK | TABLET_DISABLE_FLICKS)) != 0;
		return SetProp(al_get_win_window_handle(d), tabletAtom, (HANDLE)1) != 0;
	}
#else
	return false;
#endif
}

#ifdef _WIN32
#include <dbghelp.h>
static LONG WINAPI unhandled_exception(_EXCEPTION_POINTERS *exception_pointers)
{
	if (engine) {
		engine->unset_dragsize();
	}

	char tmp[5000];
	ALLEGRO_PATH *user_path = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
	snprintf(tmp, 5000, "%s/crashdump.dmp", al_path_cstr(user_path, ALLEGRO_NATIVE_PATH_SEP));
	std::string appdata = tmp;
	al_destroy_path(user_path);

	HANDLE handle = ::CreateFile(
		appdata.c_str(),
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (handle != INVALID_HANDLE_VALUE) {
		MINIDUMP_EXCEPTION_INFORMATION ei;
		ei.ThreadId = GetCurrentThreadId();
		ei.ExceptionPointers = exception_pointers;
		ei.ClientPointers = FALSE;

		MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			handle,
			MiniDumpNormal,
			(exception_pointers ? &ei: NULL),
			NULL,
			NULL
		);

		CloseHandle(handle);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void start_crashdumps()
{
	SetUnhandledExceptionFilter(unhandled_exception);
}
#endif

#ifdef ALLEGRO_WINDOWS
std::string get_windows_language()
{
	LONG l = GetUserDefaultLCID();

	// returns names in Steam format

	if (
		l == 1031 ||
		l == 2055 ||
		l == 3079 ||
		l == 4103 ||
		l == 5127
	) {
		return "german";
	}
	else if (
		l == 1036 ||
		l == 2060 ||
		l == 3084 ||
		l == 4108 ||
		l == 5132
	) {
		return "french";
	}
	else if (l == 1042) {
		return "korean";
	}
	else {
		return "english";
	}
}
#endif

#ifdef ALLEGRO_IPHONE
bool gamepadConnected()
{
	return false;
}
#endif

#ifdef ALLEGRO_UNIX
#include <langinfo.h>

std::string get_linux_language()
{
	std::string str;
	if (getenv("LANG")) {
		str = getenv("LANG");
	}
	else {
		str = nl_langinfo(_NL_IDENTIFICATION_LANGUAGE);
	}

	str = str.substr(0, 2);

	// convert to steam style since that was the first one we did
	if (str == "de") {
		str = "german";
	}
	else if (str == "fr") {
		str = "french";
	}
	else if (str == "ko") {
		str = "korean";
	}
	else {
		str = "english";
	}

	return str;
}
#endif

#if defined ADMOB
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

bool network_is_connected = true;
volatile bool network_thread_is_running = false;
ALLEGRO_THREAD *network_thread = NULL;

static void *network_connection_test_thread(ALLEGRO_THREAD *thread, void *arg)
{
	network_thread_is_running = true;

	struct addrinfo *a;

	int delay = 0;

	while (al_get_thread_should_stop(thread) == false) {
		if (delay == 0) {
			a = 0;
			int result = getaddrinfo("nooskewl.ca", "80", NULL, &a);
			if (result || a == 0) {
				network_is_connected = false;
				delay = 3;
			}
			else {
				network_is_connected = true;
				delay = 60;
				freeaddrinfo(a);
			}
		}
		else {
			delay--;
			al_rest(1.0);
		}
		if (engine->get_purchased() ==  true) {
			break;
		}
	}

	network_thread_is_running = false;

	return NULL;
}

void create_network_thread()
{
	if (engine->get_purchased() ==  true) {
		return;
	}
	while (network_thread == NULL) {
		network_thread = al_create_thread(network_connection_test_thread, NULL);
		if (network_thread) {
			al_start_thread(network_thread);
			while (network_thread_is_running == false) {
				// wait
			}
		}
	}
	requestNewInterstitial();
}

void destroy_network_thread()
{
	if (network_thread != NULL) {
		al_destroy_thread(network_thread);
		network_thread = NULL;
	}
}
#endif

static void do_modal(
	ALLEGRO_EVENT_QUEUE *queue,
	ALLEGRO_COLOR clear_color, // if alpha == 1 then don't use background image
	ALLEGRO_BITMAP *background,
	bool (*callback)(tgui::TGUIWidget *widget),
	bool (*check_draw_callback)(),
	void (*before_flip_callback)(),
	void (*resize_callback)(),
	bool is_network_test = false,
	bool first_stage = false
	)
{
	ALLEGRO_BITMAP *back;
	if (clear_color.a != 0) {
		back = NULL;
	}
	else if (background) {
		back = background;
	}
	else {
		back = Graphics::clone_target();
	}

	bool lost = false;

	int redraw = 0;
	ALLEGRO_TIMER *logic_timer = al_create_timer(1.0/60.0);
	al_register_event_source(queue, al_get_timer_event_source(logic_timer));
	al_start_timer(logic_timer);

#ifdef ADMOB
	int count = 0;
#endif

#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
	int delay = 0;
#endif

	while (1) {
#ifdef STEAMWORKS
		SteamAPI_RunCallbacks();
#endif

		engine->maybe_resume_music();

		ALLEGRO_EVENT event;

		bool do_acknowledge_resize = false;

		while (!al_event_queue_is_empty(queue)) {
			al_wait_for_event(queue, &event);

			if (event.type == ALLEGRO_EVENT_JOYSTICK_AXIS && event.joystick.id && al_get_joystick_num_buttons((ALLEGRO_JOYSTICK *)event.joystick.id) == 0) {
				continue;
			}

			process_dpad_events(&event);

			if (event.type == ALLEGRO_EVENT_TIMER && event.timer.source != logic_timer) {
				continue;
			}
			else if (event.type == ALLEGRO_EVENT_TIMER) {
				redraw++;
			}
			else if (event.type == ALLEGRO_EVENT_JOYSTICK_CONFIGURATION) {
				al_reconfigure_joysticks();
			}
			else if (resize_callback && event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
				resize_callback();
			}
			else if (event.type == ALLEGRO_EVENT_DISPLAY_LOST) {
				lost = true;
			}
			else if (event.type == ALLEGRO_EVENT_DISPLAY_FOUND) {
				lost = false;
			}
			if (event.type == ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY) {
				engine->switch_mouse_in();
			}
			else if (event.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY) {
				engine->switch_mouse_out();
			}
#if defined ALLEGRO_WINDOWS
			else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
				engine->unset_dragsize();
			}
			else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
				engine->set_dragsize();
			}
#endif
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
			else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
				engine->switch_out();
				delay = 60;
			}
			else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
				engine->switch_in();
				delay = 60;
			}
			else if (event.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING) {
				engine->handle_halt(&event);
				delay = 60;
			}
			else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
				al_acknowledge_resize(engine->get_display());
				engine->resizer(false);
				delay = 60;
			}
#ifndef ADMOB
			(void)delay;
#endif
#endif

			if (engine->get_send_tgui_events()) {
				tgui::handleEvent(&event);
			}

			tgui::TGUIWidget *w = tgui::update();

			if (callback(w)) {
				goto done;
			}

			if (do_acknowledge_resize) {
				al_acknowledge_resize(engine->get_display());
				do_acknowledge_resize = false;
			}

#ifdef ADMOB
			if (is_network_test) {
				if (delay > 0) {
					delay--;
				}
				else {
					if (network_is_connected) {
						goto done;
					}
					if (first_stage == false) {
						count++;
						if (count >= 30*60) { // 30 seconds...
							goto done;
						}
					}
				}
			}
#endif
		}

		if (!lost && redraw && (!check_draw_callback || check_draw_callback())) {
			redraw = 0;

			if (engine->get_render_buffer()) {
				al_set_target_bitmap(engine->get_render_buffer()->bitmap);
			}
			else {
				al_set_target_backbuffer(engine->get_display());
			}
			
			al_clear_to_color(al_map_rgb(0, 0, 0));

			ALLEGRO_TRANSFORM t, backup;
			al_copy_transform(&backup, al_get_current_transform());
			al_identity_transform(&t);
			al_use_transform(&t);
			if (back) {
				Graphics::quick_draw(back, al_map_rgb_f(0.5f, 0.5f, 0.5f), 0, 0, 0);
			}
			else {
				al_clear_to_color(clear_color);
			}
			al_use_transform(&backup);

			tgui::draw();

#if !defined ALLEGRO_ANDROID && !defined ALLEGRO_IPHONE
#ifdef ALLEGRO_RASPBERRYPI
			if (al_is_mouse_installed() && mouse_cursor) {
#else
			if (al_is_mouse_installed() && ((al_get_display_flags(engine->get_display()) & ALLEGRO_FULLSCREEN) || (al_get_display_flags(engine->get_display()) & ALLEGRO_FULLSCREEN_WINDOW)) && mouse_cursor) {
#endif
				ALLEGRO_MOUSE_STATE mouse_state;
				al_get_mouse_state(&mouse_state);
				int mx = mouse_state.x;
				int my = mouse_state.y;
				tgui::convertMousePosition(&mx, &my);
				Graphics::quick_draw(mouse_cursor->bitmap, mx, my, 0);
			}
#endif

			if (before_flip_callback) {
				before_flip_callback();
			}

			backup_dirty_bitmaps();

			al_flip_display();
		}
	}

done:
	if (clear_color.a == 0 && !background) {
		al_destroy_bitmap(back);
	}

	al_destroy_timer(logic_timer);
}

// This should be the only thing global in the game
Engine *engine;

std::string Engine::JUMP_SWITCH_INFO_MILESTONE = "jump_switch_info";

Engine::Engine() :
	timer(0),
	tween_time_adjustment(0.0),
	switched_out(false),
	done(false),
	render_buffer(NULL),
	last_area(""),
	reset_count(false),
	game_over(false),
	lost_boss_battle(false),
	purchased(false),
	continued_or_saved(false),
	_loaded_video(NULL),
	send_tgui_events(true),
	save_number_last_used(0),
	draw_touch_controls(true),
	last_direction(-1),
	can_move(false),
	started_new_game(false),
	restart_audio(false),
	entire_translation(0),
	force_tablet(false),
	drag_w(-1),
	drag_h(-1)
{
        resource_manager = new Resource_Manager;

	fading = false;
	fade_color = al_map_rgba(0, 0, 0, 0);

	frames_drawn = 0;
	curr_fps = 0;
	slow_frames = 0;
	running_slow = false;

	translation = NULL;

	_unblock_mini_loop = false;

	milestones_held = false;

	work_bitmap = NULL;

	_shake.start = 0;
	_shake.duration = 0;
	shaking = false;

	touch_input_type = TOUCHINPUT_GUI;
	touch_input_on_bottom = true;
}

Engine::~Engine()
{
}

void Engine::delete_tweens()
{
	tweens.clear();
}

void Engine::add_tween(int id)
{
	Tween t;
	t.id = id;
	tweens.push_back(t);
}

void Engine::run_tweens()
{
	lua_State *lua_state;

	Area_Loop *area_loop = General::find_in_vector<Area_Loop *, Loop *>(loops);

	if (!area_loop) {
		Battle_Loop *bl = General::find_in_vector<Battle_Loop *, Loop *>(loops);
		if (!bl)
			return;
		lua_state = bl->get_lua_state();
	}
	else {
		Area_Manager *area = area_loop->get_area();
		lua_state = area->get_lua_state();
	}

	std::list<Tween>::iterator it;
	std::vector<Tween> tweens_to_delete;
	for (it = tweens.begin(); it != tweens.end();) {
		Tween &t = *it;
		Lua::call_lua(lua_state, "run_tween", "id>b", t.id, tween_time_adjustment);
		bool tween_done = lua_toboolean(lua_state, -1);
		lua_pop(lua_state, 1);
		if (tween_done)
			tweens_to_delete.push_back(t);
		it++;
	}

	tween_time_adjustment = 0.0;

	for (it = tweens.begin(); it != tweens.end();) {
		Tween &t = *it;
		bool found = false;
		for (size_t i = 0; i < tweens_to_delete.size(); i++) {
			if (t.id == tweens_to_delete[i].id) {
				Lua::call_lua(lua_state, "delete_tween", "i>", t.id);
				it = tweens.erase(it);
				found = true;
				break;
			}
		}
		if (!found) {
			it++;
		}
	}
}

std::vector<Loop *> &Engine::get_loops()
{
	return loops;
}

ALLEGRO_DISPLAY *Engine::get_display()
{
	return display;
}

void Engine::set_loops_delay_init(std::vector<Loop *> loops, bool destroy_old, bool delay_init)
{
	if (loops.size() == 0) {
		new_loops = old_loops[old_loops.size()-1];
		for (size_t i = 0; i < new_loops.size(); i++) {
			new_loops[i]->return_to_loop();
		}
		old_loops.pop_back();
		new_tweens = tween_stack[tween_stack.size()-1];
		tween_stack.pop_back();
		tween_time_adjustment = al_get_time() - tween_pause_times[tween_pause_times.size()-1];
		tween_pause_times.pop_back();
	}
	else {
		new_loops = loops;
		if (!delay_init) {
			for (size_t i = 0; i < new_loops.size(); i++) {
				new_loops[i]->init();
			}
		}
		new_tweens = std::list<Tween>();
	}
	if (!destroy_old) {
		old_loops.push_back(this->loops);
		tween_stack.push_back(tweens);
		/* Save current time so timing is preserved on backed up tweens */
		tween_pause_times.push_back(al_get_time());
	}
	destroy_old_loop = destroy_old;
}

void Engine::set_loops(std::vector<Loop *> loops, bool destroy_old)
{
	set_loops_delay_init(loops, destroy_old, false);
}

void Engine::set_loops_only(std::vector<Loop *> loops)
{
	this->loops = loops;
}

void Engine::remove_loop(Loop *loop, bool delete_it)
{
	for (size_t i = 0; i < loops.size(); i++) {
		if (loops[i] == loop) {
			loops.erase(loops.begin()+i);
			if (delete_it)
				loops_to_delete.push_back(loop);
			return;
		}
	}
}

void Engine::add_loop(Loop *loop)
{
	loops.push_back(loop);
}

#ifdef ALLEGRO_WINDOWS
void Engine::load_d3d_resources()
{
	if (al_get_display_flags(display) & ALLEGRO_DIRECT3D) {
		Wrap::reload_loaded_shaders();
		for (size_t i = 0; i < old_loops.size(); i++) {
			for (size_t j = 0; j < old_loops[i].size(); j++) {
				old_loops[i][j]->reload_graphics();
			}
		}
		for (size_t i = 0; i < loops.size(); i++) {
			loops[i]->reload_graphics();
		}
		Wrap::reload_loaded_bitmaps();
	}
}

void Engine::destroy_d3d_resources()
{
	if (al_get_display_flags(display) & ALLEGRO_DIRECT3D) {
		Wrap::destroy_loaded_shaders();
		for (size_t i = 0; i < old_loops.size(); i++) {
			for (size_t j = 0; j < old_loops[i].size(); j++) {
				old_loops[i][j]->destroy_graphics();
			}
		}
		for (size_t i = 0; i < loops.size(); i++) {
			loops[i]->destroy_graphics();
		}
		Wrap::destroy_loaded_bitmaps();
	}
}
#endif

void Engine::setup_screen_size()
{
	float w = al_get_display_width(display);
	float h = al_get_display_height(display);

	tgui::setScreenSize(w, h);

	if (cfg.fullscreen) {
		cfg.save_screen_w = w;
		cfg.save_screen_h = h;
	}

	float r = h / General::RENDER_H;

	if (r == (int)r) {
		cfg.linear_filtering = false;
	}

	if (cfg.linear_filtering) {
		r = (int)r;
	}

	cfg.screen_w = General::RENDER_H * w / h;
	cfg.screen_h = General::RENDER_H;

	if (cfg.linear_filtering) {
		cfg.screens_w = r;
		cfg.screens_h = r;
		tgui::setScale(w/cfg.screen_w, h/cfg.screen_h);
	}
	else {
		cfg.screens_w = r;
		cfg.screens_h = r;
		tgui::setScale(r, r);
	}

	if (cfg.screen_w * cfg.screens_w < w) {
		cfg.screen_w++;
	}

	tgui::setOffset(0, 0);

	if (render_buffer) {
		Wrap::destroy_bitmap(render_buffer);
		render_buffer = NULL;
	}

	if (cfg.linear_filtering) {
		int flags = al_get_new_bitmap_flags();
		al_set_new_bitmap_flags(ALLEGRO_NO_PRESERVE_TEXTURE | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
		render_buffer = Wrap::create_bitmap_no_preserve(
			cfg.screen_w * cfg.screens_w,
			cfg.screen_h * cfg.screens_h
		);
		al_set_new_bitmap_flags(flags);
	}

	ALLEGRO_BITMAP *target = al_get_target_bitmap();
	if (render_buffer) {
		al_set_target_bitmap(render_buffer->bitmap);
	}
	ALLEGRO_TRANSFORM t;
	al_identity_transform(&t);
	al_scale_transform(&t, r, r);
	al_use_transform(&t);
	al_clear_to_color(al_map_rgb(0x00, 0x00, 0x00));
	al_set_target_bitmap(target);

	if (work_bitmap) {
		Wrap::destroy_bitmap(work_bitmap);
		work_bitmap = NULL;
	}
	if (render_buffer) {
		work_bitmap = Wrap::create_bitmap_no_preserve(
			al_get_bitmap_width(render_buffer->bitmap),
			al_get_bitmap_height(render_buffer->bitmap)
		);
	}
	else {
		work_bitmap = Wrap::create_bitmap_no_preserve(
			w, h
		);
	}
}

bool Engine::init_allegro()
{
#if defined ADMOB && defined ALLEGRO_IPHONE
	requestNewInterstitial();
#endif

	bool hide_mouse = false;

	al_init();

	General::init_textlog();

	first_frame_time = al_get_time();

	ALLEGRO_DEBUG("Allegro init done");

	al_set_org_name("Nooskewl");
	al_set_app_name("Crystal Picnic");

#ifdef _WIN32
	start_crashdumps();
#endif

	ALLEGRO_DEBUG("App name set");

#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
	al_install_mouse();
#endif

#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID || defined ALLEGRO_WINDOWS || defined ALLEGRO_UNIX
	bool touch_ok = al_install_touch_input();
#endif

	al_install_keyboard();

	bool config_read = cfg.load();

	if (config_read == false) {
#ifdef STEAMWORKS
		std::string language = get_steam_language();
#elif defined ALLEGRO_ANDROID
		std::string language = get_android_language();
#elif defined ALLEGRO_IPHONE || defined ALLEGRO_MACOSX
		std::string language = get_apple_language();
#elif defined ALLEGRO_WINDOWS
		std::string language = get_windows_language();
#elif defined ALLEGRO_UNIX
		std::string language = get_linux_language();
#else
		std::string language = "English";
#endif
		if (language == "french") {
			cfg.language = "French";
		}
		else if (language == "german") {
			cfg.language = "German";
		}
		else if (language == "korean") {
			cfg.language = "Korean";
		}
	}

#ifdef TRANSLATION_BUILD
	cfg.language = "Korean";
#endif

	// Process command line arguments
	int index;
	if ((index = General::find_arg("-adapter")) > 0) {
		cfg.adapter = atoi(General::argv[index+1]);
		General::log_message("Using adapter " + General::itos(cfg.adapter));
		al_set_new_display_adapter(cfg.adapter);
	}
	if (General::find_arg("-windowed") > 0) {
		cfg.fullscreen = false;
	}
	else if (General::find_arg("-no-windowed") > 0) {
		cfg.fullscreen = true;
	}
	if (General::find_arg("-opengl") > 0) {
		cfg.force_opengl = true;
	}
	if (General::find_arg("-no-opengl") > 0) {
		cfg.force_opengl = false;
	}
	if (General::find_arg("-linear-filtering") > 0) {
		cfg.linear_filtering = true;
	}
	if (General::find_arg("-no-linear-filtering") > 0) {
		cfg.linear_filtering = false;
	}
	if (General::find_arg("-reverb") > 0) {
		cfg.reverb = true;
	}
	if (General::find_arg("-no-reverb") > 0) {
		cfg.reverb = false;
	}
	if ((index = General::find_arg("-width")) > 0) {
		cfg.screen_w = atoi(General::argv[index+1]);
	}
	if ((index = General::find_arg("-height")) > 0) {
		cfg.screen_h = atoi(General::argv[index+1]);
	}
	if (General::find_arg("-hide-mouse") > 0) {
		hide_mouse = true;
	}
	if (General::find_arg("-force-tablet") > 0) {
		force_tablet = true;
		hide_mouse = true;
	}

	ALLEGRO_DEBUG("Configuration loaded");

	if (al_install_joystick()) {
		General::log_message("Joystick driver installed.");
	}

	ALLEGRO_DEBUG("Input drivers installed");

	load_cpa();

	al_init_primitives_addon();
	al_init_image_addon();
	al_init_font_addon();
	al_init_ttf_addon();

	ALLEGRO_DEBUG("Addons initialized");

#if defined ALLEGRO_UNIX && !defined ALLEGRO_RASPBERRYPI
	int icon_format = al_get_new_bitmap_format();
	int icon_flags = al_get_new_bitmap_flags();
	al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
	al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
	Wrap::Bitmap *icon_tmp;
	// Workaround for IceWM which doesn't like 256x256 icons
	if (getenv("DESKTOP_SESSION") && strcasestr(getenv("DESKTOP_SESSION"), "IceWM") != NULL) {
		icon_tmp = Wrap::load_bitmap("misc_graphics/icon.png");
	}
	else {
		icon_tmp = Wrap::load_bitmap("misc_graphics/icon256.png");
	}
	al_x_set_initial_icon(icon_tmp->bitmap);
	Wrap::destroy_bitmap(icon_tmp);
	al_set_new_bitmap_format(icon_format);
	al_set_new_bitmap_flags(icon_flags);
#endif

	if (cfg.vsync) {
		al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);
	}
	else {
		al_set_new_display_option(ALLEGRO_VSYNC, 0, ALLEGRO_SUGGEST);
	}

#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
	al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS, ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE, ALLEGRO_REQUIRE);
#endif

	al_set_new_display_flags(
		ALLEGRO_PROGRAMMABLE_PIPELINE
#ifdef ALLEGRO_WINDOWS
		| (cfg.force_opengl ? ALLEGRO_OPENGL : ALLEGRO_DIRECT3D)
#endif
#if defined USE_FULLSCREEN_WINDOW
		| (cfg.fullscreen ? ALLEGRO_FULLSCREEN_WINDOW : ALLEGRO_WINDOWED)
#else
		| (cfg.fullscreen ? ALLEGRO_FULLSCREEN : ALLEGRO_WINDOWED)
#endif
	);

	ALLEGRO_DEBUG("Display flags set");

#ifdef ALLEGRO_IPHONE
	al_set_new_display_option(ALLEGRO_COLOR_SIZE, 16, ALLEGRO_REQUIRE);
#else
	al_set_new_display_option(ALLEGRO_COLOR_SIZE, 32, ALLEGRO_REQUIRE);
#endif

	display = al_create_display(cfg.screen_w, cfg.screen_h);

	if (!display) {
		cfg.screen_w = 1024;
		cfg.screen_h = 576;
		cfg.fullscreen = false;
		al_set_new_display_flags(al_get_new_display_flags() & ~ALLEGRO_FULLSCREEN);
		al_set_new_display_flags(al_get_new_display_flags() & ~ALLEGRO_FULLSCREEN_WINDOW);
		display = al_create_display(cfg.screen_w, cfg.screen_h);
	}

	if (!display) {
		General::log_message("Unable to create display.");
		return false;
	}

#ifdef ALLEGRO_WINDOWS
	enable_press_and_hold(false, display);
	set_dragsize();
#endif

	if (al_is_mouse_installed()) {
		al_set_mouse_xy(display, al_get_display_width(display)*0.85f, al_get_display_height(display)*0.85f);
	}

#if !defined ALLEGRO_ANDROID
	al_inhibit_screensaver(true);
#endif

#ifdef ALLEGRO_ANDROID
	glDisable(GL_DITHER);
#endif

#if defined USE_FULLSCREEN_WINDOW
	if (cfg.fullscreen) {
		cfg.screen_w = al_get_display_width(display);
		cfg.screen_h = al_get_display_height(display);
	}
#endif

#ifdef ALLEGRO_MACOSX
	// Allegro fullscreen window on Mac is broken
	if (cfg.fullscreen) {
		al_toggle_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, false);
		al_toggle_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, true);
	}
#endif

	al_clear_to_color(al_map_rgb_f(0, 0, 0));

	backup_dirty_bitmaps();

	al_flip_display();

	General::log_message("OpenGL: " + General::itos(cfg.force_opengl));

#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID && !defined ALLEGRO_RASPBERRYPI && !defined ALLEGRO_UNIX
	// With OpenGL driver, crashes if icon is not a memory bitmap
	int flags = al_get_new_bitmap_flags();
	al_set_new_bitmap_flags(flags | ALLEGRO_MEMORY_BITMAP);
	Wrap::Bitmap *icon_tmp = Wrap::load_bitmap("misc_graphics/icon.png");
	al_set_display_icon(display, icon_tmp->bitmap);
	Wrap::destroy_bitmap(icon_tmp);
	al_set_new_bitmap_flags(flags);
#endif

	al_set_new_bitmap_flags(ALLEGRO_NO_PRESERVE_TEXTURE);

#ifdef ALLEGRO_WINDOWS
	if (al_get_display_flags(display) & ALLEGRO_DIRECT3D) {
		al_set_d3d_device_release_callback(Direct3D::lost_callback);
		al_set_d3d_device_restore_callback(Direct3D::found_callback);
	}
#endif

	halt_mutex = al_create_mutex();
	halt_cond = al_create_cond();

#if defined ADMOB
	al_set_window_title(display, "Crystal Picnic Unleashed");
#elif defined DEMO
	al_set_window_title(display, "Crystal Picnic Demo");
#else
	al_set_window_title(display, "Crystal Picnic");
#endif

	tgui::init(display);
	ALLEGRO_DEBUG("TGUI initialized");

	setup_screen_size();

#if !defined ALLEGRO_ANDROID && !defined ALLEGRO_IPHONE
	if (hide_mouse || al_is_mouse_installed() == false) {
#ifndef ALLEGRO_RASPBERRYPI
		if (al_is_mouse_installed()) {
			al_hide_mouse_cursor(display);
		}
#endif
		mouse_cursor = 0;
		system_mouse_cursor = 0;
	}
	else {
#ifdef ALLEGRO_RASPBERRYPI
		if (true) {
#else
		if ((al_get_display_flags(display) & ALLEGRO_FULLSCREEN) || (al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW)) {
#endif
			system_mouse_cursor = 0;
			mouse_cursor = Wrap::load_bitmap("misc_graphics/mouse_cursor.png");
#ifndef ALLEGRO_RASPBERRYPI
			if (al_is_mouse_installed()) {
				al_hide_mouse_cursor(display);
			}
#endif
		}
		else {
			mouse_cursor = 0;
			Wrap::Bitmap *cursor = Wrap::load_bitmap("misc_graphics/mouse_cursor.png");
			int scale = MAX(1, al_get_display_height(display) / cfg.screen_h);
			int w = al_get_bitmap_width(cursor->bitmap);
			int h = al_get_bitmap_height(cursor->bitmap);
			for (; scale > 1; scale--) {
				if (w * scale <= 32 && h * scale <= 32) {
					break;
				}
			}
			Wrap::Bitmap *scaled = Wrap::create_bitmap(32, 32);
			ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
			al_set_target_bitmap(scaled->bitmap);
			al_clear_to_color(al_map_rgba(0, 0, 0, 0));
			Graphics::quick_draw(
				cursor->bitmap,
				0, 0,
				al_get_bitmap_width(cursor->bitmap), al_get_bitmap_height(cursor->bitmap),
				0, 0,
				scale * al_get_bitmap_width(cursor->bitmap), scale * al_get_bitmap_height(cursor->bitmap),
				0
			);
			al_set_target_bitmap(old_target);
			system_mouse_cursor = al_create_mouse_cursor(scaled->bitmap, 0, 0);
			Wrap::destroy_bitmap(scaled);
			Wrap::destroy_bitmap(cursor);
			al_set_mouse_cursor(display, system_mouse_cursor);
			al_show_mouse_cursor(display);
		}
	}
#else
	(void)hide_mouse;
#endif

#ifdef ALLEGRO_WINDOWS
	if (al_get_display_flags(display) & ALLEGRO_OPENGL) {
		load_d3d_resources();
	}
#endif

	if (al_get_display_flags(display) & ALLEGRO_OPENGL) {
		General::noalpha_bmp_format = ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE;
		General::font_bmp_format = ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE;
		General::default_bmp_format = ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE;
		cfg.opengl = true;
	}
	else {
		General::noalpha_bmp_format = ALLEGRO_PIXEL_FORMAT_XRGB_8888;
		General::font_bmp_format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
		General::default_bmp_format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
		cfg.opengl = false;
	}

	al_set_new_bitmap_format(General::default_bmp_format);

	ALLEGRO_DEBUG("Display setup done");

	event_queue = al_create_event_queue();

	timer = al_create_timer(1.0/60.0);

	al_register_event_source(event_queue, al_get_timer_event_source(timer));
	al_register_event_source(event_queue, al_get_display_event_source(display));
#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
	if (al_is_mouse_installed()) {
		al_register_event_source(event_queue, al_get_mouse_event_source());
	}
#endif
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID || defined ALLEGRO_WINDOWS || defined ALLEGRO_UNIX
	if (touch_ok) {
		al_register_event_source(event_queue, al_get_touch_input_event_source());
	}
#endif
	al_register_event_source(event_queue, al_get_keyboard_event_source());
#if !defined ALLEGRO_IPHONE
	al_register_event_source(event_queue, al_get_joystick_event_source());
#endif

	Music::init();

	// Create user settings dir
	ALLEGRO_PATH *user_path = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
	al_drop_path_tail(user_path);
	if (!al_filename_exists(al_path_cstr(user_path, ALLEGRO_NATIVE_PATH_SEP))) {
		mkdir(al_path_cstr(user_path, ALLEGRO_NATIVE_PATH_SEP), 0755);
	}
	al_destroy_path(user_path);
	user_path = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
	if (!al_filename_exists(al_path_cstr(user_path, ALLEGRO_NATIVE_PATH_SEP))) {
		mkdir(al_path_cstr(user_path, ALLEGRO_NATIVE_PATH_SEP), 0755);
	}
	al_destroy_path(user_path);

	ALLEGRO_DEBUG("Allegro initialized");

#ifdef GAMECENTER
	authenticatePlayer();
#endif

#ifdef ALLEGRO_MACOSX
	[NSApp activateIgnoringOtherApps:YES];
#endif

	return true;
}

void Engine::load_sfx()
{
	Sample s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/chest_open.ogg");
	sfx["sfx/chest_open.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/error.ogg");
	sfx["sfx/error.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/menu_select.ogg");
	sfx["sfx/menu_select.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/main_menu.ogg");
	sfx["sfx/main_menu.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/quack.ogg");
	sfx["sfx/quack.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/ribbit.ogg");
	sfx["sfx/ribbit.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/pyou.ogg");
	sfx["sfx/pyou.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/bisou.ogg");
	sfx["sfx/bisou.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/open_door.ogg");
	sfx["sfx/open_door.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/item_found.ogg");
	sfx["sfx/item_found.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/exit_battle.ogg");
	sfx["sfx/exit_battle.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/enter_area.ogg");
	sfx["sfx/enter_area.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/land.ogg");
	sfx["sfx/land.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/use_item.ogg");
	sfx["sfx/use_item.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/switch_to_bisou.ogg");
	sfx["sfx/switch_to_bisou.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/switch_to_egbert.ogg");
	sfx["sfx/switch_to_egbert.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/switch_to_frogbert.ogg");
	sfx["sfx/switch_to_frogbert.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/healing_item.ogg");
	sfx["sfx/healing_item.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/single_jump.ogg");
	sfx["sfx/single_jump.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/enemy_alerted.ogg");
	sfx["sfx/enemy_alerted.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/trip.ogg");
	sfx["sfx/trip.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/slip.ogg");
	sfx["sfx/slip.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/npc.ogg");
	sfx["sfx/npc.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/coin.ogg");
	sfx["sfx/coin.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/throw.ogg");
	sfx["sfx/throw.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/swing_weapon.ogg");
	sfx["sfx/swing_weapon.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/poison_again.ogg");
	sfx["sfx/poison_again.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/enter_battle.ogg");
	sfx["sfx/enter_battle.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/enemy_die.ogg");
	sfx["sfx/enemy_die.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/hit.ogg");
	sfx["sfx/hit.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/swing_heavy_metal.ogg");
	sfx["sfx/swing_heavy_metal.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/hit_heavy_metal.ogg");
	sfx["sfx/hit_heavy_metal.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/double_jump.ogg");
	sfx["sfx/double_jump.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/enemy_jump.ogg");
	sfx["sfx/enemy_jump.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/bomb.ogg");
	sfx["sfx/bomb.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/bow_and_arrow.ogg");
	sfx["sfx/bow_and_arrow.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/open_circle.ogg");
	sfx["sfx/open_circle.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/circle_scroll_left.ogg");
	sfx["sfx/circle_scroll_left.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/circle_scroll_right.ogg");
	sfx["sfx/circle_scroll_right.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/slash.ogg");
	sfx["sfx/slash.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/throw_ability.ogg");
	sfx["sfx/throw_ability.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/kick.ogg");
	sfx["sfx/kick.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/kick_hit.ogg");
	sfx["sfx/kick_hit.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/plant_shovel.ogg");
	sfx["sfx/plant_shovel.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/plant_seed.ogg");
	sfx["sfx/plant_seed.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/plant_pop_up.ogg");
	sfx["sfx/plant_pop_up.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/plant_pop_down.ogg");
	sfx["sfx/plant_pop_down.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/plant_fire.ogg");
	sfx["sfx/plant_fire.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/ice_blast.ogg");
	sfx["sfx/ice_blast.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/fire.ogg");
	sfx["sfx/fire.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/fire_hit.ogg");
	sfx["sfx/fire_hit.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/roll.ogg");
	sfx["sfx/roll.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/heal_cast.ogg");
	sfx["sfx/heal_cast.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/heal_drop.ogg");
	sfx["sfx/heal_drop.ogg"] = s;

	s.count = 1;
	s.looping = false;
	s.sample = Sound::load("sfx/poison_initial.ogg");
	sfx["sfx/poison_initial.ogg"] = s;
}

void Engine::destroy_sfx()
{
	std::map<std::string, Sample>::iterator it;
	for (it = sfx.begin(); it != sfx.end(); it++) {
		std::pair<const std::string, Sample> &p = *it;
		if (p.second.sample) {
			Sound::destroy(p.second.sample);
		}
	}
}

bool Engine::init()
{
	if (!init_allegro())
		return false;

#ifdef ADMOB
	create_network_thread();
#endif

	Graphics::init();

	reset_game();

	General::init();
	ALLEGRO_DEBUG("General::init done");

	load_translation();
	General::load_fonts();
	ALLEGRO_DEBUG("Translation loaded");

	al_init_user_event_source(&event_source);
	add_event_source((ALLEGRO_EVENT_SOURCE *)&event_source);

	ALLEGRO_DEBUG("Created user event source");

	load_sfx();

	ALLEGRO_DEBUG("Loaded sfx");

	ALLEGRO_DEBUG("Emoticons loaded");

	hero_shadow_bmp = Wrap::load_bitmap("misc_graphics/normal_character_shadow.png");
	big_shadow_bmp = Wrap::load_bitmap("misc_graphics/big_character_shadow.png");

#if defined ALLEGRO_ANDROID || defined ALLEGRO_IPHONE || defined ALLEGRO_WINDOWS || defined ALLEGRO_UNIX
	touch_bitmaps[TOUCH_ACTIVATE] = Wrap::load_bitmap("misc_graphics/interface/touch_ui/activate.png");
	touch_bitmaps[TOUCH_ANALOGSTICK] = Wrap::load_bitmap("misc_graphics/interface/touch_ui/analogstick.png");
	touch_bitmaps[TOUCH_MENU] = Wrap::load_bitmap("misc_graphics/interface/touch_ui/menu.png");
	touch_bitmaps[TOUCH_SPECIAL] = Wrap::load_bitmap("misc_graphics/interface/touch_ui/special.png");
	touch_bitmaps[TOUCH_SWITCH] = Wrap::load_bitmap("misc_graphics/interface/touch_ui/switch.png");
	touch_bitmaps[TOUCH_USE] = Wrap::load_bitmap("misc_graphics/interface/touch_ui/use.png");
	touch_bitmaps[TOUCH_ADVANCE] = Wrap::load_bitmap("misc_graphics/interface/touch_ui/advance.png");
	touch_bitmaps[TOUCH_ANALOGBASE] = Wrap::load_bitmap("misc_graphics/interface/touch_ui/analogbase.png");
	touch_bitmaps[TOUCH_ATTACK] = Wrap::load_bitmap("misc_graphics/interface/touch_ui/attack.png");
	touch_bitmaps[TOUCH_JUMP] = Wrap::load_bitmap("misc_graphics/interface/touch_ui/jump.png");
	helping_hand = Wrap::load_bitmap("misc_graphics/interface/touch_ui/helping_hand.png");
#endif

	// pre load often used particles
	resource_manager->reference_bitmap("misc_graphics/particles/arrow.png");
	resource_manager->reference_bitmap("misc_graphics/particles/arrow-IRONARROW.png");
	resource_manager->reference_bitmap("misc_graphics/particles/vert_arrow.png");
	resource_manager->reference_bitmap("misc_graphics/particles/vert_arrow-IRONARROW.png");
	resource_manager->reference_bitmap("misc_graphics/particles/BAT.png");
	resource_manager->reference_bitmap("misc_graphics/particles/CLEAVER.png");
	resource_manager->reference_bitmap("misc_graphics/particles/STICK.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/BAT/1.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/BAT/2.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/BAT/3.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/BAT/4.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/BAT/5.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/BAT/6.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/BAT/7.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/BAT/8.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/CLEAVER/1.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/CLEAVER/2.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/CLEAVER/3.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/CLEAVER/4.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/CLEAVER/5.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/CLEAVER/6.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/CLEAVER/7.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/CLEAVER/8.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/STICK/1.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/STICK/2.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/STICK/3.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/STICK/4.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/STICK/5.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/STICK/6.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/STICK/7.png");
	resource_manager->reference_bitmap("misc_graphics/particles/throw/STICK/8.png");
	resource_manager->reference_bitmap("misc_graphics/particles/bash/1.png");
	resource_manager->reference_bitmap("misc_graphics/particles/bash/2.png");
	resource_manager->reference_bitmap("misc_graphics/particles/bash/3.png");
	resource_manager->reference_bitmap("misc_graphics/particles/bash/4.png");
	resource_manager->reference_bitmap("misc_graphics/particles/bash/5.png");
	resource_manager->reference_bitmap("misc_graphics/particles/bash/6.png");
	resource_manager->reference_bitmap("misc_graphics/particles/bash/7.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle1.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle2.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle3.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle4.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle5.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle6.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle7.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle8.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle9.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle10.png");
	resource_manager->reference_bitmap("misc_graphics/particles/fire_particle11.png");
	resource_manager->reference_bitmap("misc_graphics/particles/ice_pellet.png");
	resource_manager->reference_bitmap("misc_graphics/particles/ice_shard1.png");
	resource_manager->reference_bitmap("misc_graphics/particles/ice_shard2.png");
	resource_manager->reference_bitmap("misc_graphics/particles/ice_shard3.png");
	resource_manager->reference_bitmap("misc_graphics/particles/petal1.png");
	resource_manager->reference_bitmap("misc_graphics/particles/petal2.png");
	resource_manager->reference_bitmap("misc_graphics/particles/petal3.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/1.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/2.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/3.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/4.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/5.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/6.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/7.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/8.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/9.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/10.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/11.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/12.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/13.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/14.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/15.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/16.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/17.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_drop/18.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_star/1.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_star/2.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_star/3.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_star/4.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_star/5.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_star/6.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_star/7.png");
	resource_manager->reference_bitmap("misc_graphics/particles/heal_star/8.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/1.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/2.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/3.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/4.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/5.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/6.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/7.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/8.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/9.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/10.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/11.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/12.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/13.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/14.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/15.png");
	resource_manager->reference_bitmap("misc_graphics/particles/acorn/16.png");

	// cache coins
	coin0 = new Animation_Set();
	coin0->load("battle/entities/coin0");
	coin1 = new Animation_Set();
	coin1->load("battle/entities/coin1");
	coin2 = new Animation_Set();
	coin2->load("battle/entities/coin2");

#ifdef ALLEGRO_IPHONE
	trophy = resource_manager->reference_bitmap("misc_graphics/trophy.png");
	achievement_time = -1000000.0;
#endif

	return true;
}

void Engine::destroy_loops()
{
	for (size_t i = 0; i < loops.size(); i++) {
		delete loops[i];
	}
	loops.clear();
	for (size_t i = 0; i < old_loops.size(); i++) {
		for (size_t j = 0; j < old_loops[i].size(); j++) {
			delete old_loops[i][j];
		}
	}
	old_loops.clear();
}

void Engine::shutdown()
{
#if defined ADMOB
	destroy_network_thread();
#endif

	cleanup_battle_transition_in();

#if !defined ALLEGRO_ANDROID && !defined ALLEGRO_IPHONE
	if (mouse_cursor) {
		Wrap::destroy_bitmap(mouse_cursor);
		mouse_cursor = 0;
	}
	if (system_mouse_cursor) {
		al_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);
		al_destroy_mouse_cursor(system_mouse_cursor);
		system_mouse_cursor = 0;
	}
#endif

	Graphics::shutdown();

	destroy_sfx();

	Wrap::destroy_bitmap(hero_shadow_bmp);
	Wrap::destroy_bitmap(big_shadow_bmp);

#if defined ALLEGRO_ANDROID || defined ALLEGRO_IPHONE || defined ALLEGRO_WINDOWS || defined ALLEGRO_UNIX
	for (int i = 0; i < TOUCH_NONE; i++) {
		Wrap::destroy_bitmap(touch_bitmaps[i]);
	}
	Wrap::destroy_bitmap(helping_hand);
#endif

	resource_manager->release_bitmap("misc_graphics/particles/arrow.png");
	resource_manager->release_bitmap("misc_graphics/particles/arrow-IRONARROW.png");
	resource_manager->release_bitmap("misc_graphics/particles/vert_arrow.png");
	resource_manager->release_bitmap("misc_graphics/particles/vert_arrow-IRONARROW.png");
	resource_manager->release_bitmap("misc_graphics/particles/BAT.png");
	resource_manager->release_bitmap("misc_graphics/particles/CLEAVER.png");
	resource_manager->release_bitmap("misc_graphics/particles/STICK.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/BAT/1.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/BAT/2.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/BAT/3.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/BAT/4.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/BAT/5.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/BAT/6.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/BAT/7.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/BAT/8.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/CLEAVER/1.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/CLEAVER/2.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/CLEAVER/3.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/CLEAVER/4.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/CLEAVER/5.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/CLEAVER/6.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/CLEAVER/7.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/CLEAVER/8.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/STICK/1.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/STICK/2.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/STICK/3.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/STICK/4.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/STICK/5.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/STICK/6.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/STICK/7.png");
	resource_manager->release_bitmap("misc_graphics/particles/throw/STICK/8.png");
	resource_manager->release_bitmap("misc_graphics/particles/bash/1.png");
	resource_manager->release_bitmap("misc_graphics/particles/bash/2.png");
	resource_manager->release_bitmap("misc_graphics/particles/bash/3.png");
	resource_manager->release_bitmap("misc_graphics/particles/bash/4.png");
	resource_manager->release_bitmap("misc_graphics/particles/bash/5.png");
	resource_manager->release_bitmap("misc_graphics/particles/bash/6.png");
	resource_manager->release_bitmap("misc_graphics/particles/bash/7.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle1.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle2.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle3.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle4.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle5.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle6.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle7.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle8.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle9.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle10.png");
	resource_manager->release_bitmap("misc_graphics/particles/fire_particle11.png");
	resource_manager->release_bitmap("misc_graphics/particles/ice_pellet.png");
	resource_manager->release_bitmap("misc_graphics/particles/ice_shard1.png");
	resource_manager->release_bitmap("misc_graphics/particles/ice_shard2.png");
	resource_manager->release_bitmap("misc_graphics/particles/ice_shard3.png");
	resource_manager->release_bitmap("misc_graphics/particles/petal1.png");
	resource_manager->release_bitmap("misc_graphics/particles/petal2.png");
	resource_manager->release_bitmap("misc_graphics/particles/petal3.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/1.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/2.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/3.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/4.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/5.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/6.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/7.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/8.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/9.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/10.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/11.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/12.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/13.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/14.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/15.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/16.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/17.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_drop/18.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_star/1.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_star/2.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_star/3.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_star/4.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_star/5.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_star/6.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_star/7.png");
	resource_manager->release_bitmap("misc_graphics/particles/heal_star/8.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/1.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/2.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/3.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/4.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/5.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/6.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/7.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/8.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/9.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/10.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/11.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/12.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/13.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/14.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/15.png");
	resource_manager->release_bitmap("misc_graphics/particles/acorn/16.png");

	delete coin0;
	delete coin1;
	delete coin2;

#ifdef ALLEGRO_IPHONE
	resource_manager->release_bitmap("misc_graphics/trophy.png");
#endif

	remove_event_source((ALLEGRO_EVENT_SOURCE *)&event_source);
	al_destroy_user_event_source(&event_source);

	if (render_buffer) {
		Wrap::destroy_bitmap(render_buffer);
		render_buffer = NULL;
	}

	Wrap::destroy_bitmap(work_bitmap);

	tgui::shutdown();

	General::shutdown();

	delete resource_manager;

	Music::stop();
	Music::shutdown();

	al_shutdown_ttf_addon();
	al_shutdown_primitives_addon();
	al_destroy_display(display);

	al_destroy_config(translation);

	delete cpa;

	al_destroy_mutex(halt_mutex);
	al_destroy_cond(halt_cond);

	if (cfg.save())
		General::log_message("Configuration saved.");
	else
		General::log_message("Warning: Configuration not saved.");

	al_ustr_free(entire_translation);

	al_uninstall_system();
}

static void destroy_event(ALLEGRO_USER_EVENT *u)
{
	delete (User_Event_Data *)u->data1;
	delete (double *)u->data2;
	delete (ALLEGRO_EVENT *)u->data3;
}

void Engine::top()
{
	for (size_t i = 0; i < loops.size(); i++) {
		loops[i]->top();
	}
}

// Handle Direct3D lost/found
void Engine::handle_display_lost()
{
#ifdef ALLEGRO_WINDOWS
	if (al_get_display_flags(display) & ALLEGRO_DIRECT3D) {
		if (Direct3D::got_display_lost) {
			Direct3D::got_display_lost = false;
		}
		if (Direct3D::got_display_found) {
			Direct3D::got_display_found = false;
			General::destroy_fonts();
			destroy_d3d_resources();
			setup_screen_size();
			load_translation();
			General::load_fonts();
			load_d3d_resources();
		}
	}
#endif
}

void Engine::do_event_loop()
{
	fade_color = al_map_rgba(0, 0, 0, 0);
	_shake.start = al_get_time();
	_shake.duration = 0.0;
	shaking = false;

	done = false;

	logic_count = 0;
	int draw_count = 0;

	engine->start_timers();

	while (!done) {
#ifdef STEAMWORKS
		SteamAPI_RunCallbacks();
#endif

		maybe_resume_music();

		top();

		bool do_acknowledge_resize = false;

loop_top1:
		ALLEGRO_EVENT event;
		al_wait_for_event(event_queue, &event);

		if (event.type == ALLEGRO_EVENT_JOYSTICK_AXIS && event.joystick.id && al_get_joystick_num_buttons((ALLEGRO_JOYSTICK *)event.joystick.id) == 0) {
			continue;
		}

		process_dpad_events(&event);

#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
		if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			done = true;
			goto end;
		}
		else
#endif
		if (event.type == ALLEGRO_EVENT_JOYSTICK_CONFIGURATION) {
			al_reconfigure_joysticks();
		}

#ifdef ALLEGRO_UNIX // touchscreen hack
		bool is_touch_begin = event.type == ALLEGRO_EVENT_TOUCH_BEGIN;
		if (is_touch_begin) {
			logic_count++;
		}
#else
		bool is_touch_begin = false;
#endif

		process_touch_input(&event);
		if (send_tgui_events) {
			tgui::handleEvent_pretransformed(&event);
		}

		for (size_t i = 0; i < loops.size(); i++) {
			if (loops[i]->handle_event(&event)) {
				break;
			}
		}
		for (size_t i = 0; i < loops.size(); i++) {
			bool bail = false;
			for (size_t j = 0; j < extra_events.size(); j++) {
				if (loops[i]->handle_event(&extra_events[j])) {
					bail = true;
					break;
				}
			}
			if (bail) {
				break;
			}
		}
		extra_events.clear();
		if (new_loops.size() > 0) {
			goto loop_end1;
		}

		if (event.type == ALLEGRO_EVENT_TIMER) {
			Game_Specific_Globals::elapsed_time += General::LOGIC_MILLIS / 1000.0;
			logic_count++;
			draw_count++;
			/* pump user events */
			std::list<ALLEGRO_EVENT *>::iterator it;
			it = timed_events.begin();
			while (it != timed_events.end()) {
				ALLEGRO_EVENT *e = *it;
				double event_time;
				event_time = *((double *)e->user.data2);
				if (al_get_time() >= event_time) {
					al_emit_user_event(&event_source, e, destroy_event);
					it = timed_events.erase(it);
				}
				else {
					break;
				}
			}
		}
		if (event.type == ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY) {
			switch_mouse_in();
		}
		else if (event.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY) {
			switch_mouse_out();
		}
#if defined ALLEGRO_WINDOWS
		else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
			unset_dragsize();
		}
		else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
			set_dragsize();
		}
#endif
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
		if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
			switch_out();
		}
		else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
			switch_in();
		}
		else if (event.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING) {
			handle_halt(&event);
		}
		else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
			al_acknowledge_resize(engine->get_display());
			engine->resizer(false);
		}
#endif

#ifdef TRANSLATION_BUILD
		if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_F1) {
			load_translation();
			General::destroy_fonts();
			General::load_fonts();
		}
#endif

		// NOT else if
		if (event.type >= ALLEGRO_GET_EVENT_TYPE('B', 'A', 'R', 'Y')) {
			al_unref_user_event(&event.user);
		}

		if (is_touch_begin == false && !al_event_queue_is_empty(event_queue)) {
			goto loop_top1;
		}

loop_end1:

		if (reset_count) {
			logic_count = General::sign(logic_count);
			reset_count = false;
		}

		if (do_acknowledge_resize) {
			al_acknowledge_resize(display);
			do_acknowledge_resize = false;
		}

		if (game_over) {
			for (size_t i = 0; i < new_loops.size(); i++) {
				delete new_loops[i];
			}
			new_loops.clear();
			break;
		}

		bool stop_draw = false;

		can_move = false;
		
		if (new_loops.size() > 0) {
			logic_count = 0;
		}
		else {
			if (running_slow && logic_count > 1) {
				logic_count = 1;
			}
			while (logic_count > 0) {
				logic();
				run_tweens(); // before logic or after?
				logic_count--;
				General::logic();
				bool bail = false;
				for (size_t i = 0; i < loops.size(); i++) {
					if ((stop_draw = loops[i]->logic())) {
						bail = true;
						break;
					}
				}
				if (bail) break;
				if (new_loops.size() > 0) {
					break;
				}
			}
		}

		if (done) {
			break;
		}

		delete_pending_loops();

		bool lost;
#ifdef ALLEGRO_WINDOWS
		handle_display_lost();
		lost = Direct3D::lost;
#else
		lost = false;
#endif

		if (!lost && !switched_out && !stop_draw && draw_count > 0 && new_loops.size() == 0) {
			draw_count = 0;

			al_clear_to_color(al_map_rgb(0, 0, 0));

			draw_all(loops, false);

			backup_dirty_bitmaps();

			al_flip_display();

			if (_loaded_video) {
				_loaded_video->reset_elapsed();
				_loaded_video = NULL;
			}

			frames_drawn++;
		}

		if (new_loops.size() > 0) {
			if (destroy_old_loop) {
				for (size_t i = 0; i < loops.size(); i++) {
					delete loops[i];
				}
			}

			loops = new_loops;
			new_loops.clear();

			tweens = new_tweens;
			new_tweens.clear();

			/* NOTE: Each loop should guard against being inited twice! */
			for (size_t i = 0; i < loops.size(); i++) {
				loops[i]->init();
			}
		}

#ifdef ADMOB
		if (engine->get_purchased() == false)
			if (network_is_connected == false) {
				show_please_connect_dialog(true);
			}
#endif
	}

#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
end:;
#endif

	if (milestone_is_complete("beat_game")) {
		cfg.beat_game = true;
	}
}

void Engine::unblock_mini_loop()
{
	_unblock_mini_loop = true;
}

void Engine::do_blocking_mini_loop(std::vector<Loop *> loops, const char *cb)
{
	_in_mini_loop = true;

	bool cb_exists = false;
	lua_State *lua_state = NULL;

	Area_Loop *al = General::find_in_vector<Area_Loop *, Loop *>(loops);
	if (al) {
		lua_state = al->get_area()->get_lua_state();
		lua_getglobal(lua_state, cb);
		cb_exists = !lua_isnil(lua_state, -1);
		lua_pop(lua_state, 1);
	}

	logic_count = 0;
	int draw_count = 0;

	while (!done) {
#ifdef STEAMWORKS
		SteamAPI_RunCallbacks();
#endif

		maybe_resume_music();

		top();

		bool do_acknowledge_resize = false;

loop_top2:
		ALLEGRO_EVENT event;
		al_wait_for_event(event_queue, &event);

		if (event.type == ALLEGRO_EVENT_JOYSTICK_AXIS && event.joystick.id && al_get_joystick_num_buttons((ALLEGRO_JOYSTICK *)event.joystick.id) == 0) {
			continue;
		}

		process_dpad_events(&event);

#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
		if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			done = true;
			goto end;
		}
		else
#endif
		if (event.type == ALLEGRO_EVENT_JOYSTICK_CONFIGURATION) {
			al_reconfigure_joysticks();
		}
		
#ifdef ALLEGRO_UNIX // touchscreen hack
		bool is_touch_begin = event.type == ALLEGRO_EVENT_TOUCH_BEGIN;
		if (is_touch_begin) {
			logic_count++;
		}
#else
		bool is_touch_begin = false;
#endif

		process_touch_input(&event);
		if (send_tgui_events) {
			tgui::handleEvent_pretransformed(&event);
		}

		for (size_t i = 0; i < loops.size(); i++) {
			if (loops[i]->handle_event(&event)) {
				break;
			}
		}
		for (size_t i = 0; i < loops.size(); i++) {
			bool bail = false;
			for (size_t j = 0; j < extra_events.size(); j++) {
				if (loops[i]->handle_event(&extra_events[j])) {
					bail = true;
					break;
				}
			}
			if (bail) {
				break;
			}
		}
		extra_events.clear();
		if (new_loops.size() > 0) {
			goto loop_end2;
		}

		if (event.type == ALLEGRO_EVENT_TIMER) {
			Game_Specific_Globals::elapsed_time += General::LOGIC_MILLIS / 1000.0;
			logic_count++;
			draw_count++;
		}
		if (event.type == ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY) {
			switch_mouse_in();
		}
		else if (event.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY) {
			switch_mouse_out();
		}
#if defined ALLEGRO_WINDOWS
		else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
			unset_dragsize();
		}
		else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
			set_dragsize();
		}
#endif
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
		if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
			switch_out();
		}
		else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
			switch_in();
		}
		else if (event.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING) {
			handle_halt(&event);
		}
		else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
			al_acknowledge_resize(engine->get_display());
			engine->resizer(false);
		}
#endif

#ifdef TRANSLATION_BUILD
		if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_F1) {
			load_translation();
			General::destroy_fonts();
			General::load_fonts();
		}
#endif

		if (event.type >= ALLEGRO_GET_EVENT_TYPE('B', 'A', 'R', 'Y')) {
			al_unref_user_event(&event.user);
		}

		if (is_touch_begin == false && !al_event_queue_is_empty(event_queue)) {
			goto loop_top2;
		}

loop_end2:

		if (reset_count) {
			logic_count = General::sign(logic_count);
			reset_count = false;
		}

		if (do_acknowledge_resize) {
			al_acknowledge_resize(display);
			do_acknowledge_resize = false;
		}

		can_move = false;
		
		if (running_slow && logic_count > 1) {
			logic_count = 1;
		}
		while (logic_count > 0) {
			logic();
			run_tweens(); // before logic or after?
			logic_count--;
			General::logic();
			for (size_t i = 0; i < loops.size(); i++) {
				loops[i]->logic();
			}
			Area_Loop *al = General::find_in_vector<Area_Loop *, Loop *>(loops);
			if (al) {
				if (cb_exists) {
					Lua::call_lua(lua_state, cb, ">b");
					bool done = lua_toboolean(lua_state, -1);
					lua_pop(lua_state, 1);
					if (done) {
						goto end;
					}
				}
			}
			if (_unblock_mini_loop) {
				_unblock_mini_loop = false;
				goto end;
			}
		}

		bool lost;
#ifdef ALLEGRO_WINDOWS
		handle_display_lost();
		lost = Direct3D::lost;
#else
		lost = false;
#endif

		if (!lost && !switched_out && draw_count > 0 && new_loops.size() == 0) {
			draw_count = 0;

			al_clear_to_color(al_map_rgb(0, 0, 0));

			draw_all(loops, false);

			backup_dirty_bitmaps();

			al_flip_display();

			if (_loaded_video) {
				_loaded_video->reset_elapsed();
				_loaded_video = NULL;
			}

			frames_drawn++;
		}
	}

end:

	for (size_t i = 0; i < loops.size(); i++) {
		delete loops[i];
	}

	mini_loops.clear();
	_in_mini_loop = false;
}

void Engine::add_event_source(ALLEGRO_EVENT_SOURCE *evt_source)
{
	al_register_event_source(event_queue, evt_source);
}

void Engine::remove_event_source(ALLEGRO_EVENT_SOURCE *evt_source)
{
	al_unregister_event_source(event_queue, evt_source);
}

static bool compare_user_events(ALLEGRO_EVENT *a, ALLEGRO_EVENT *b)
{
	double a_t = *((double *)a->user.data2);
	double b_t = *((double *)b->user.data2);
	return a_t < b_t;
}

void Engine::push_event(int type, void *data, double time)
{
	ALLEGRO_EVENT *event = new ALLEGRO_EVENT;
	event->user.type = type;
	event->user.data1 = (intptr_t)data;
	event->user.data2 = (intptr_t)(new double);
	*((double *)event->user.data2) = time;
	event->user.data3 = (intptr_t)event;
	timed_events.push_back(event);
	timed_events.sort(compare_user_events);
}

void Engine::push_event(int type, void *data)
{
	ALLEGRO_EVENT *event = new ALLEGRO_EVENT;
	event->user.type = type;
	event->user.data1 = (intptr_t)data;
	event->user.data2 = 0;
	event->user.data3 = (intptr_t)event;
	al_emit_user_event(&event_source, event, destroy_event);
}

void Engine::load_sample(std::string name, bool loop)
{
	std::map<std::string, Sample>::iterator it;
	for (it = sfx.begin(); it != sfx.end(); it++) {
		std::pair<const std::string, Sample> &p = *it;
		if (p.first == name) {
			p.second.count++;
			return;
		}
	}

	Sample s;
	s.count = 1;
	s.looping = false;
	s.sample = Sound::load(name, loop);
	if (s.sample != NULL) {
		sfx[name] = s;
	}
}

void Engine::play_sample(std::string name, float vol, float pan, float speed)
{
	if (sfx.find(name) == sfx.end()) {
		return;
	}
	Sample &s = sfx[name];
	if (s.sample) {
		Sound::play(s.sample, vol, pan, speed);
		if (s.sample->loop) {
			s.looping = true;
		}
	}
}

void Engine::stop_sample(std::string name)
{
	if (sfx.find(name) == sfx.end()) {
		return;
	}
	Sample &s = sfx[name];
	if (s.sample) {
		Sound::stop(s.sample);
		if (s.sample->loop) {
			s.looping = false;
		}
	}
}

void Engine::adjust_sample(std::string name, float vol, float pan, float speed)
{
	if (sfx.find(name) == sfx.end()) {
		return;
	}
	Sample &s = sfx[name];
	if (s.sample) {
		Sound::adjust(s.sample, vol, pan, speed);
	}
}

void Engine::destroy_sample(std::string name)
{
	std::map<std::string, Sample>::iterator it;
	for (it = sfx.begin(); it != sfx.end(); it++) {
		std::pair<const std::string, Sample> &p = *it;
		if (p.first == name) {
			p.second.count--;
			if (p.second.count == 0) {
				Sound::destroy(p.second.sample);
				sfx.erase(it);
				return;
			}
		}
	}
}

void Engine::add_flash(double start, double up, double stay, double down, ALLEGRO_COLOR color)
{
	Flash f;
	f.start = al_get_time() + start;
	f.up = up;
	f.stay = stay;
	f.down = down;
	f.color = color;
	flashes.push_back(f);
}

void Engine::shake(double start, double duration, int amount)
{
	_shake.start = start + al_get_time();
	_shake.duration = duration;
	_shake.amount = amount;
	shaking = true;
}

void Engine::end_shake()
{
	_shake.start = 0;
	_shake.duration = 0;
	Area_Loop *l = GET_AREA_LOOP;
	if (l) {
		l->get_area()->set_rumble_offset(General::Point<float>(0, 0));
	}
	else {
		Battle_Loop *loop = GET_BATTLE_LOOP;
		if (loop) {
			loop->set_rumble_offset(General::Point<float>(0, 0));
		}
	}
	shaking = false;
}

void Engine::fade(double start, double duration, ALLEGRO_COLOR color)
{
	Fade f;
	f.fade_start = start + al_get_time();
	f.fade_duration = duration;
	f.fading_to = color;
	fades.push_back(f);
}

void Engine::hold_milestones(bool hold)
{
	if (hold) {
		held_milestones = milestones;
	}
	else {
		milestones = held_milestones;
	}
	milestones_held = hold;
}

void Engine::clear_milestones()
{
	milestones.clear();
}

bool Engine::milestone_is_complete(std::string name)
{
	if (milestones_held) {
		if (held_milestones.find(name) == held_milestones.end()) {
			return false;
		}
		return held_milestones[name];
	}
	else {
		if (milestones.find(name) == milestones.end()) {
			return false;
		}
		return milestones[name];
	}
}

void Engine::set_milestone_complete(std::string name, bool complete)
{
	if (milestones_held) {
		if (complete == false) {
			std::map<std::string, bool>::iterator it;
			if ((it = held_milestones.find(name)) != held_milestones.end()) {
				held_milestones.erase(it);
			}
		}
		else {
			held_milestones[name] = complete;
		}
	}
	else {
		if (complete == false) {
			std::map<std::string, bool>::iterator it;
			if ((it = milestones.find(name)) != milestones.end()) {
				milestones.erase(it);
			}
		}
		else {
			milestones[name] = complete;
		}
	}

#if defined STEAMWORKS || defined GOOGLEPLAY || defined GAMECENTER || defined AMAZON
	if (
		milestone_is_complete("antcolony_chest1") &&
		milestone_is_complete("antcolony_chest2") &&
		milestone_is_complete("ants_chest1") &&
		milestone_is_complete("ants_chest2") &&
		milestone_is_complete("ants_chest3") &&
		milestone_is_complete("ants_chest4") &&
		milestone_is_complete("ants_chest5") &&
		milestone_is_complete("castle_banquet_hockeymask") &&
		milestone_is_complete("castle_crystal_cash") &&
		milestone_is_complete("castle_hall_sock") &&
		milestone_is_complete("castle_tower3_chest1") &&
		milestone_is_complete("castle_tower3_chest2") &&
		milestone_is_complete("caverns2_chest1") &&
		milestone_is_complete("caverns2_chest2") &&
		milestone_is_complete("caverns2_chest3") &&
		milestone_is_complete("caverns3_chest2") &&
		milestone_is_complete("caverns3_chest3") &&
		milestone_is_complete("caverns4_chest1") &&
		milestone_is_complete("caverns4_chest2") &&
		milestone_is_complete("caverns4_chest4") &&
		milestone_is_complete("caverns7_chest1") &&
		milestone_is_complete("of_chest1") &&
		milestone_is_complete("of_chest2") &&
		milestone_is_complete("of_chest3") &&
		milestone_is_complete("of_chest4") &&
		milestone_is_complete("of_chest5") &&
		milestone_is_complete("of2_chest1") &&
		milestone_is_complete("pyou2_chest1") &&
		milestone_is_complete("pyou2_chest2") &&
		milestone_is_complete("pyou2_chest3") &&
		milestone_is_complete("stonecrater1_chest1") &&
		milestone_is_complete("stonecrater2_chest1") &&
		milestone_is_complete("stonecrater2_chest2") &&
		milestone_is_complete("stonecrater2_chest3") &&
		milestone_is_complete("stonecrater2.5_chest1") &&
		milestone_is_complete("stonecrater2.5_chest2") &&
		milestone_is_complete("stonecrater2.5_chest3") &&
		milestone_is_complete("stonecrater2.5_chest4") &&
		milestone_is_complete("stonecrater4_chest1")
	) {
		achieve("chests");
	}
#endif
}

void Engine::process_touch_input(ALLEGRO_EVENT *event)
{
#ifdef ALLEGRO_RASPBERRYPI
	if (!al_is_mouse_installed()) {
		return;
	}
#endif
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID || defined ALLEGRO_WINDOWS || defined ALLEGRO_UNIX
#if defined ALLEGRO_WINDOWS || defined ALLEGRO_UNIX
	if (engine->forced_tablet() && (
#else
	if ((
#endif
	event->type == ALLEGRO_EVENT_TOUCH_BEGIN ||
	event->type == ALLEGRO_EVENT_TOUCH_END ||
	event->type == ALLEGRO_EVENT_TOUCH_MOVE)) {
		int tmp_x = event->touch.x;
		int tmp_y = event->touch.y;
		tgui::convertMousePosition(&tmp_x, &tmp_y);
		event->touch.x = tmp_x;
		event->touch.y = tmp_y;
		if (touch_input_type == TOUCHINPUT_GUI) {
			if (event->type == ALLEGRO_EVENT_TOUCH_BEGIN) {
				event->type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN;
			}
			else if (event->type == ALLEGRO_EVENT_TOUCH_END) {
				event->type = ALLEGRO_EVENT_MOUSE_BUTTON_UP;
			}
			else {
				event->type = ALLEGRO_EVENT_MOUSE_AXES;
			}
			event->mouse.x = tmp_x;
			event->mouse.y = tmp_y;
		}
		else {
			update_touch(event);
		}
	}
#endif
#if !defined ALLEGRO_ANDROID && !defined ALLEGRO_IPHONE
	else if (
	event->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN ||
	event->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP ||
	event->type == ALLEGRO_EVENT_MOUSE_AXES) {
		int tmp_x = event->mouse.x;
		int tmp_y = event->mouse.y;
		tgui::convertMousePosition(&tmp_x, &tmp_y);
		event->mouse.x = tmp_x;
		event->mouse.y = tmp_y;
	}
#endif
}

void Engine::post_draw()
{
	// draw flashes
	double now = al_get_time();
	for (size_t i = 0; i < flashes.size(); i++) {
		Flash &f = flashes[i];
		double end_time =
			f.start + f.up + f.stay + f.down;
		if (f.start > now)
			continue;
		else if (end_time < now) {
			flashes.erase(flashes.begin()+i);
			i--;
			continue;
		}
		float a;
		float r, g, b, target_a;
		al_unmap_rgba_f(
			f.color,
			&r, &g, &b, &target_a
		);
		if (now < f.start + f.up) {
			double elapsed = now - f.start;
			a = elapsed / f.up;
			a *= target_a;
		}
		else if (now < f.start + f.up + f.stay) {
			a = target_a;
		}
		else {
			double elapsed = now - (f.start + f.up + f.stay);
			a = 1.0f - (elapsed / f.down);
			a *= target_a;
		}
		r *= a;
		g *= a;
		b *= a;
		al_draw_filled_rectangle(
			0, 0,
			al_get_display_width(display), al_get_display_height(display),
			al_map_rgba_f(r, g, b, a)
		);
	}

	now = al_get_time();
	Fade fade = { 0, };
	if (fades.size() > 0)
		fade = fades[0];
	if (fading && now > fade.fade_start+fade.fade_duration) {
		fading = false;
		fade_color = fade.fading_to;
		fades.erase(fades.begin());
	}
	if (fades.size() > 0) {
		fade = fades[0];
		fading = true;
	}
	ALLEGRO_COLOR fade_tmp;
	if (fading && now >= fade.fade_start) {
		float p = (now - fade.fade_start) / fade.fade_duration;
		float r1, g1, b1, a1;
		float r2, g2, b2, a2;
		al_unmap_rgba_f(fade_color, &r1, &g1, &b1, &a1);
		al_unmap_rgba_f(fade.fading_to, &r2, &g2, &b2, &a2);
		float dr = r2 - r1;
		float dg = g2 - g1;
		float db = b2 - b1;
		float da = a2 - a1;
		float r, g, b, a;
		r = r1 + (p * dr);
		g = g1 + (p * dg);
		b = b1 + (p * db);
		a = a1 + (p * da);
		fade_tmp = al_map_rgba_f(r, g, b, a);
	}
	else
		fade_tmp = fade_color;
	float r, g, b, a;
	al_unmap_rgba_f(fade_tmp, &r, &g, &b, &a);
	r *= a;
	g *= a;
	b *= a;

	if (a <= 0)
		return;

	al_draw_filled_rectangle(
		0, 0,
		al_get_display_width(display), al_get_display_height(display),
		fade_tmp
	);
}

ALLEGRO_BITMAP *Engine::set_draw_target(bool force_no_target_change)
{
	ALLEGRO_BITMAP *old_target = NULL;

	if (!force_no_target_change) {
		old_target = al_get_target_bitmap();
		if (render_buffer) {
			al_set_target_bitmap(render_buffer->bitmap);
		}
	}

	return old_target;
}

void Engine::finish_draw(bool force_no_target_change, ALLEGRO_BITMAP *old_target)
{
	if (!force_no_target_change) {
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID || defined ALLEGRO_WINDOWS || defined ALLEGRO_UNIX
		ALLEGRO_TRANSFORM backup_transform;
#endif
		if (render_buffer) {
			int w = al_get_bitmap_width(old_target);
			int h = al_get_bitmap_height(old_target);
			al_set_target_bitmap(old_target);
			Graphics::quick_draw(
				render_buffer->bitmap,
				0, 0,
				al_get_bitmap_width(render_buffer->bitmap),
				al_get_bitmap_height(render_buffer->bitmap),
				0, 0,
				w, h,
				0
			);
		}
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID || defined ALLEGRO_WINDOWS || defined ALLEGRO_UNIX
		else {
			al_copy_transform(&backup_transform, al_get_current_transform());
			ALLEGRO_TRANSFORM t;
			al_identity_transform(&t);
			al_use_transform(&t);
		}
#endif
#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID || defined ALLEGRO_WINDOWS || defined ALLEGRO_UNIX
#if defined ALLEGRO_WINDOWS || defined ALLEGRO_UNIX
		if (engine->forced_tablet() && draw_touch_controls) {
#else
		if (draw_touch_controls && !gamepadConnected()) {
#endif
			switch (touch_input_type) {
				case TOUCHINPUT_GUI:
					// nothing
					break;
				case TOUCHINPUT_SPEECH:
					draw_touch_input_button(3, TOUCH_ADVANCE);
					break;
				case TOUCHINPUT_BATTLE: {
					Battle_Loop *loop = GET_BATTLE_LOOP;
					if (!loop) {
						loop = General::find_in_vector<Battle_Loop *, Loop *>(mini_loops);
					}
					if (loop) {
						for (int i = 0; i < 4; i++) {
							Player *player = loop->get_active_player()->get_player();
							if (!player) {
								continue;
							}
							TouchType type;
							std::string ability = player->get_selected_abilities(true, dynamic_cast<Runner_Loop *>(loop), loop->is_cart_battle())[i];
							if (ability == "") {
								continue;
							}
							if (ability == "ATTACK") {
								type = TOUCH_ATTACK;
							}
							else if (ability == "USE") {
								type = TOUCH_USE;
							}
							else if (ability == "JUMP") {
								type = TOUCH_JUMP;
							}
							else {
								type = TOUCH_SPECIAL;
							}
							draw_touch_input_button(i, type);
						}
						if (!dynamic_cast<Runner_Loop *>(loop) && !loop->is_cart_battle()) {
							draw_touch_input_button(5, TOUCH_SWITCH);
						}
						draw_touch_input_stick();
					}

					// Draw "help" for first battle

					Battle_Loop *bl = GET_BATTLE_LOOP;

					if (bl && !dynamic_cast<Runner_Loop *>(bl) && !engine->milestone_is_complete("heading_west_guards")) {
						float f = fmod(al_get_time(), 4);
						int dx, dy;
						if (f < 1) {
							dx = 0;
							dy = -1;
						}
						else if (f < 2) {
							dx = 1;
							dy = 0;
							f -= 1;
						}
						else if (f < 3) {
							dx = 0;
							dy = 1;
							f -= 2;
						}
						else {
							dx = -1;
							dy = 0;
							f -= 3;
						}

						float x = 50 + dx * 50 * f;
						float y = 100 + dy * 50 * f;

						Wrap::Bitmap *bmp = helping_hand;
						int bmpw = al_get_bitmap_width(bmp->bitmap);
						int bmph = al_get_bitmap_height(bmp->bitmap);

						float screen_w = al_get_display_width(display);
						float screen_h = al_get_display_height(display);
						float scalex = screen_w / cfg.screen_w;
						float scaley = screen_h / cfg.screen_h;

						int height = scaley * TOUCH_BUTTON_RADIUS * 2;

						ALLEGRO_COLOR tint = al_map_rgba_f(0.5f, 0.5f, 0.5f, 0.5f);

						Graphics::quick_draw(
							bmp->bitmap,
							tint,
							0, 0, bmpw, bmph,
							x * scalex,
							y * scaley,
							scaley * TOUCH_BUTTON_RADIUS * 2,
							height,
							0
						);
					}

					break;
				}
				case TOUCHINPUT_MAP: {
					draw_touch_input_button(3, TOUCH_ADVANCE);
					draw_touch_input_button(4, TOUCH_MENU);
					draw_touch_input_stick();
					break;
				}
				case TOUCHINPUT_AREA: {
					if (can_move) {
						draw_touch_input_button(1, TOUCH_SPECIAL);
						draw_touch_input_button(3, TOUCH_ACTIVATE);
						draw_touch_input_button(4, TOUCH_MENU);
						draw_touch_input_button(5, TOUCH_SWITCH);
						draw_touch_input_stick();
					}
					break;
				}
			}
		}
#ifdef ALLEGRO_IPHONE
		const double display_time = 2.0;
		const double half_display_time = display_time / 2.0;
		if (achievement_time-al_get_time() > -display_time) {
			ALLEGRO_BITMAP *target = al_get_target_bitmap();
			int w = al_get_bitmap_width(target);
			int h = al_get_bitmap_height(target);
			float t = float(al_get_time() - achievement_time); // 0-2
			float scale = h / al_get_bitmap_height(trophy->bitmap);
			float alpha = (t <= half_display_time ? t/half_display_time : (half_display_time-(t-half_display_time))/half_display_time);
			ALLEGRO_COLOR tint = al_map_rgba_f(alpha, alpha, alpha, alpha);
			al_draw_tinted_scaled_rotated_bitmap(
				trophy->bitmap,
				tint,
				al_get_bitmap_width(trophy->bitmap)/2,
				al_get_bitmap_height(trophy->bitmap)/2,
				w/2,
				h/2,
				(t*scale)+0.001f,
				(t*scale)+0.001f,
				0.0f,
				0
			);
		}
#endif
		if (!render_buffer) {
			al_use_transform(&backup_transform);
		}
#endif
#if !defined ALLEGRO_ANDROID && !defined ALLEGRO_IPHONE
#ifdef ALLEGRO_RASPBERRYPI
		if (al_is_mouse_installed() && mouse_cursor) {
#else
		if (al_is_mouse_installed() && ((al_get_display_flags(engine->get_display()) & ALLEGRO_FULLSCREEN) || (al_get_display_flags(engine->get_display()) & ALLEGRO_FULLSCREEN_WINDOW)) && mouse_cursor) {
#endif
			ALLEGRO_TRANSFORM identity, backup;
			al_identity_transform(&identity);
			al_copy_transform(&backup, al_get_current_transform());
			al_use_transform(&identity);
			ALLEGRO_MOUSE_STATE mouse_state;
			al_get_mouse_state(&mouse_state);
			int mx = mouse_state.x;
			int my = mouse_state.y;
			//tgui::convertMousePosition(&mx, &my);
			int w = al_get_bitmap_width(mouse_cursor->bitmap);
			int h = al_get_bitmap_height(mouse_cursor->bitmap);
			Graphics::quick_draw(mouse_cursor->bitmap, 0, 0, w, h, mx, my, w*cfg.screens_w, h*cfg.screens_h, 0);
			bool was_quick = Graphics::is_quick_on();
			Graphics::quick(false);
			Graphics::quick(was_quick);
			al_use_transform(&backup);
		}
#endif
	}
}

void Engine::draw_to_display(ALLEGRO_BITMAP *cfg_screen_sized_bitmap)
{
	ALLEGRO_BITMAP *save = al_get_target_bitmap();
	al_set_target_backbuffer(display);
	
	al_clear_to_color(al_map_rgb(0, 0, 0));

	ALLEGRO_BITMAP *old_target = set_draw_target(false);

	Graphics::quick_draw(cfg_screen_sized_bitmap, 0, 0, 0);

	finish_draw(false, old_target);

	al_set_target_bitmap(save);
}

void Engine::draw_all(std::vector<Loop *> loops, bool force_no_target_change)
{
	ALLEGRO_BITMAP *old_target = set_draw_target(force_no_target_change);

	Graphics::ttf_quick(true);

	draw_loops(loops);
	post_draw();
	post_draw_loops(loops);

	Graphics::ttf_quick(false);

	// draw fps
	if (cfg.show_fps) {
		double now = al_get_time();
		double elapsed = now - first_frame_time;
		if (elapsed >= 1) {
			curr_fps = round(frames_drawn / elapsed);
			frames_drawn = 0;
			first_frame_time = now;

			if (curr_fps < 15) {
				if (slow_frames < 5) {
					slow_frames++;
				}
			}
			else {
				if (slow_frames > 0) {
					slow_frames--;
				}
			}

			if (running_slow) {
				if (slow_frames == 0) {
					running_slow = false;
				}
			}
			else {
				if (slow_frames == 5) {
					// NOTE: this could be useful?
					//running_slow = true;
				}
			}
		}
		General::draw_text(
			General::itos(curr_fps),
			1, 1,
			0,
			General::FONT_HEAVY
		);
	}

	finish_draw(force_no_target_change, old_target);

	/* started in logic */
	Area_Loop *l = GET_AREA_LOOP;
	if (l) {
		l->get_area()->set_rumble_offset(General::Point<float>(0, 0));
	}
}

void Engine::draw_loops(std::vector<Loop *> loops)
{
	for (size_t i = 0; i < loops.size(); i++) {
		loops[i]->draw();
	}
}

void Engine::post_draw_loops(std::vector<Loop *> loops)
{
	for (size_t i = 0; i < loops.size(); i++) {
		loops[i]->post_draw();
	}
}

void Engine::delete_pending_loops()
{
	if (loops_to_delete.size() > 0) {
		for (size_t i = 0; i < loops_to_delete.size(); i++) {
			delete loops_to_delete[i];
		}
		loops_to_delete.clear();
	}
}

void Engine::load_translation()
{
	char modded[5000];

	if (entire_translation != 0) {
		al_ustr_free(entire_translation);
	}
	entire_translation = al_ustr_new("");

	ALLEGRO_DEBUG("In load_translation");

	if (translation) {
		al_destroy_config(translation);
	}

#ifdef TRANSLATION_BUILD_XXX
	ALLEGRO_FILE *f = al_fopen("TRANSLATION.utf8", "rb");
#else
	ALLEGRO_FILE *f = cpa->load("text/" + cfg.language + ".utf8");
#endif
	translation = al_load_config_file_f(f);
	al_fclose(f);
	f = cpa->load("text/" + cfg.language + ".utf8");
	ALLEGRO_CONFIG *copy = al_load_config_file_f(f);
	al_fclose(f);

	ALLEGRO_CONFIG_ENTRY *it;
	char const *key = al_get_first_config_entry(
		copy,
		NULL,
		&it
	);

	while (key) {
		const char *txt = al_get_config_value(
			translation,
			NULL,
			key
		);

		al_ustr_append_cstr(entire_translation, txt);

		if (txt[0] == '"') {
			strcpy(modded, txt+1);
			if (modded[strlen(modded)-1] == '"') {
				modded[strlen(modded)-1] = '\0';
				al_set_config_value(
					translation,
					NULL,
					key,
					modded
				);
			}
		}
		key = al_get_next_config_entry(&it);
	}

	al_destroy_config(copy);

	al_ustr_append_cstr(entire_translation, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789`~!@#$%^&*()_-+={[}]|\\;:'\",<.>/?©é");

	//al_get_ustr_width(General::get_font(General::FONT_LIGHT), all);
	//al_get_ustr_width(General::get_font(General::FONT_HEAVY), all);
	//al_ustr_free(all);
}

// Below doesn't want convenience 't' defined (see engine.h at the end)
#undef t

const char *Engine::t(const char *tag)
{
	static char missing[100];

	if (tag[0] == 0 || translation == NULL) {
		missing[0] = 0;
		return missing;
	}

	const char *str = al_get_config_value(
		translation,
		NULL,
		tag
	);

	if (str) {
		return str;
	}

	snprintf(missing, 100, "***%s***", tag);
	return missing;
}

static bool halt_ended = false;
struct Wait_Info {
	ALLEGRO_EVENT_QUEUE *queue;
	Engine *engine;
};

void *wait_for_drawing_resume(void *arg)
{
	Wait_Info *i = (Wait_Info *)arg;
	ALLEGRO_EVENT_QUEUE *queue = i->queue;
	Engine *engine = i->engine;

	while (1) {
		ALLEGRO_EVENT event;
		al_wait_for_event(queue, &event);
		if (event.type == ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING) {
			break;
		}
		else {
			if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
				engine->switch_in(true);
#ifdef ALLEGRO_WINDOWS
				engine->set_dragsize();
#endif
			}
			else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
				engine->switch_out();
#ifdef ALLEGRO_WINDOWS
				engine->unset_dragsize();
#endif
			}
			else if (event.type == ALLEGRO_EVENT_JOYSTICK_CONFIGURATION) {
				al_reconfigure_joysticks();
			}
			else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
				al_acknowledge_resize(engine->get_display());
			}
		}
	}

	halt_ended = true;
	al_broadcast_cond(engine->get_halt_cond());

	return NULL;
}

void Engine::switch_out()
{
#if defined ADMOB
	destroy_network_thread();
#endif

#if defined ALLEGRO_ANDROID || defined ALLEGRO_IPHONE
	cfg.save();
#endif

	stop_timers();

	switched_out = true;

	switch_music_out();
}

void Engine::switch_in(bool in_halt)
{
#if defined ADMOB
	if (in_halt == false) {
		create_network_thread();
	}
#endif

	start_timers();

	switched_out = false;

	restart_audio = true;
}

void Engine::resizer(bool is_handle_halt)
{
#if defined ADMOB
	destroy_network_thread();
#endif

	stop_timers();

#ifdef ALLEGRO_ANDROID
	Wrap::destroy_loaded_shaders();
	for (size_t i = 0; i < old_loops.size(); i++) {
		for (size_t j = 0; j < old_loops[i].size(); j++) {
			old_loops[i][j]->destroy_graphics();
		}
	}
	for (size_t i = 0; i < loops.size(); i++) {
		loops[i]->destroy_graphics();
	}
	General::destroy_fonts();
	Wrap::destroy_loaded_bitmaps();
	//switch_music_out();

	if (render_buffer) {
		Wrap::destroy_bitmap(render_buffer);
		render_buffer = NULL;
	}
	if (work_bitmap) {
		Wrap::destroy_bitmap(work_bitmap);
		work_bitmap = NULL;
	}
#endif

	if (is_handle_halt) {
		al_acknowledge_drawing_halt(display);

		Wait_Info i;
		i.queue = event_queue;
		i.engine = this;

		al_run_detached_thread(wait_for_drawing_resume, &i);

		al_lock_mutex(halt_mutex);
		while (!halt_ended) {
			al_wait_cond(halt_cond, halt_mutex);
		}
		al_unlock_mutex(halt_mutex);

		halt_ended = false;

		al_acknowledge_drawing_resume(display);
	}

	start_timers();

#ifdef ALLEGRO_ANDROID
	glDisable(GL_DITHER);

	setup_screen_size();

	Wrap::reload_loaded_shaders();
	for (size_t i = 0; i < old_loops.size(); i++) {
		for (size_t j = 0; j < old_loops[i].size(); j++) {
			old_loops[i][j]->reload_graphics();
		}
	}
	for (size_t i = 0; i < loops.size(); i++) {
		loops[i]->reload_graphics();
	}
	load_translation();
	General::load_fonts();
	Wrap::reload_loaded_bitmaps();
	//restart_audio = true;
#endif

#if defined ADMOB
	create_network_thread();
#endif
}

void Engine::handle_halt(ALLEGRO_EVENT *event)
{
#ifdef ALLEGRO_ANDROID
	// This is a hack for Android... to avoid a race condition.
	// Display nativeOnDestroy waits on a condition but al_acknowledge_drawing_halt can be called before that
	// in resizer below so we wait

	al_rest(0.5);
#endif

	resizer(true);
}

void Engine::stop_timers()
{
	if (timer) {
		al_stop_timer(timer);
	}
}

void Engine::start_timers()
{
	if (timer) {
		al_start_timer(timer);
	}
}

int Engine::add_particle_group(std::string type, int layer, int align, std::vector<std::string> bitmap_names)
{
	Particle::Particle_Group *pg = new Particle::Particle_Group(type, layer, align, bitmap_names);
	particle_groups.push_back(pg);
	return pg->id;
}

void Engine::delete_particle_group(int id)
{
	for (size_t i = 0; i < particle_groups.size(); i++) {
		if (particle_groups[i]->id == id) {
			Particle::Particle_Group *pg = particle_groups[i];
			particle_groups.erase(particle_groups.begin()+i);
			delete pg;
		}
	}
}

Particle::Particle_Group *Engine::get_particle_group(int id)
{
	for (size_t i = 0; i < particle_groups.size(); i++) {
		if (particle_groups[i]->id == id) {
			return particle_groups[i];
		}
	}

	return NULL;
}

// -1 == boss save
void Engine::save_game(int number)
{
	General::log_message("Saving game " + General::itos(number));

	// FIXME: update save state version here
	Lua::set_save_state_version(1);

	bool is_map = false;
	std::string area_name;
	Area_Manager *area = NULL;

	/* Saving game happens from menu so Area_Loop is in the old_loops stack.
	 * Search it from back (newest) to front
	 */
	Area_Loop *l = NULL;
	Map_Loop *ml = NULL;
	for (int i = old_loops.size()-1; i >= 0; i--) {
		l = General::find_in_vector<Area_Loop *, Loop *>(old_loops[i]);
		if (l) {
			is_map = false;
			area = l->get_area();
			area_name = area->get_name();
			break;
		}
	}

	if (l == NULL) {
		l = General::find_in_vector<Area_Loop *, Loop *>(loops);

		if (l == NULL) {
			// We must be in a map, get its name
			is_map = true;
			for (int i = old_loops.size()-1; i >= 0; i--) {
				ml = General::find_in_vector<Map_Loop *, Loop *>(old_loops[i]);
				if (ml) {
					break;
				}
			}
			if (!ml) {
				ml = General::find_in_vector<Map_Loop *, Loop *>(loops);
			}
			if (ml) {
				area_name = "map:" + ml->get_current_location_name();
			}
		}
		else {
			is_map = false;
			area = l->get_area();
			area_name = area->get_name();
		}
	}

	Lua::clear_saved_lua_lines();

	char line[1000];

	// NOTE: Only Egbert+Frogbert and Egbert+Frogbert+Bisou (in any order) supported
	std::vector<Player *> players;

	if (is_map) {
		players = ml->get_players();
	}
	else {
		players = l->get_players();
	}

	if (players.size() == 2) {
		snprintf(line, 1000, "set_saved_players(\"%s\", \"%s\")\n",
			players[0]->get_name().c_str(), players[1]->get_name().c_str());
	}
	else {
		snprintf(line, 1000, "set_saved_players(\"%s\", \"%s\", \"%s\")\n",
			players[0]->get_name().c_str(), players[1]->get_name().c_str(), players[2]->get_name().c_str());
	}
	Lua::add_saved_lua_line(line);

	snprintf(line, 1000, "set_elapsed_time(%f)\n", Game_Specific_Globals::elapsed_time);
	Lua::add_saved_lua_line(line);
	snprintf(line, 1000, "set_cash(%d)\n", Game_Specific_Globals::cash);
	Lua::add_saved_lua_line(line);
	snprintf(line, 1000, "add_crystals(%d)\n", Game_Specific_Globals::crystals);
	Lua::add_saved_lua_line(line);

	std::vector<Game_Specific_Globals::Item> &items = Game_Specific_Globals::get_items();
	std::vector<Equipment::Weapon> &weapons = Game_Specific_Globals::get_weapons();
	std::vector<Equipment::Armor> &armor = Game_Specific_Globals::get_armor();
	std::vector<Equipment::Accessory> &accessories = Game_Specific_Globals::get_accessories();

	for (size_t i = 0; i < items.size(); i++) {
		snprintf(line, 1000, "give_items(\"%s\", %d)\n", items[i].name.c_str(), items[i].quantity);
		Lua::add_saved_lua_line(line);
	}
	for (size_t i = 0; i < weapons.size(); i++) {
		snprintf(line, 1000, "give_equipment(WEAPON, \"%s\", %d)\n", weapons[i].name.c_str(), weapons[i].quantity);
		Lua::add_saved_lua_line(line);
	}
	for (size_t i = 0; i < armor.size(); i++) {
		snprintf(line, 1000, "give_equipment(ARMOR, \"%s\", %d)\n", armor[i].name.c_str(), 1);
		Lua::add_saved_lua_line(line);
	}
	for (size_t i = 0; i < accessories.size(); i++) {
		snprintf(line, 1000, "give_equipment(ACCESSORY, \"%s\", %d)\n", accessories[i].name.c_str(), 1);
		Lua::add_saved_lua_line(line);
	}

	snprintf(line, 1000, "difficulty(%d)\n", (int)cfg.difficulty);
	Lua::add_saved_lua_line(line);

	Lua::store_battle_attributes(players);
	Lua::add_battle_attributes_lines();

	if (!is_map) {
		lua_State *lua_state = area->get_lua_state();

		lua_getglobal(lua_state, "is_dungeon");
		bool exists = !lua_isnil(lua_state, -1);
		lua_pop(lua_state, 1);

		General::Point<float> pos;

		if (exists) {
			Map_Entity *player = area->get_entity(0);
			pos = player->get_position();
			snprintf(line, 1000, "restore_item(SAVE_PLAYER, %d, %f, %f)\n", player->get_layer(), pos.x, pos.y);
			Lua::add_saved_lua_line(line);
			/* NOTE: When this is called, the area loop is not in main loops so most lua callbacks won't work */
			Lua::call_lua(area->get_lua_state(), "save_level_state", ">");
		}
		else {
			pos = area->get_player_start_position();
			snprintf(line, 1000, "restore_item(SAVE_PLAYER, %d, %f, %f)\n", area->get_player_start_layer(), pos.x, pos.y);
			Lua::add_saved_lua_line(line);
		}
	}

	std::string save_filename = General::get_save_filename(number);

	ALLEGRO_FILE *f = al_fopen(save_filename.c_str(), "w");

	if (f) {
		char buf[2000];
		snprintf(buf, 2000, "set_save_state_version(%d)\n", Lua::get_save_state_version());
		al_fputs(f, buf);

		snprintf(buf, 2000, "set_area_name(\"%s\")\n", area_name.c_str());
		al_fputs(f, buf);

		std::map<std::string, bool>::iterator it;
		for (it = milestones.begin(); it != milestones.end(); it++) {
			std::pair<const std::string, bool> &p = *it;
			snprintf(buf, 2000, "set_milestone_complete(\"%s\", %s)\n",
				p.first.c_str(),
				p.second ? "true" : "false"
			);
			al_fputs(f, buf);
		}
		Lua::write_saved_lua_lines(f);
		al_fclose(f);
	}

	if (number != -1) {
		save_number_last_used = number;
	}
}

#ifdef STEAMWORKS //_XXX_DONT_DO_THIS
static void maybe_force_achievement(std::string name)
{
	bool complete = engine->milestone_is_complete(name);
	if (complete) {
		steam_log_string(name + " complete, forcing");
		engine->set_milestone_complete(name, false);
		achieve(name);
		engine->set_milestone_complete(name, true);
	}
	else {
		steam_log_string(name + " not complete, not forcing");
	}
}
#endif

// -1 == boss save
void Engine::load_game(int number)
{
	General::log_message("Loading game " + General::itos(number));

	// If there isn't a line, we need to set version 1
	Lua::set_save_state_version(1);

	Lua::clear_before_load();

	// Initialize stuff

	std::string save_filename = General::get_save_filename(number);

	lua_State *lua_state = luaL_newstate();
	Lua::open_lua_libs(lua_state);
	Lua::register_c_functions(lua_state);

	Lua::load_global_scripts(lua_state);

	if (!luaL_loadfile(lua_state, save_filename.c_str())) {
		started_new_game = false;
		if (lua_pcall(lua_state, 0, 0, 0)) {
			Lua::dump_lua_stack(lua_state);
		}
	}
	else {
		started_new_game = true;
	}

	lua_close(lua_state);

	game_just_loaded = true;

	if (number != -1) {
		save_number_last_used = number;
	}

#ifdef STEAMWORKS //_XXX_DONT_DO_THESE_CAUSE_LOADING_GAME_WOULD_UNLOCK_ALL_ACHIEVEMENTS
	maybe_force_achievement("chests");
	maybe_force_achievement("credits");
	maybe_force_achievement("bonus");
	maybe_force_achievement("crystals");
	maybe_force_achievement("crystal");
	maybe_force_achievement("whack");
	maybe_force_achievement("attack");
	maybe_force_achievement("defense");
	maybe_force_achievement("egberts");
	maybe_force_achievement("frogberts");
#endif
}

void Engine::reset_game()
{
	Lua::reset_game();
	Lua::init_battle_attributes();
}

void Engine::logic()
{
	Area_Loop *l = GET_AREA_LOOP;
	double now = al_get_time();
        if (now >= _shake.start && now <= _shake.start+_shake.duration) {
		int x = General::rand() % (_shake.amount);
		int y = General::rand() % (_shake.amount);
		if (l) {
			l->get_area()->set_rumble_offset(General::Point<float>(x, y));
		}
		else {
			Battle_Loop *loop = GET_BATTLE_LOOP;
			if (loop) {
				loop->set_rumble_offset(General::Point<float>(x, y));
			}
		}
	}
	else if (shaking && now >= _shake.start) {
		if (l) {
			l->get_area()->set_rumble_offset(General::Point<float>(0, 0));
		}
		else {
			Battle_Loop *loop = GET_BATTLE_LOOP;
			if (loop) {
				loop->set_rumble_offset(General::Point<float>(0, 0));
			}
		}
		shaking = false;
	}
}

void Engine::flush_event_queue()
{
	al_flush_event_queue(event_queue);
}

Wrap::Bitmap *Engine::get_hero_shadow()
{
	return hero_shadow_bmp;
}

Wrap::Bitmap *Engine::get_big_shadow()
{
	return big_shadow_bmp;
}

Wrap::Bitmap *Engine::get_work_bitmap()
{
	return work_bitmap;
}

static void before_flip_callback()
{
	ALLEGRO_BITMAP *target = al_get_target_bitmap();
	al_set_target_backbuffer(engine->get_display());
	Graphics::quick_draw(
		target,
		0, 0,
		al_get_bitmap_width(target),
		al_get_bitmap_height(target),
		0, 0,
		al_get_display_width(engine->get_display()),
		al_get_display_height(engine->get_display()),
		0
	);
	al_set_target_bitmap(target);
}

static bool ok_callback(tgui::TGUIWidget *widget)
{
	return widget != NULL;
}

void Engine::notify(std::vector<std::string> texts, std::vector<Loop *> *loops_to_draw, bool is_network_test, bool first_stage)
{
	std::vector<std::string> new_texts;
	if (is_network_test == false || first_stage == true) {
		new_texts = texts;
	}
	else {
		new_texts.push_back(t("TESTING_CONNECTION"));
	}
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
	int th = General::get_font_line_height(General::FONT_LIGHT);
	int w = 200;
	int h = (4+new_texts.size())*th+4;
	Wrap::Bitmap *frame_bmp = Wrap::create_bitmap((w+10)*General::font_scale, (h+10)*General::font_scale);
	al_set_target_bitmap(work_bitmap->bitmap);
	al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
	al_draw_filled_rectangle(
		5*General::font_scale, 5*General::font_scale,
		(5+w)*General::font_scale, (5+h)*General::font_scale,
		al_map_rgb(0x00, 0x00, 0x00)
	);
	al_set_target_bitmap(frame_bmp->bitmap);
	al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
	Graphics::draw_bitmap_shadow_region_no_intermediate(work_bitmap, 0, 0, 10+w, 10+h, 0, 0);
	ALLEGRO_COLOR main_color = al_map_rgb(0xd3, 0xd3, 0xd3);
	ALLEGRO_COLOR dark_color = Graphics::change_brightness(main_color, 0.5f);
	al_draw_filled_rectangle(
		5*General::font_scale, 5*General::font_scale,
		(5+w)*General::font_scale, (5+h)*General::font_scale,
		main_color
	);
	al_draw_rectangle(
		5*General::font_scale+General::font_scale/2, 5*General::font_scale+General::font_scale/2,
		(5+w)*General::font_scale-General::font_scale/2, (5+h)*General::font_scale-General::font_scale/2,
		dark_color,
		General::font_scale
	);
	bool ttf_was_quick = Graphics::ttf_is_quick();
	Graphics::ttf_quick(true);
	for (size_t i = 0; i < new_texts.size(); i++) {
		int x = w / 2 + 5;
		int y = th+i*th + 5;
		General::draw_text(new_texts[i], al_map_rgb(0x00, 0x00, 0x00), x, y, ALLEGRO_ALIGN_CENTER);
	}
	Graphics::ttf_quick(ttf_was_quick);
	main_color = Graphics::change_brightness(main_color, 1.1f);
	dark_color = Graphics::change_brightness(dark_color, 1.1f);
	Wrap::Bitmap *ok_bitmap = Wrap::create_bitmap(50*General::font_scale, (th+4)*General::font_scale);
	al_set_target_bitmap(ok_bitmap->bitmap);
	al_clear_to_color(dark_color);
	al_draw_filled_rectangle(
		General::font_scale, General::font_scale,
		(50*General::font_scale)-General::font_scale, ((th+4)*General::font_scale)-General::font_scale,
		main_color
	);
	General::draw_text(t("OK"), al_map_rgb(0x00, 0x00, 0x00), 25, 2, ALLEGRO_ALIGN_CENTER);
	al_set_target_bitmap(old_target);

	W_Icon *frame = new W_FontScale_Icon(frame_bmp);
	frame->setX(cfg.screen_w/2-5-w/2);
	frame->setY(cfg.screen_h/2-5-h/2);

	int framex = frame->getX()+5;
	int framey = frame->getY()+5;

	W_Icon *ok_icon = new W_FontScale_Icon(ok_bitmap);
	ok_icon->setX(framex+w/2-25);
	ok_icon->setY(framey+th*(2+new_texts.size()));
	W_Button *ok_button = new W_Button(
		ok_icon->getX(),
		ok_icon->getY(),
		ok_icon->getWidth(),
		ok_icon->getHeight()
	);

	ALLEGRO_BITMAP *bg;
	int flags = al_get_new_bitmap_flags();
	draw_touch_controls = false;
	if (render_buffer) {
		int no_preserve_flag;
#ifdef ALLEGRO_ANDROID
		no_preserve_flag = 0;
#else
#ifdef ALLEGRO_WINDOWS
		if (al_get_display_flags(engine->get_display()) & ALLEGRO_DIRECT3D) {
			no_preserve_flag = 0;
		}
		else
#endif
		{
#ifdef ALLEGRO_ANDROID
			no_preserve_flag = 0;
#else
			no_preserve_flag = ALLEGRO_NO_PRESERVE_TEXTURE;
#endif
		}
#endif
		al_set_new_bitmap_flags(no_preserve_flag | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
		bg = al_create_bitmap(
			al_get_bitmap_width(render_buffer->bitmap),
			al_get_bitmap_height(render_buffer->bitmap)
		);
		al_set_target_bitmap(bg);
		draw_all(loops_to_draw == NULL ? loops : *loops_to_draw, false);
	}
	else {
		al_set_new_bitmap_flags(flags & ~ALLEGRO_NO_PRESERVE_TEXTURE);
		bg = al_create_bitmap(
			al_get_display_width(display),
			al_get_display_height(display)
		);
		al_set_target_bitmap(bg);
		ALLEGRO_TRANSFORM t;
		al_identity_transform(&t);
		float scale = (float)al_get_display_width(display) / cfg.screen_w;
		al_scale_transform(&t, scale, scale);
		al_use_transform(&t);
		draw_all(loops_to_draw == NULL ? loops : *loops_to_draw, false);
	}
	draw_touch_controls = true;
	al_set_new_bitmap_flags(flags);

	tgui::hide();
	tgui::push();

	tgui::addWidget(frame);
	tgui::addWidget(ok_icon);
	tgui::addWidget(ok_button);
	tgui::setFocus(ok_button);

	void (*bfc)();
	if (render_buffer) {
		al_set_target_bitmap(render_buffer->bitmap);
		bfc = before_flip_callback;
	}
	else {
		al_set_target_backbuffer(display);
		bfc = NULL;
	}

	do_modal(
		event_queue,
		al_map_rgba_f(0, 0, 0, 0),
		bg,
		ok_callback,
		NULL,
		bfc,
		NULL,
		is_network_test,
		first_stage
	);

	al_destroy_bitmap(bg);
	al_set_target_bitmap(old_target);

	frame->remove();
	ok_button->remove();
	ok_icon->remove();
	delete frame;
	delete ok_button;
	delete ok_icon;

	Wrap::destroy_bitmap(frame_bmp);
	Wrap::destroy_bitmap(ok_bitmap);

	tgui::pop();
	tgui::unhide();

	al_flush_event_queue(event_queue);

#ifdef ADMOB
	if (is_network_test) {
		if (network_is_connected == false) {
			if (first_stage == true) {
				first_stage = false;
			}
			else {
				first_stage = true;
			}
			notify(texts, loops_to_draw, is_network_test, first_stage);
		}
	}
#endif
}

static W_Button *button1;
static W_Button *button2;
static int prompt_result;
static bool prompt_callback(tgui::TGUIWidget *widget)
{
	if (widget == button1) {
		prompt_result = 0;
		return true;
	}
	if (widget == button2) {
		prompt_result = 1;
		return true;
	}
	return false;
}

int Engine::prompt(std::vector<std::string> texts, std::string text1, std::string text2, std::vector<Loop *> *loops_to_draw)
{
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
	int th = General::get_font_line_height(General::FONT_LIGHT);
	int w = 200;
	int h = (4+texts.size())*th+4;
	Wrap::Bitmap *frame_bmp = Wrap::create_bitmap((w+10)*General::font_scale, (h+10)*General::font_scale);
	al_set_target_bitmap(work_bitmap->bitmap);
	al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
	al_draw_filled_rectangle(
		5*General::font_scale, 5*General::font_scale,
		(5+w)*General::font_scale, (5+h)*General::font_scale,
		al_map_rgb(0x00, 0x00, 0x00)
	);
	al_set_target_bitmap(frame_bmp->bitmap);
	al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
	Graphics::draw_bitmap_shadow_region_no_intermediate(work_bitmap, 0, 0, 10+w, 10+h, 0, 0);
	ALLEGRO_COLOR main_color = al_map_rgb(0xd3, 0xd3, 0xd3);
	ALLEGRO_COLOR dark_color = Graphics::change_brightness(main_color, 0.5f);
	al_draw_filled_rectangle(
		5*General::font_scale, 5*General::font_scale,
		(5+w)*General::font_scale, (5+h)*General::font_scale,
		main_color
	);
	al_draw_rectangle(
		5*General::font_scale+General::font_scale/2, 5*General::font_scale+General::font_scale/2,
		(5+w)*General::font_scale-General::font_scale/2, (5+h)*General::font_scale-General::font_scale/2,
		dark_color,
		General::font_scale
	);
	bool ttf_was_quick = Graphics::ttf_is_quick();
	Graphics::ttf_quick(true);
	for (size_t i = 0; i < texts.size(); i++) {
		int x = w / 2 + 5;
		int y = th+i*th + 5;
		General::draw_text(texts[i], al_map_rgb(0x00, 0x00, 0x00), x, y, ALLEGRO_ALIGN_CENTER);
	}
	Graphics::ttf_quick(ttf_was_quick);
	main_color = Graphics::change_brightness(main_color, 1.1f);
	dark_color = Graphics::change_brightness(dark_color, 1.1f);
	Wrap::Bitmap *bitmap1 = Wrap::create_bitmap((50)*General::font_scale, (th+4)*General::font_scale);
	al_set_target_bitmap(bitmap1->bitmap);
	al_clear_to_color(dark_color);
	al_draw_filled_rectangle(
		General::font_scale, General::font_scale,
		(50*General::font_scale)-General::font_scale, ((th+4)*General::font_scale)-General::font_scale,
		main_color
	);
	General::draw_text(text1, al_map_rgb(0x00, 0x00, 0x00), 25, 2, ALLEGRO_ALIGN_CENTER);
	Wrap::Bitmap *bitmap2 = Wrap::create_bitmap(50*General::font_scale, (th+4)*General::font_scale);
	al_set_target_bitmap(bitmap2->bitmap);
	al_clear_to_color(dark_color);
	al_draw_filled_rectangle(
		General::font_scale, General::font_scale,
		(50*General::font_scale)-General::font_scale, ((th+4)*General::font_scale)-General::font_scale,
		main_color
	);
	General::draw_text(text2, al_map_rgb(0x00, 0x00, 0x00), 25, 2, ALLEGRO_ALIGN_CENTER);
	al_set_target_bitmap(old_target);

	W_Icon *frame = new W_FontScale_Icon(frame_bmp);
	frame->setX(cfg.screen_w/2-5-w/2);
	frame->setY(cfg.screen_h/2-5-h/2);

	int framex = frame->getX()+5;
	int framey = frame->getY()+5;

	W_Icon *icon1 = new W_FontScale_Icon(bitmap1);
	icon1->setX(framex+w/2-50-5);
	icon1->setY(framey+th*(2+texts.size()));
	button1 = new W_Button(
		icon1->getX(),
		icon1->getY(),
		icon1->getWidth(),
		icon1->getHeight()
	);

	W_Icon *icon2 = new W_FontScale_Icon(bitmap2);
	icon2->setX(framex+w/2+5);
	icon2->setY(framey+th*(2+texts.size()));
	button2 = new W_Button(
		icon2->getX(),
		icon2->getY(),
		icon2->getWidth(),
		icon2->getHeight()
	);

	ALLEGRO_BITMAP *bg;
	int flags = al_get_new_bitmap_flags();
	draw_touch_controls = false;
	if (render_buffer) {
		int no_preserve_flag;
#ifdef ALLEGRO_ANDROID
		no_preserve_flag = 0;
#else
#ifdef ALLEGRO_WINDOWS
		if (al_get_display_flags(engine->get_display()) & ALLEGRO_DIRECT3D) {
			no_preserve_flag = 0;
		}
		else
#endif
		{
#ifdef ALLEGRO_ANDROID
			no_preserve_flag = 0;
#else
			no_preserve_flag = ALLEGRO_NO_PRESERVE_TEXTURE;
#endif
		}
#endif
		al_set_new_bitmap_flags(no_preserve_flag | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
		bg = al_create_bitmap(
			al_get_bitmap_width(render_buffer->bitmap),
			al_get_bitmap_height(render_buffer->bitmap)
		);
		al_set_target_bitmap(bg);
		draw_all(loops_to_draw == NULL ? loops : *loops_to_draw, false);
	}
	else {
		al_set_new_bitmap_flags(flags & ~ALLEGRO_NO_PRESERVE_TEXTURE);
		bg = al_create_bitmap(
			al_get_display_width(display),
			al_get_display_height(display)
		);
		al_set_target_bitmap(bg);
		ALLEGRO_TRANSFORM t;
		al_identity_transform(&t);
		float scale = (float)al_get_display_width(display) / cfg.screen_w;
		al_scale_transform(&t, scale, scale);
		al_use_transform(&t);
		draw_all(loops_to_draw == NULL ? loops : *loops_to_draw, false);
	}
	draw_touch_controls = true;
	al_set_new_bitmap_flags(flags);

	tgui::hide();
	tgui::push();

	tgui::addWidget(frame);
	tgui::addWidget(icon1);
	tgui::addWidget(button1);
	tgui::addWidget(icon2);
	tgui::addWidget(button2);
	tgui::setFocus(button1);

	void (*bfc)();
	if (render_buffer) {
		al_set_target_bitmap(render_buffer->bitmap);
		bfc = before_flip_callback;
	}
	else {
		al_set_target_backbuffer(display);
		bfc = NULL;
	}

	do_modal(
		event_queue,
		al_map_rgba_f(0, 0, 0, 0),
		bg,
		prompt_callback,
		NULL,
		bfc,
		NULL
	);

	al_destroy_bitmap(bg);
	al_set_target_bitmap(old_target);

	frame->remove();
	button1->remove();
	button2->remove();
	icon1->remove();
	icon2->remove();
	delete frame;
	delete button1;
	delete button2;
	delete icon1;
	delete icon2;

	Wrap::destroy_bitmap(frame_bmp);
	Wrap::destroy_bitmap(bitmap1);
	Wrap::destroy_bitmap(bitmap2);

	tgui::pop();
	tgui::unhide();

	al_flush_event_queue(event_queue);

	return prompt_result;
}

static W_Button *yes_button;
static W_Button *no_button;
static bool yes_no_result;
static bool yes_no_callback(tgui::TGUIWidget *widget)
{
	if (widget == yes_button) {
		yes_no_result = true;
		return true;
	}
	if (widget == no_button) {
		yes_no_result = false;
		return true;
	}
	return false;
}

bool Engine::yes_no_prompt(std::vector<std::string> texts, std::vector<Loop *> *loops_to_draw)
{
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
	int th = General::get_font_line_height(General::FONT_LIGHT);
	int w = 200;
	int h = (4+texts.size())*th+4;
	Wrap::Bitmap *frame_bmp = Wrap::create_bitmap((w+10)*General::font_scale, (h+10)*General::font_scale);
	al_set_target_bitmap(work_bitmap->bitmap);
	al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
	al_draw_filled_rectangle(
		5*General::font_scale, 5*General::font_scale,
		(5+w)*General::font_scale, (5+h)*General::font_scale,
		al_map_rgb(0x00, 0x00, 0x00)
	);
	al_set_target_bitmap(frame_bmp->bitmap);
	al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
	Graphics::draw_bitmap_shadow_region_no_intermediate(work_bitmap, 0, 0, 10+w, 10+h, 0, 0);
	ALLEGRO_COLOR main_color = al_map_rgb(0xd3, 0xd3, 0xd3);
	ALLEGRO_COLOR dark_color = Graphics::change_brightness(main_color, 0.5f);
	al_draw_filled_rectangle(
		5*General::font_scale, 5*General::font_scale,
		(5+w)*General::font_scale, (5+h)*General::font_scale,
		main_color
	);
	al_draw_rectangle(
		5*General::font_scale+General::font_scale/2, 5*General::font_scale+General::font_scale/2,
		(5+w)*General::font_scale-General::font_scale/2, (5+h)*General::font_scale-General::font_scale/2,
		dark_color,
		General::font_scale
	);
	bool ttf_was_quick = Graphics::ttf_is_quick();
	Graphics::ttf_quick(true);
	for (size_t i = 0; i < texts.size(); i++) {
		int x = w / 2 + 5;
		int y = th+i*th + 5;
		General::draw_text(texts[i], al_map_rgb(0x00, 0x00, 0x00), x, y, ALLEGRO_ALIGN_CENTER);
	}
	Graphics::ttf_quick(ttf_was_quick);
	main_color = Graphics::change_brightness(main_color, 1.1f);
	dark_color = Graphics::change_brightness(dark_color, 1.1f);
	Wrap::Bitmap *yes_bitmap = Wrap::create_bitmap(50*General::font_scale, (th+4)*General::font_scale);
	al_set_target_bitmap(yes_bitmap->bitmap);
	al_clear_to_color(dark_color);
	al_draw_filled_rectangle(
		General::font_scale, General::font_scale,
		(50*General::font_scale)-General::font_scale, ((th+4)*General::font_scale)-General::font_scale,
		main_color
	);
	General::draw_text(t("YES"), al_map_rgb(0x00, 0x00, 0x00), 25, 2, ALLEGRO_ALIGN_CENTER);
	Wrap::Bitmap *no_bitmap = Wrap::create_bitmap(50*General::font_scale, (th+4)*General::font_scale);
	al_set_target_bitmap(no_bitmap->bitmap);
	al_clear_to_color(dark_color);
	al_draw_filled_rectangle(
		General::font_scale, General::font_scale,
		(50*General::font_scale)-General::font_scale, ((th+4)*General::font_scale)-General::font_scale,
		main_color
	);
	General::draw_text(t("NO"), al_map_rgb(0x00, 0x00, 0x00), 25, 2, ALLEGRO_ALIGN_CENTER);
	al_set_target_bitmap(old_target);

	W_Icon *frame = new W_FontScale_Icon(frame_bmp);
	frame->setX(cfg.screen_w/2-5-w/2);
	frame->setY(cfg.screen_h/2-5-h/2);

	int framex = frame->getX()+5;
	int framey = frame->getY()+5;

	W_Icon *yes_icon = new W_FontScale_Icon(yes_bitmap);
	yes_icon->setX(framex+w/2-50-5);
	yes_icon->setY(framey+th*(2+texts.size()));
	yes_button = new W_Button(
		yes_icon->getX(),
		yes_icon->getY(),
		yes_icon->getWidth(),
		yes_icon->getHeight()
	);

	W_Icon *no_icon = new W_FontScale_Icon(no_bitmap);
	no_icon->setX(framex+w/2+5);
	no_icon->setY(framey+th*(2+texts.size()));
	no_button = new W_Button(
		no_icon->getX(),
		no_icon->getY(),
		no_icon->getWidth(),
		no_icon->getHeight()
	);

	ALLEGRO_BITMAP *bg;
	int flags = al_get_new_bitmap_flags();
	draw_touch_controls = false;
	if (render_buffer) {
		int no_preserve_flag;
#ifdef ALLEGRO_ANDROID
		no_preserve_flag = 0;
#else
#ifdef ALLEGRO_WINDOWS
		if (al_get_display_flags(engine->get_display()) & ALLEGRO_DIRECT3D) {
			no_preserve_flag = 0;
		}
		else
#endif
		{
#ifdef ALLEGRO_ANDROID
			no_preserve_flag = 0;
#else
			no_preserve_flag = ALLEGRO_NO_PRESERVE_TEXTURE;
#endif
		}
#endif
		al_set_new_bitmap_flags(no_preserve_flag | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
		bg = al_create_bitmap(
			al_get_bitmap_width(render_buffer->bitmap),
			al_get_bitmap_height(render_buffer->bitmap)
		);
		al_set_target_bitmap(bg);
		draw_all(loops_to_draw == NULL ? loops : *loops_to_draw, false);
	}
	else {
		al_set_new_bitmap_flags(flags & ~ALLEGRO_NO_PRESERVE_TEXTURE);
		bg = al_create_bitmap(
			al_get_display_width(display),
			al_get_display_height(display)
		);
		al_set_target_bitmap(bg);
		ALLEGRO_TRANSFORM t;
		al_identity_transform(&t);
		float scale = (float)al_get_display_width(display) / cfg.screen_w;
		al_scale_transform(&t, scale, scale);
		al_use_transform(&t);
		draw_all(loops_to_draw == NULL ? loops : *loops_to_draw, false);
	}
	draw_touch_controls = true;
	al_set_new_bitmap_flags(flags);

	tgui::hide();
	tgui::push();

	tgui::addWidget(frame);
	tgui::addWidget(yes_icon);
	tgui::addWidget(yes_button);
	tgui::addWidget(no_icon);
	tgui::addWidget(no_button);
	tgui::setFocus(yes_button);

	void (*bfc)();
	if (render_buffer) {
		al_set_target_bitmap(render_buffer->bitmap);
		bfc = before_flip_callback;
	}
	else {
		al_set_target_backbuffer(display);
		bfc = NULL;
	}

	do_modal(
		event_queue,
		al_map_rgba_f(0, 0, 0, 0),
		bg,
		yes_no_callback,
		NULL,
		bfc,
		NULL
	);

	al_destroy_bitmap(bg);
	al_set_target_bitmap(old_target);

	frame->remove();
	yes_button->remove();
	no_button->remove();
	yes_icon->remove();
	no_icon->remove();
	delete frame;
	delete yes_button;
	delete no_button;
	delete yes_icon;
	delete no_icon;

	Wrap::destroy_bitmap(frame_bmp);
	Wrap::destroy_bitmap(yes_bitmap);
	Wrap::destroy_bitmap(no_bitmap);

	tgui::pop();
	tgui::unhide();

	al_flush_event_queue(event_queue);

	return yes_no_result;
}

static int get_number_min;
static int get_number_max;
static W_Button *ok_button;
static W_Button *cancel_button;
static W_Button *get_number_up_button;
static W_Button *get_number_down_button;
static W_Integer *number_widget;
static int get_number_result;
static bool get_number_callback(tgui::TGUIWidget *widget)
{
	if (widget == ok_button) {
		get_number_result = number_widget->get_number();
		return true;
	}
	else if (widget == cancel_button) {
		get_number_result = -1;
		return true;
	}
	else if (widget == get_number_up_button) {
		int n = number_widget->get_number();
		if (n < get_number_max) {
			n++;
			number_widget->set_number(n);
		}
	}
	else if (widget == get_number_down_button) {
		int n = number_widget->get_number();
		if (n > get_number_min) {
			n--;
			number_widget->set_number(n);
		}
	}
	return false;
}

int Engine::get_number(std::vector<std::string> texts, int low, int high, int start, std::vector<Loop *> *loops_to_draw)
{
	get_number_min = low;
	get_number_max = high;
	ALLEGRO_BITMAP *old_target = al_get_target_bitmap();
	int th = General::get_font_line_height(General::FONT_LIGHT);
	int w = 200;
	int h = (6+texts.size())*th+4;
	Wrap::Bitmap *frame_bmp = Wrap::create_bitmap((w+10)*General::font_scale, (h+10)*General::font_scale);
	al_set_target_bitmap(work_bitmap->bitmap);
	al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
	al_draw_filled_rectangle(
		5*General::font_scale, 5*General::font_scale,
		(5+w)*General::font_scale, (5+h)*General::font_scale,
		al_map_rgb(0x00, 0x00, 0x00)
	);
	al_set_target_bitmap(frame_bmp->bitmap);
	al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
	Graphics::draw_bitmap_shadow_region_no_intermediate(work_bitmap, 0, 0, 10+w, 10+h, 0, 0);
	ALLEGRO_COLOR main_color = al_map_rgb(0xd3, 0xd3, 0xd3);
	ALLEGRO_COLOR dark_color = Graphics::change_brightness(main_color, 0.5f);
	al_draw_filled_rectangle(
		5*General::font_scale, 5*General::font_scale,
		(5+w)*General::font_scale, (5+h)*General::font_scale,
		main_color
	);
	al_draw_rectangle(
		5*General::font_scale+General::font_scale/2, 5*General::font_scale+General::font_scale/2,
		(5+w)*General::font_scale-General::font_scale/2, (5+h)*General::font_scale-General::font_scale/2,
		dark_color,
		General::font_scale
	);
	bool ttf_was_quick = Graphics::ttf_is_quick();
	Graphics::ttf_quick(true);
	for (size_t i = 0; i < texts.size(); i++) {
		int x = (w+10) / 2;
		int y = th+i*th + 5;
		General::draw_text(texts[i], al_map_rgb(0x00, 0x00, 0x00), x, y, ALLEGRO_ALIGN_CENTER);
	}
	Graphics::ttf_quick(ttf_was_quick);

	W_Icon *frame = new W_FontScale_Icon(frame_bmp);
	frame->setX(cfg.screen_w/2-5-w/2);
	frame->setY(cfg.screen_h/2-5-h/2);

	int framex = frame->getX()+5;
	int framey = frame->getY()+5;

	number_widget = new W_Integer(start);
	number_widget->setX(framex+w/2);
	number_widget->setY(framey+th*(2+texts.size())+th/2);
	get_number_down_button = new W_Button("misc_graphics/interface/small_down_arrow.png");
	tgui::centerWidget(get_number_down_button, framex+w/2 - 25, framey+th*(2+texts.size())+th/2);
	get_number_up_button = new W_Button("misc_graphics/interface/small_up_arrow.png");
	tgui::centerWidget(get_number_up_button, framex+w/2 + 25, framey+th*(2+texts.size())+th/2);

	main_color = Graphics::change_brightness(main_color, 1.1f);
	dark_color = Graphics::change_brightness(dark_color, 1.1f);
	Wrap::Bitmap *ok_bitmap = Wrap::create_bitmap(50*General::font_scale, (th+4)*General::font_scale);
	al_set_target_bitmap(ok_bitmap->bitmap);
	al_clear_to_color(dark_color);
	al_draw_filled_rectangle(
		General::font_scale, General::font_scale,
		(50*General::font_scale)-General::font_scale, ((th+4)*General::font_scale)-General::font_scale,
		main_color
	);
	General::draw_text(t("OK"), al_map_rgb(0x00, 0x00, 0x00), 25, 2, ALLEGRO_ALIGN_CENTER);
	Wrap::Bitmap *cancel_bitmap = Wrap::create_bitmap(50*General::font_scale, (th+4)*General::font_scale);
	al_set_target_bitmap(cancel_bitmap->bitmap);
	al_clear_to_color(dark_color);
	al_draw_filled_rectangle(
		General::font_scale, General::font_scale,
		(50*General::font_scale)-General::font_scale, ((th+4)*General::font_scale)-General::font_scale,
		main_color
	);
	General::draw_text(t("CANCEL"), al_map_rgb(0x00, 0x00, 0x00), 25, 2, ALLEGRO_ALIGN_CENTER);
	al_set_target_bitmap(old_target);

	W_Icon *ok_icon = new W_FontScale_Icon(ok_bitmap);
	ok_icon->setX(framex+w/2-5-50);
	ok_icon->setY(framey+th*(4+texts.size()));
	ok_button = new W_Button(
		ok_icon->getX(),
		ok_icon->getY(),
		ok_icon->getWidth(),
		ok_icon->getHeight()
	);

	W_Icon *cancel_icon = new W_FontScale_Icon(cancel_bitmap);
	cancel_icon->setX(framex+w/2+5);
	cancel_icon->setY(framey+th*(4+texts.size()));
	cancel_button = new W_Button(
		cancel_icon->getX(),
		cancel_icon->getY(),
		cancel_icon->getWidth(),
		cancel_icon->getHeight()
	);

	ALLEGRO_BITMAP *bg;
	int flags = al_get_new_bitmap_flags();
	draw_touch_controls = false;
	if (render_buffer) {
		int no_preserve_flag;
#ifdef ALLEGRO_ANDROID
		no_preserve_flag = 0;
#else
#ifdef ALLEGRO_WINDOWS
		if (al_get_display_flags(engine->get_display()) & ALLEGRO_DIRECT3D) {
			no_preserve_flag = 0;
		}
		else
#endif
		{
#ifdef ALLEGRO_ANDROID
			no_preserve_flag = 0;
#else
			no_preserve_flag = ALLEGRO_NO_PRESERVE_TEXTURE;
#endif
		}
#endif
		al_set_new_bitmap_flags(no_preserve_flag | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
		bg = al_create_bitmap(
			al_get_bitmap_width(render_buffer->bitmap),
			al_get_bitmap_height(render_buffer->bitmap)
		);
		al_set_target_bitmap(bg);
		draw_all(loops_to_draw == NULL ? loops : *loops_to_draw, false);
	}
	else {
		al_set_new_bitmap_flags(flags & ~ALLEGRO_NO_PRESERVE_TEXTURE);
		bg = al_create_bitmap(
			al_get_display_width(display),
			al_get_display_height(display)
		);
		al_set_target_bitmap(bg);
		ALLEGRO_TRANSFORM t;
		al_identity_transform(&t);
		float scale = (float)al_get_display_width(display) / cfg.screen_w;
		al_scale_transform(&t, scale, scale);
		al_use_transform(&t);
		draw_all(loops_to_draw == NULL ? loops : *loops_to_draw, false);
	}
	draw_touch_controls = true;
	al_set_new_bitmap_flags(flags);

	tgui::hide();
	tgui::push();

	tgui::addWidget(frame);
	tgui::addWidget(get_number_down_button);
	tgui::addWidget(number_widget);
	tgui::addWidget(get_number_up_button);
	tgui::addWidget(ok_icon);
	tgui::addWidget(ok_button);
	tgui::addWidget(cancel_icon);
	tgui::addWidget(cancel_button);

	tgui::setFocus(ok_button);

	void (*bfc)();
	if (render_buffer) {
		al_set_target_bitmap(render_buffer->bitmap);
		bfc = before_flip_callback;
	}
	else {
		al_set_target_backbuffer(display);
		bfc = NULL;
	}

	do_modal(
		event_queue,
		al_map_rgba_f(0, 0, 0, 0),
		bg,
		get_number_callback,
		NULL,
		bfc,
		NULL
	);

	al_destroy_bitmap(bg);
	al_set_target_bitmap(old_target);

	frame->remove();
	get_number_down_button->remove();
	number_widget->remove();
	get_number_up_button->remove();
	ok_button->remove();
	cancel_button->remove();
	ok_icon->remove();
	cancel_icon->remove();
	delete frame;
	delete get_number_down_button;
	delete number_widget;
	delete get_number_up_button;
	delete ok_button;
	delete cancel_button;
	delete ok_icon;
	delete cancel_icon;

	Wrap::destroy_bitmap(frame_bmp);
	Wrap::destroy_bitmap(ok_bitmap);
	Wrap::destroy_bitmap(cancel_bitmap);

	tgui::pop();
	tgui::unhide();

	al_flush_event_queue(event_queue);

	return get_number_result;
}

std::string Engine::get_last_area()
{
	return last_area;
}

void Engine::set_last_area(std::string last_area)
{
	this->last_area = last_area;
}

static void fade(std::vector<Loop *> loops, double time, bool out)
{
	double fade_start = al_get_time();

	while (true) {
		ALLEGRO_BITMAP *old_target = engine->set_draw_target(false);

		al_clear_to_color(al_map_rgb(0, 0, 0));

		engine->draw_all(loops, true);

		bool done = false;

		double elapsed = (al_get_time() - fade_start) / time;
		if (elapsed > 1) {
			elapsed = 1;
			done = true;
		}

		if (out) {
			al_draw_filled_rectangle(0,  0, al_get_display_width(engine->get_display()), al_get_display_height(engine->get_display()),
				al_map_rgba_f(0, 0, 0, elapsed));
		}
		else {
			float p = 1.0f - (float)elapsed;
			al_draw_filled_rectangle(0,  0, al_get_display_width(engine->get_display()), al_get_display_height(engine->get_display()),
				al_map_rgba_f(0, 0, 0, p));
		}

		engine->finish_draw(false, old_target);

		backup_dirty_bitmaps();

		al_flip_display();

		if (done) {
			break;
		}

		al_rest(1.0/60.0);
	}

	tgui::releaseKeysAndButtons();
	engine->flush_event_queue();
}

void Engine::fade_out(double time)
{
	::fade(engine->get_loops(), time, true);
}

void Engine::fade_in(double time)
{
	::fade(engine->get_loops(), time, false);
}

void Engine::fade_out(std::vector<Loop *> loops, double time)
{
	::fade(loops, time, true);
}

void Engine::fade_in(std::vector<Loop *> loops, double time)
{
	::fade(loops, time, false);
}

void Engine::set_game_over(bool game_over)
{
	this->game_over = game_over;
}

bool Engine::game_is_over()
{
	return game_over;
}

bool Engine::can_use_crystals()
{
	return milestone_is_complete("pyou_intro");
}

void Engine::load_cpa()
{
#if defined ALLEGRO_ANDROID
	// Loaded by PHYSFS
	cpa = new CPA("assets/data.cpa.uncompressed");
#elif defined ALLEGRO_IPHONE
	cpa = new CPA("data.cpa.uncompressed");
#elif defined ALLEGRO_RASPBERRYPI
	cpa = new CPA("stuff/data.cpa.uncompressed");
#elif defined __linux__
	cpa = new CPA("stuff/data.cpa");
#else
	cpa = new CPA("data.cpa");
#endif
}

void Engine::set_done(bool done)
{
	this->done = done;
}

bool Engine::get_done()
{
	return done;
}

void Engine::set_purchased(bool purchased)
{
	this->purchased = purchased;
}

bool Engine::get_purchased()
{
	return purchased;
}

ALLEGRO_EVENT_QUEUE *Engine::get_event_queue()
{
	return event_queue;
}

bool Engine::is_switched_out()
{
	return switched_out;
}

bool Engine::get_continued_or_saved()
{
	return continued_or_saved;
}

void Engine::set_continued_or_saved(bool continued_or_saved)
{
	this->continued_or_saved = continued_or_saved;
}

void Engine::loaded_video(Video_Player *v)
{
	_loaded_video = v;
}

std::string Engine::get_music_name()
{
	return music_name;
}

void Engine::set_send_tgui_events(bool send_tgui_events)
{
	this->send_tgui_events = send_tgui_events;
	if (send_tgui_events == false) {
		tgui::releaseKeysAndButtons();
	}
}

bool Engine::get_send_tgui_events()
{
	return send_tgui_events;
}

int Engine::get_save_number_last_used()
{
	return save_number_last_used;
}

bool Engine::is_render_buffer(ALLEGRO_BITMAP *bmp)
{
	return render_buffer && render_buffer->bitmap == bmp;
}

bool Engine::in_mini_loop()
{
	return _in_mini_loop;
}

std::vector<Loop *> Engine::get_mini_loops()
{
	return mini_loops;
}

void Engine::set_mini_loops(std::vector<Loop *> loops)
{
	mini_loops = loops;
}

TouchInputType Engine::get_touch_input_type()
{
	return touch_input_type;
}

void Engine::set_touch_input_type(TouchInputType type)
{
	touch_input_type = type;
}

void Engine::get_touch_input_button_position(int location, int *x, int *y)
{
	// offset from middle to top button
	int y_offset = (cfg.screen_h/2 - TOUCH_BUTTON_RADIUS*5) / 2;
	int stick_y_offset = (cfg.screen_h/2 - TOUCH_STICK_RADIUS*2) / 2;
	// actual y offset of top button (top or bottom)
	int base_y = touch_input_on_bottom ? cfg.screen_h / 2 + y_offset : y_offset;

	int base_x = cfg.screen_w - 10  - TOUCH_BUTTON_RADIUS * 5;

	int square_y = touch_input_on_bottom ? 10 : cfg.screen_h - 10 - TOUCH_BUTTON_RADIUS * TOUCH_SQUARE_PERCENT;

	int button_x, button_y;

	switch (location) {
		case -1: // stick
			button_x = 10 + TOUCH_STICK_TRAVEL;
			button_y = touch_input_on_bottom ? cfg.screen_h/2 + stick_y_offset : stick_y_offset;
			break;
		case 4:
			button_x = base_x;
			button_y = square_y;
			break;
		case 5:
			button_x = base_x + TOUCH_BUTTON_RADIUS * 3;
			button_y = square_y;
			break;
		case 0:
			button_x = base_x + TOUCH_BUTTON_RADIUS * 1.5f;
			button_y = base_y;
			break;
		case 1:
			button_x = base_x;
			button_y = base_y + TOUCH_BUTTON_RADIUS * 1.5f;
			break;
		case 2:
			button_x = base_x + TOUCH_BUTTON_RADIUS * 3;
			button_y = base_y + TOUCH_BUTTON_RADIUS * 1.5f;
			break;
		default:
			button_x = base_x + TOUCH_BUTTON_RADIUS * 1.5f;
			button_y = base_y + TOUCH_BUTTON_RADIUS * 3;
			break;
	}

	*x = button_x;
	*y = button_y;
}

void Engine::draw_touch_input_button(int location, TouchType bitmap)
{
	int x, y;

	get_touch_input_button_position(location, &x, &y);

	Wrap::Bitmap *bmp = touch_bitmaps[bitmap];
	int bmpw = al_get_bitmap_width(bmp->bitmap);
	int bmph = al_get_bitmap_height(bmp->bitmap);

	float screen_w = al_get_display_width(display);
	float screen_h = al_get_display_height(display);
	float scalex = screen_w / cfg.screen_w;
	float scaley = screen_h / cfg.screen_h;

	ALLEGRO_STATE state;
	al_store_state(&state, ALLEGRO_STATE_BLENDER);

	bool pressed = false;
	for (size_t i = 0; i < touches.size(); i++) {
		if (touches[i].location == location) {
			pressed = true;
			break;
		}
	}

	int height;
	if (location == 4 || location == 5) {
		height = scaley * TOUCH_BUTTON_RADIUS * TOUCH_SQUARE_PERCENT;
	}
	else {
		height = scaley * TOUCH_BUTTON_RADIUS * 2;
	}

	ALLEGRO_COLOR tint = al_map_rgba_f(0.5f, 0.5f, 0.5f, 0.5f);

	Graphics::quick_draw(
		bmp->bitmap,
		tint,
		0, 0, bmpw, bmph,
		x * scalex,
		y * scaley,
		scaley * TOUCH_BUTTON_RADIUS * 2,
		height,
		0
	);

	if (pressed) {
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
		Graphics::quick_draw(
			bmp->bitmap,
			tint,
			0, 0, bmpw, bmph,
			x * scalex,
			y * scaley,
			scaley * TOUCH_BUTTON_RADIUS * 2,
			height,
			0
		);
	}

	al_restore_state(&state);
}

bool Engine::touch_is_on_button(ALLEGRO_EVENT *event, int location)
{
	int x, y;
	get_touch_input_button_position(location, &x, &y);

	if (location == 4 || location == 5) {
		General::Point<float> p(event->touch.x, event->touch.y);
		General::Point<float> topleft(x, y);
		General::Point<float> bottomright(x+TOUCH_BUTTON_RADIUS*2, y+TOUCH_BUTTON_RADIUS*TOUCH_SQUARE_PERCENT);
		if (checkcoll_point_box(p, topleft, bottomright)) {
			return true;
		}
		return false;
	}

	x += TOUCH_BUTTON_RADIUS;
	y += TOUCH_BUTTON_RADIUS;

	if (General::distance(x, y, event->touch.x, event->touch.y) <= TOUCH_BUTTON_RADIUS) {
		return true;
	}
	return false;
}

void Engine::add_touch(ALLEGRO_EVENT *event, int location)
{
	Touch t;
	t.id = event->touch.id;
	t.x = event->touch.x;
	t.y = event->touch.y;
	t.location = location;
	touches.push_back(t);
}

int Engine::find_touch(int id)
{
	int index = -1;

	for (size_t i = 0; i < touches.size(); i++) {
		if (touches[i].id == id) {
			index = i;
			break;
		}
	}

	return index;
}

void Engine::update_touch(ALLEGRO_EVENT *event)
{
	// This moves the controls to top or bottom but we don't want that anymore.
	/*
	if (event->type == ALLEGRO_EVENT_TOUCH_BEGIN) {
		if (event->touch.x >= cfg.screen_w/2-25 && event->touch.x <= cfg.screen_w/2+25) {
			if (event->touch.y < cfg.screen_h/2) {
				touch_input_on_bottom = false;
			}
			else {
				touch_input_on_bottom = true;
			}
			return;
		}
	}
	*/

	// Battle stick code
	if (GET_BATTLE_LOOP && !dynamic_cast<Runner_Loop *>(GET_BATTLE_LOOP)) {
		if (event->type == ALLEGRO_EVENT_TOUCH_BEGIN || event->type == ALLEGRO_EVENT_TOUCH_MOVE) {
			bool go = true;
			for (int i = 0; i < 6; i++) {
				if (touch_is_on_button(event, i)) {
					go = false;
					break;
				}
			}
			if (go) {
				go = false;

				if (event->type == ALLEGRO_EVENT_TOUCH_BEGIN) {
					for (size_t i = 0; i < touches.size(); i++) {
						if (touches[i].location == -1) {
							// Don't put multiple touches on stick
							return;
						}
					}
					add_touch(event, -1);

					Area_Loop *al = GET_AREA_LOOP;
					Battle_Loop *bl = GET_BATTLE_LOOP;
					if (bl) {
						Battle_Player *player = bl->get_active_player();
						if (player->is_facing_right()) {
							last_direction = 0;
						}
						else {
							last_direction = M_PI;
						}
					}
					else if (al) {
						Area_Manager *area = al->get_area();
						Map_Entity *player = area->get_entity(0);
						General::Direction dir = player->get_direction();
						switch (dir) {
							case General::DIR_N:
								last_direction = M_PI * 1.5;
								break;
							case General::DIR_NE:
								last_direction = M_PI * 1.5 + M_PI / 4;
								break;
							case General::DIR_E:
								last_direction = 0;
								break;
							case General::DIR_SE:
								last_direction = M_PI / 4;
								break;
							case General::DIR_S:
								last_direction = M_PI / 2;
								break;
							case General::DIR_SW:
								last_direction = M_PI / 2 + M_PI / 4;
								break;
							case General::DIR_W:
								last_direction = M_PI;
								break;
							default:
								last_direction = M_PI + M_PI / 4;
								break;
						}
					}
					else {
						last_direction = 0;
					}
				}
				else {
					int index = find_touch(event->touch.id);
					if (index >= 0 && touches[index].location == -1) {
						float old_x = touches[index].x;
						float old_y = touches[index].y;
						float x = event->touch.x;
						float y = event->touch.y;

						float dx = x - old_x;
						float dy = y - old_y;

						int dw = al_get_display_width(display);
						int dh = al_get_display_height(display);
						int sm = MIN(dw, dh);

						if (General::distance(x, y, old_x, old_y) >= (sm * (GET_BATTLE_LOOP ? 0.005f: 0.01f))) {
							touches[index].x = x;
							touches[index].y = y;

							float a = atan2(dy, dx);

							while (a < 0) a += M_PI*2;
							while (a >= M_PI*2) a -= M_PI*2;

							if (GET_BATTLE_LOOP) {
								if (a >= M_PI*1.5 + M_PI/4 || a < M_PI/4) {
									a = 0;
								}
								else if (a < M_PI/4*3) {
									a = M_PI/2;
								}
								else if (a < M_PI/4*5) {
									a = M_PI;
								}
								else {
									a = M_PI*1.5;
								}
							}
							else {
								if (GET_AREA_LOOP && GET_AREA_LOOP->get_area()->get_entity(0)->get_inputs()[Map_Entity::X] == 0 && GET_AREA_LOOP->get_area()->get_entity(0)->get_inputs()[Map_Entity::Y] == 0) {
									if (General::angle_difference(a, 0) <= M_PI/4) {
										a = 0;
									}
									else if (General::angle_difference(a, M_PI/2) <= M_PI/4) {
										a = M_PI/2;
									}
									else if (General::angle_difference(a, M_PI) <= M_PI/4) {
										a = M_PI;
									}
									else {
										a = M_PI * 1.5;
									}
								}
								else {
									if (General::angle_difference(last_direction, a) < M_PI/4) {
										a = last_direction;
									}
									else if (General::angle_difference_clockwise(last_direction, a) < M_PI/2) {
										a = last_direction + M_PI / 4;
									}
									else if (General::angle_difference_counter_clockwise(last_direction, a) < M_PI/2) {
										a = last_direction - M_PI / 4;
									}
									else {
										if (General::angle_difference(a, 0) <= M_PI/4) {
											a = 0;
										}
										else if (General::angle_difference(a, M_PI/2) <= M_PI/4) {
											a = M_PI/2;
										}
										else if (General::angle_difference(a, M_PI) <= M_PI/4) {
											a = M_PI;
										}
										else {
											a = M_PI * 1.5;
										}
									}
								}
							}

							while (a < 0) a += M_PI*2;
							while (a >= M_PI*2) a -= M_PI*2;

							last_direction = a;

							float xx = cosf(a);
							float yy = sinf(a);

							event->type = ALLEGRO_EVENT_JOYSTICK_AXIS;
							event->joystick.id = 0;
							event->joystick.stick = 0;
							event->joystick.axis = 0;
							event->joystick.pos = xx;
							ALLEGRO_EVENT extra;
							extra.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
							extra.joystick.id = 0;
							extra.joystick.stick = 0;
							extra.joystick.axis = 1;
							extra.joystick.pos = yy;
							extra_events.push_back(extra);
						}
					}
				}

				return;
			}
		}
		else {
			int index = find_touch(event->touch.id);
			if (index >= 0 && touches[index].location == -1) {
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_AXIS;
				event->joystick.id = 0;
				event->joystick.stick = 0;
				event->joystick.axis = 0;
				event->joystick.pos = 0;
				ALLEGRO_EVENT extra;
				extra.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
				extra.joystick.id = 0;
				extra.joystick.stick = 0;
				extra.joystick.axis = 1;
				extra.joystick.pos = 0;
				extra_events.push_back(extra);
				last_direction = -1;
				return;
			}
		}
	}
	else { // Other stick code
		if (event->type == ALLEGRO_EVENT_TOUCH_BEGIN || event->type == ALLEGRO_EVENT_TOUCH_MOVE) {
			int x, y;
			get_touch_input_button_position(-1, &x, &y);
			x += TOUCH_STICK_RADIUS;
			y += TOUCH_STICK_RADIUS;
			bool go = false;
			if (event->type == ALLEGRO_EVENT_TOUCH_BEGIN) {
				if (General::distance(x, y, event->touch.x, event->touch.y) <= TOUCH_STICK_RADIUS+TOUCH_STICK_TRAVEL) {
					for (size_t i = 0; i < touches.size(); i++) {
						if (touches[i].location == -1) {
							// Don't put multiple touches on stick
							return;
						}
					}
					add_touch(event, -1);
					go = true;
				}
			}
			else {
				int index = find_touch(event->touch.id);
				if (index >= 0 && touches[index].location == -1) {
					touches[index].x = event->touch.x;
					touches[index].y = event->touch.y;
					go = true;
				}
			}

			if (go) {
				float xx = event->touch.x - x;
				float yy = event->touch.y - y;
				xx /= (TOUCH_STICK_RADIUS+TOUCH_STICK_TRAVEL)/2;
				yy /= (TOUCH_STICK_RADIUS+TOUCH_STICK_TRAVEL)/2;
				if (xx < -1) xx = -1;
				if (xx > 1) xx = 1;
				if (yy < -1) yy = -1;
				if (yy > 1) yy = 1;
				event->type = ALLEGRO_EVENT_JOYSTICK_AXIS;
				event->joystick.id = 0;
				event->joystick.stick = 0;
				event->joystick.axis = 0;
				event->joystick.pos = xx;
				ALLEGRO_EVENT extra;
				extra.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
				extra.joystick.id = 0;
				extra.joystick.stick = 0;
				extra.joystick.axis = 1;
				extra.joystick.pos = yy;
				extra_events.push_back(extra);

				return;
			}
		}
		else {
			int index = find_touch(event->touch.id);
			if (index >= 0 && touches[index].location == -1) {
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_AXIS;
				event->joystick.id = 0;
				event->joystick.stick = 0;
				event->joystick.axis = 0;
				event->joystick.pos = 0;
				ALLEGRO_EVENT extra;
				extra.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
				extra.joystick.id = 0;
				extra.joystick.stick = 0;
				extra.joystick.axis = 1;
				extra.joystick.pos = 0;
				extra_events.push_back(extra);
				return;
			}
		}
	}

	Battle_Loop *battle_loop = GET_BATTLE_LOOP;
	if (!battle_loop) {
		battle_loop = General::find_in_vector<Battle_Loop *, Loop *>(mini_loops);
	}

	if (event->type == ALLEGRO_EVENT_TOUCH_BEGIN) {
		if (touch_input_type == TOUCHINPUT_SPEECH) {
			if (touch_is_on_button(event, 3)) {
				add_touch(event, 3);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_ability[3];
			}
		}
		else if (touch_input_type == TOUCHINPUT_BATTLE) {
			bool processed = false;
			for (int i = 0; i < 4; i++) {
				if (touch_is_on_button(event, i)) {
					add_touch(event, i);
					event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
					event->joystick.button = cfg.joy_ability[i];
					processed = true;
					break;
				}
			}
			if (battle_loop && !dynamic_cast<Runner_Loop *>(battle_loop) && !battle_loop->is_cart_battle() && !processed && touch_is_on_button(event, 5)) {
				add_touch(event, 5);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_switch;
			}
		}
		else if (touch_input_type == TOUCHINPUT_MAP) {
			if (touch_is_on_button(event, 3)) {
				add_touch(event, 3);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_ability[3];
			}
			else if (touch_is_on_button(event, 4)) {
				add_touch(event, 4);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_menu;
			}
		}
		else if (touch_input_type == TOUCHINPUT_AREA) {
			if (touch_is_on_button(event, 1)) {
				add_touch(event, 1);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_ability[1];
			}
			else if (touch_is_on_button(event, 3)) {
				add_touch(event, 3);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_ability[3];
			}
			else if (touch_is_on_button(event, 4)) {
				add_touch(event, 4);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_menu;
			}
			else if (touch_is_on_button(event, 5)) {
				add_touch(event, 5);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_switch;
			}
		}
	}
	else if (event->type == ALLEGRO_EVENT_TOUCH_END) {
		int index = find_touch(event->touch.id);
		if (index >= 0) {
			if (touch_input_type == TOUCHINPUT_SPEECH) {
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				event->joystick.button = cfg.joy_ability[3];
			}
			else if (touch_input_type == TOUCHINPUT_BATTLE) {
				int b = touches[index].location;
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				if (b == 5) {
					event->joystick.button = cfg.joy_switch;
				}
				else {
					event->joystick.button = cfg.joy_ability[b];
				}
			}
			else if (touch_input_type == TOUCHINPUT_MAP) {
				int b = touches[index].location;
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				if (b == 4) {
					event->joystick.button = cfg.joy_menu;
				}
				else {
					event->joystick.button = cfg.joy_ability[3];
				}
			}
			else if (touch_input_type == TOUCHINPUT_AREA) {
				int b = touches[index].location;
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				if (b == 4) {
					event->joystick.button = cfg.joy_menu;
				}
				else if (b == 5) {
					event->joystick.button = cfg.joy_switch;
				}
				else {
					event->joystick.button = cfg.joy_ability[b];
				}
			}
		}
	}
	else {
		int index = find_touch(event->touch.id);
		if (touch_input_type == TOUCHINPUT_SPEECH) {
			if (index < 0 && touch_is_on_button(event, 3)) {
				add_touch(event, 3);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_ability[3];
			}
			else if (index >= 0 && touches[index].location == 3 && !touch_is_on_button(event, 3)) {
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				event->joystick.button = cfg.joy_ability[3];
			}
		}
		else if (touch_input_type == TOUCHINPUT_BATTLE) {
			bool processed = false;
			for (int i = 0; i < 4; i++) {
				if (index < 0 && touch_is_on_button(event, i)) {
					add_touch(event, i);
					event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
					event->joystick.button = cfg.joy_ability[i];
					processed = true;
					break;
				}
				else if (index >= 0 && touches[index].location == i && !touch_is_on_button(event, i)) {
					touches.erase(touches.begin() + index);
					event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
					event->joystick.button = cfg.joy_ability[i];
					processed = true;
					break;
				}
			}
			if (!processed && battle_loop && !dynamic_cast<Runner_Loop *>(battle_loop) && !battle_loop->is_cart_battle()) {
				if (index < 0 && touch_is_on_button(event, 5)) {
					add_touch(event, 5);
					event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
					event->joystick.button = cfg.joy_switch;
				}
				else if (index >= 0 && touches[index].location == 5 && !touch_is_on_button(event, 5)) {
					touches.erase(touches.begin() + index);
					event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
					event->joystick.button = cfg.joy_switch;
				}
			}
		}
		else if (touch_input_type == TOUCHINPUT_MAP) {
			if (index < 0 && touch_is_on_button(event, 3)) {
				add_touch(event, 3);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_ability[3];
			}
			else if (index >= 0 && touches[index].location == 3 && !touch_is_on_button(event, 3)) {
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				event->joystick.button = cfg.joy_ability[3];
			}
			else if (index < 0 && touch_is_on_button(event, 4)) {
				add_touch(event, 4);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_menu;
			}
			else if (index >= 0 && touches[index].location == 4 && !touch_is_on_button(event, 4)) {
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				event->joystick.button = cfg.joy_menu;
			}
		}
		else if (touch_input_type == TOUCHINPUT_AREA) {
			if (index < 0 && touch_is_on_button(event, 1)) {
				add_touch(event, 1);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_ability[1];
			}
			else if (index >= 0 && touches[index].location == 1 && !touch_is_on_button(event, 1)) {
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				event->joystick.button = cfg.joy_ability[1];
			}
			else if (index < 0 && touch_is_on_button(event, 3)) {
				add_touch(event, 3);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_ability[3];
			}
			else if (index >= 0 && touches[index].location == 3 && !touch_is_on_button(event, 3)) {
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				event->joystick.button = cfg.joy_ability[3];
			}
			else if (index < 0 && touch_is_on_button(event, 4)) {
				add_touch(event, 4);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_menu;
			}
			else if (index >= 0 && touches[index].location == 4 && !touch_is_on_button(event, 4)) {
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				event->joystick.button = cfg.joy_menu;
			}
			else if (index < 0 && touch_is_on_button(event, 5)) {
				add_touch(event, 5);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN;
				event->joystick.button = cfg.joy_switch;
			}
			else if (index >= 0 && touches[index].location == 5 && !touch_is_on_button(event, 5)) {
				touches.erase(touches.begin() + index);
				event->type = ALLEGRO_EVENT_JOYSTICK_BUTTON_UP;
				event->joystick.button = cfg.joy_switch;
			}
		}
	}
}

void Engine::draw_touch_input_stick()
{
	if (GET_BATTLE_LOOP) {
		return;
	}

	int index = -1;

	for (size_t i = 0; i < touches.size(); i++) {
		if (touches[i].location == -1) {
			index = i;
			break;
		}
	}

	int x, y;
	get_touch_input_button_position(-1, &x, &y);

	Wrap::Bitmap *bmp = touch_bitmaps[TOUCH_ANALOGBASE];
	int bmpw = al_get_bitmap_width(bmp->bitmap);
	int bmph = al_get_bitmap_height(bmp->bitmap);

	float screen_w = al_get_display_width(display);
	float screen_h = al_get_display_height(display);
	float scalex = screen_w / cfg.screen_w;
	float scaley = screen_h / cfg.screen_h;

	ALLEGRO_COLOR tint = al_map_rgba_f(0.5f, 0.5f, 0.5f, 0.5f);

	Graphics::quick_draw(
		bmp->bitmap,
		tint,
		0, 0, bmpw, bmph,
		x * scalex,
		y * scaley,
		TOUCH_STICK_RADIUS * 2 * scaley,
		TOUCH_STICK_RADIUS * 2 * scaley,
		0
	);

	if (index >= 0) {
		float x2 = touches[index].x - TOUCH_STICK_RADIUS;
		float y2 = touches[index].y - TOUCH_STICK_RADIUS;
		float dx = x2 - x;
		float dy = y2 - y;
		float a = atan2(dy, dx);
		float length = sqrtf(dx*dx + dy*dy);
		if (length > TOUCH_STICK_RADIUS) {
			x = x + cosf(a) * TOUCH_STICK_RADIUS;
			y = y + sinf(a) * TOUCH_STICK_RADIUS;
		}
		else {
			x = x2;
			y = y2;
		}
	}

	bmp = touch_bitmaps[TOUCH_ANALOGSTICK];
	bmpw = al_get_bitmap_width(bmp->bitmap);
	bmph = al_get_bitmap_height(bmp->bitmap);

	Graphics::quick_draw(
		bmp->bitmap,
		tint,
		0, 0, bmpw, bmph,
		x * scalex,
		y * scaley,
		TOUCH_STICK_RADIUS * 2 * scaley,
		TOUCH_STICK_RADIUS * 2 * scaley,
		0
	);
}

void Engine::clear_touches()
{
	touches.clear();

}

void Engine::switch_music_out()
{
	std::string mname = Music::get_playing();
	if (mname != "") {
		music_name = mname;
	}
	Music::stop();

	std::map<std::string, Sample>::iterator it;
	for (it = sfx.begin(); it != sfx.end(); it++) {
		std::pair<const std::string, Sample> &p = *it;
		if (p.second.looping) {
			Sound::stop(p.second.sample);
		}
	}
}

void Engine::switch_music_in()
{
	restart_audio = true;
}

void Engine::set_can_move(bool can_move)
{
	this->can_move = can_move;
}

bool Engine::get_started_new_game()
{
	return started_new_game;
}

void Engine::add_extra_event(ALLEGRO_EVENT event)
{
	extra_events.push_back(event);
}

void process_dpad_events(ALLEGRO_EVENT *event)
{
	if (dont_process_dpad_events) {
		return;
	}

	if (event->type == ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN && event->joystick.button == cfg.joy_dpad_l) {
		event->joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
		event->joystick.id = 0;
		event->joystick.stick = 0;
		event->joystick.axis = 0;
		event->joystick.pos = -1;
	}
	else if (event->type == ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN && event->joystick.button == cfg.joy_dpad_r) {
		event->joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
		event->joystick.id = 0;
		event->joystick.stick = 0;
		event->joystick.axis = 0;
		event->joystick.pos = 1;
	}
	else if (event->type == ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN && event->joystick.button == cfg.joy_dpad_u) {
		event->joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
		event->joystick.id = 0;
		event->joystick.stick = 0;
		event->joystick.axis = 1;
		event->joystick.pos = -1;
	}
	else if (event->type == ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN && event->joystick.button == cfg.joy_dpad_d) {
		event->joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
		event->joystick.id = 0;
		event->joystick.stick = 0;
		event->joystick.axis = 1;
		event->joystick.pos = 1;
	}
	else if (event->type == ALLEGRO_EVENT_JOYSTICK_BUTTON_UP && event->joystick.button == cfg.joy_dpad_l) {
		event->joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
		event->joystick.id = 0;
		event->joystick.stick = 0;
		event->joystick.axis = 0;
		event->joystick.pos = 0;
	}
	else if (event->type == ALLEGRO_EVENT_JOYSTICK_BUTTON_UP && event->joystick.button == cfg.joy_dpad_r) {
		event->joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
		event->joystick.id = 0;
		event->joystick.stick = 0;
		event->joystick.axis = 0;
		event->joystick.pos = 0;
	}
	else if (event->type == ALLEGRO_EVENT_JOYSTICK_BUTTON_UP && event->joystick.button == cfg.joy_dpad_u) {
		event->joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
		event->joystick.id = 0;
		event->joystick.stick = 0;
		event->joystick.axis = 1;
		event->joystick.pos = 0;
	}
	else if (event->type == ALLEGRO_EVENT_JOYSTICK_BUTTON_UP && event->joystick.button == cfg.joy_dpad_d) {
		event->joystick.type = ALLEGRO_EVENT_JOYSTICK_AXIS;
		event->joystick.id = 0;
		event->joystick.stick = 0;
		event->joystick.axis = 1;
		event->joystick.pos = 0;
	}
}

void Engine::maybe_resume_music()
{
	if (restart_audio) {
		Music::play(music_name);

		std::map<std::string, Sample>::iterator it;
		for (it = sfx.begin(); it != sfx.end(); it++) {
			std::pair<const std::string, Sample> &p = *it;
			if (p.second.looping) {
				play_sample(p.first);
			}
		}

		restart_audio = false;
	}
}

ALLEGRO_USTR *Engine::get_entire_translation()
{
	return entire_translation;
}

void Engine::switch_mouse_in()
{
#if !defined ALLEGRO_RASPBERRYPI && !defined ALLEGRO_ANDROID && !defined ALLEGRO_IPHONE
	if (system_mouse_cursor != 0 && !((al_get_display_flags(display) & ALLEGRO_FULLSCREEN) || (al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW))) {
		mouse_in_display = true;
		al_set_mouse_cursor(display, system_mouse_cursor);
		al_show_mouse_cursor(display);
	}
#endif
}

void Engine::switch_mouse_out()
{
#if !defined ALLEGRO_RASPBERRYPI && !defined ALLEGRO_ANDROID && !defined ALLEGRO_IPHONE
	if (system_mouse_cursor != 0 && !((al_get_display_flags(display) & ALLEGRO_FULLSCREEN) || (al_get_display_flags(display) & ALLEGRO_FULLSCREEN_WINDOW))) {
		mouse_in_display = false;
		al_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT);
	}
#endif
}

#ifdef ALLEGRO_IPHONE
void Engine::show_trophy()
{
	achievement_time = al_get_time();
}
#endif

bool Engine::forced_tablet()
{
	return force_tablet;
}
	
void Engine::set_dragsize()
{
#ifdef ALLEGRO_WINDOWS
	drag_w = GetSystemMetrics(SM_CXDRAG);
	drag_h = GetSystemMetrics(SM_CYDRAG);
	SystemParametersInfoA(SPI_SETDRAGWIDTH, MAX(al_get_display_width(display), al_get_display_height(display)), NULL, 0);
	SystemParametersInfoA(SPI_SETDRAGHEIGHT, MAX(al_get_display_width(display), al_get_display_height(display)), NULL, 0);
#endif
}

void Engine::unset_dragsize()
{
#ifdef ALLEGRO_WINDOWS
	if (drag_w >= 0) {
		SystemParametersInfoA(SPI_SETDRAGWIDTH, drag_w, NULL, 0);
	}
	if (drag_h >= 0) {
		SystemParametersInfoA(SPI_SETDRAGHEIGHT, drag_h, NULL, 0);
	}
#endif
}
