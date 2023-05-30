#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <map>
#include <string>

#include "animation.h"
#include "general_types.h"

struct ATLAS;

namespace Wrap {
	class Bitmap;
}

class Resource_Manager {
public:
	Wrap::Bitmap *reference_bitmap(std::string filename);
	void release_bitmap(std::string filename);

	bool check_atlas_exists(std::string name);
	ATLAS *reference_atlas(std::string name);
	void set_atlas(std::string name, ATLAS *atlas);
	void release_atlas(std::string name);

	bool check_animation_exists(std::string name);
	Animation::Shared *reference_animation(std::string name);
	void set_animation(std::string name, Animation::Shared *animation);
	void release_animation(std::string name);

private:
	struct Resource {
		unsigned int reference_count;
		void *ptr;
	};

	std::map<std::string, Resource *> bitmaps;
	std::map<std::string, Resource *> atlases;
	std::map<std::string, Resource *> animations;
};

extern Resource_Manager *resource_manager;


#endif // RESOURCE_MANAGER_H
