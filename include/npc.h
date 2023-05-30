#ifndef NPC_H
#define NPC_H

#include <string>

#include "character_map_entity.h"
#include "general_types.h"

class NPC : public Character_Map_Entity {
public:
	friend class Character_Map_Entity;

	void set_position(General::Point<float> pos);
	void logic();
	void draw();

	NPC(std::string name);
	virtual ~NPC() {}
};

#endif // NPC_H
