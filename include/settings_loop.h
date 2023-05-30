#ifndef SETTINGS_LOOP_H
#define SETTINGS_LOOP_H

#include <allegro5/allegro.h>

#include "general_types.h"
#include "loop.h"

class W_Translated_Button;
class W_Button;

namespace Wrap {
	class Bitmap;
}

class Settings_Loop : public Loop {
public:
	// Loop interface
	bool init();
	void top();
	bool handle_event(ALLEGRO_EVENT *event);
	bool logic();
	void draw();

	Settings_Loop();
	virtual ~Settings_Loop();

private:

	W_Translated_Button *audio_button;
	W_Translated_Button *video_button;
	W_Translated_Button *keyboard_button;
	W_Translated_Button *gamepad_button;
	W_Translated_Button *language_button;
	W_Translated_Button *restore_button;
	W_Translated_Button *licenses_button;
	W_Button *return_button;
};

#endif // SETTINGS_LOOP_H
