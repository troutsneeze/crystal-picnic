#ifndef LANGUAGE_CONFIG_LOOP_H
#define LANGUAGE_CONFIG_LOOP_H

#include <string>
#include <vector>

#include <allegro5/allegro.h>

#include "general_types.h"
#include "loop.h"

class W_Scrolling_List;
class W_Vertical_Scrollbar;
class W_Button;

namespace Wrap {
	class Bitmap;
}

class Language_Config_Loop : public Loop {
public:
	// Loop interface
	bool init();
	void top();
	bool handle_event(ALLEGRO_EVENT *event);
	bool logic();
	void draw();

	Language_Config_Loop();
	virtual ~Language_Config_Loop();

private:

	W_Scrolling_List *language_list;
	W_Vertical_Scrollbar *scrollbar;
	W_Button *save_button;
	W_Button *cancel_button;
	bool list_was_activated;
	std::vector<std::string> untranslated_languages;
};

#endif // LANGUAGE_CONFIG_LOOP_H
