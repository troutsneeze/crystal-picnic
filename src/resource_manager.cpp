#include <cctype>
#include <cstdio>

#include "animation.h"
#include "atlas.h"
#include "frame.h"
#include "resource_manager.h"
#include "wrap.h"

Resource_Manager *resource_manager;

Wrap::Bitmap *Resource_Manager::reference_bitmap(std::string filename)
{
	std::map<std::string, Resource *>::iterator it =
		bitmaps.find(filename);
	
	if (it == bitmaps.end()) {
		Resource *r = new Resource;
		r->reference_count = 1;
		Wrap::Bitmap *b = Wrap::load_bitmap(filename);
		if (b) {
			r->ptr = b;
			bitmaps[filename] = r;
			return b;
		}
		else {
			delete r;
			return NULL;
		}
	}
	else {
		std::pair<std::string, Resource *> p = *it;
		Resource *r = p.second;
		r->reference_count++;
		return (Wrap::Bitmap *)r->ptr;
	}
}

void Resource_Manager::release_bitmap(std::string filename)
{
	std::map<std::string, Resource *>::iterator it =
		bitmaps.find(filename);
	
	if (it == bitmaps.end()) {
		return;
	}
	
	std::pair<std::string, Resource *> p = *it;

	Resource *r = p.second;

	r->reference_count--;
	if (r->reference_count <= 0) {
		Wrap::Bitmap *b = (Wrap::Bitmap *)r->ptr;
		if (b) {
			Wrap::destroy_bitmap(b);
		}
		delete r;
		bitmaps.erase(it);
	}
}

//--

bool Resource_Manager::check_atlas_exists(std::string name)
{
	std::map<std::string, Resource *>::iterator it =
		atlases.find(name);
	
	return it == atlases.end() ? false : true;
}

ATLAS *Resource_Manager::reference_atlas(std::string name)
{
	std::map<std::string, Resource *>::iterator it =
		atlases.find(name);
	
	if (it == atlases.end()) {
		return 0;
	}
	else {
		std::pair<std::string, Resource *> p = *it;
		Resource *r = p.second;
		r->reference_count++;
		return (ATLAS *)r->ptr;
	}
}

void Resource_Manager::set_atlas(std::string name, ATLAS *atlas)
{
	Resource *r = new Resource;
	r->reference_count = 1;
	r->ptr = atlas;
	atlases[name] = r;
}

void Resource_Manager::release_atlas(std::string name)
{
	std::map<std::string, Resource *>::iterator it =
		atlases.find(name);
	
	if (it == atlases.end()) {
		return;
	}
	
	std::pair<std::string, Resource *> p = *it;

	Resource *r = p.second;

	r->reference_count--;
	if (r->reference_count <= 0) {
		ATLAS *a = (ATLAS *)r->ptr;
		if (a) {
			atlas_destroy(a);
		}
		delete r;
		atlases.erase(it);
	}
}

//--

bool Resource_Manager::check_animation_exists(std::string name)
{
	std::map<std::string, Resource *>::iterator it =
		animations.find(name);
	
	return it == animations.end() ? false : true;
}

Animation::Shared *Resource_Manager::reference_animation(std::string name)
{
	std::map<std::string, Resource *>::iterator it =
		animations.find(name);
	
	if (it == animations.end()) {
		return 0;
	}
	else {
		std::pair<std::string, Resource *> p = *it;
		Resource *r = p.second;
		r->reference_count++;
		return (Animation::Shared *)r->ptr;
	}
}

void Resource_Manager::set_animation(std::string name, Animation::Shared *animation)
{
	Resource *r = new Resource;
	r->reference_count = 1;
	r->ptr = animation;
	animations[name] = r;
}

void Resource_Manager::release_animation(std::string name)
{
	std::map<std::string, Resource *>::iterator it =
		animations.find(name);
	
	if (it == animations.end()) {
		return;
	}
	
	std::pair<std::string, Resource *> p = *it;

	Resource *r = p.second;

	r->reference_count--;
	if (r->reference_count <= 0) {
		Animation::Shared *a = (Animation::Shared *)r->ptr;
		if (a) {
			for (size_t i = 0; i < a->frames.size(); i++) {
				delete a->frames[i];
			}
			delete a;
		}
		delete r;
		animations.erase(it);
	}
}

