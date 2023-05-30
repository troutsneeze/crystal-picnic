#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <string>
#include <vector>

#include <allegro5/allegro.h>

#include "general_types.h"

class Frame;

class Animation {
public:
	friend class Animation_Set;

	enum Loop_Mode {
		LOOP_NORMAL = 0,
		LOOP_PINGPONG
	};

	struct Shared {
		std::string id;

		std::vector<Frame *> frames;
		int num_frames;
		std::string name;
		bool looping;
		Loop_Mode loop_mode;
		std::vector<std::string> tags;

		// for rotating animations
		bool rotating;
		float angle_inc;
		General::Point<float> rotation_center;
		
		int loop_start;
	};

	/* Tags */
	void set_tags(std::vector<std::string> tags);
	std::vector<std::string> get_tags()
	{
		return shared->tags;
	}

	bool has_tag(std::string tagName) {
		for (int i = 0; i < (int)shared->tags.size(); i++) {
			if (shared->tags[i] == tagName)
				return true;
		}

		return false;
	}

	std::string get_tag(std::string tag_start) {
		for (int i = 0; i < (int)shared->tags.size(); i++) {
			if (shared->tags[i].substr(0, tag_start.length()) == tag_start)
				return shared->tags[i];
		}

		return "";
	}

	/* Returns false on fail.
	 * frame is not copied so should not be destroyed.
	 */
	bool add_frame(Frame *frame);
	int get_length();

	/* -1 for current
	 */
	Frame *get_frame(int num)
	{
		return shared->frames[num];
	}

	Frame *get_current_frame()
	{
		return shared->frames[current_frame];
	}

	bool is_finished();

	unsigned int get_current_frame_num()
	{
		return current_frame;
	}


	unsigned int get_num_frames()
	{
		return shared->num_frames;
	}


	std::string get_name()
	{
		return shared->name;
	}


	/* Returns how many frames passed
	 * Can go backwards.
	 * Step is in milliseconds.
	 */
	int update(int step);
	int update();

	void set_looping(bool l);
	void set_loop_mode(Loop_Mode m);
	void set_loop_start(int loop_start);
	bool is_looping() { return shared->looping; }

	void reset();
	void set_frame(int frame);

	void draw(int x, int y, int flags);
	void draw_rotated(int x, int y, int flags);
	void draw_tinted(ALLEGRO_COLOR tint, int x, int y, int flags);
	void draw_add_tinted(ALLEGRO_COLOR tint, float p, int x, int y, int flags);
	void draw_tinted_rotated(ALLEGRO_COLOR tint, int x, int y, int flags);
	void draw_add_tinted_rotated(ALLEGRO_COLOR tint, float p, int x, int y, int flags);
	void draw_scaled(int sx, int sy, int sw, int sh, int dx, int dy,
		int dw, int dh, int flags);

	float get_angle() { return angle; }
	void set_angle(float a) { angle = a; }

	void set_angle_inc(float ai) { shared->angle_inc = ai; }
	void set_rotation_center(General::Point<float> p) {
		shared->rotation_center = p;
	}
	
	Animation(std::string name, Shared *shared);
	/* Frames are destroyed
	 */
	~Animation();

protected:
	void wrap();
	
	Shared *shared;

	int current_frame;
	int count;
	int increment;
	float angle;
};

#endif

