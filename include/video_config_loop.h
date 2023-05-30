#ifndef VIDEO_CONFIG_LOOP_H
#define VIDEO_CONFIG_LOOP_H

#include <vector>

#include <allegro5/allegro.h>

#include <tgui2_widgets.hpp>

#include "general_types.h"
#include "loop.h"

class W_Scrolling_List;
class W_Vertical_Scrollbar;
class W_Button;

namespace Wrap {
	class Bitmap;
}

class Video_Config_Loop : public Loop {
public:
	// Loop interface
	bool init();
	void top();
	bool handle_event(ALLEGRO_EVENT *event);
	bool logic();
	void draw();

	Video_Config_Loop();
	virtual ~Video_Config_Loop();

private:
	struct Mode {
		int width;
		int height;
		bool windowed_only;
		bool exact;
	};

#if !(defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID)
	W_Scrolling_List *mode_list;
	W_Vertical_Scrollbar *scrollbar;
	TGUI_Checkbox *fs_checkbox;
	TGUI_Checkbox *linear_checkbox;
#endif
	TGUI_Checkbox *advanced_effects_checkbox;
	W_Button *save_button;
	W_Button *cancel_button;
	std::vector<Mode> modes;
	bool list_was_activated;
};

#endif // VIDEO_CONFIG_LOOP_H
