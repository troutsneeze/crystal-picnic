#ifndef ANIMATION_SET_H
#define ANIMATION_SET_H

#include <map>
#include <string>
#include <vector>

#include "general_types.h"

class Animation;
struct ATLAS;

class Animation_Set {
public:
	void sync(Animation_Set *sync_to);

	bool load(std::string path, std::string xml_path = "");

	void set_sub_animation(int index);
	int get_num_animations();

	Animation *get_sub_animation(std::string subName);
	bool set_sub_animation(std::string subName, bool force = false);
	void set_frame(int frame);
	void set_prefix(std::string prefix);
	std::string get_prefix();
	bool check_sub_animation_exists(std::string subName);
	
	int get_length(std::string anim_name);

	std::string get_sub_animation_name();
	int get_frame();
	Animation* get_current_animation();

	void reset();
	int update(int step);
	int update();
	
	bool load();

	Animation_Set();
	~Animation_Set();
private:
	std::string name;
	std::vector<Animation *> anims;
	int curr_anim;
	std::string prefix;
	
	std::string load_path;
	std::string load_xml_path;

	double push_time;

	std::map<std::string, bool> exists_map;
	std::map<std::string, int> index_map;

	ATLAS *atlas;
};

#endif
