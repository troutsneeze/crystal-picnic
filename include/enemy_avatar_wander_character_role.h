#ifndef ENEMY_AVATAR_WANDER_CHARACTER_ROLE_H
#define ENEMY_AVATAR_WANDER_CHARACTER_ROLE_H

#include "general_types.h"
#include "wander_character_role.h"

class Area_Manager;

class Enemy_Avatar_Wander_Character_Role : public Wander_Character_Role
{
public:
	enum Battle_Event_Type {
		BATTLE_EVENT_SIGHTED = 0,
		BATTLE_EVENT_TRIPPED,
		BATTLE_EVENT_SLIPPED
	};

	void update(Area_Manager *area);

	Enemy_Avatar_Wander_Character_Role(
		Character_Map_Entity *character,
		int max_distance_from_home,
		double pause_min,
		double pause_max);
	virtual ~Enemy_Avatar_Wander_Character_Role();

private:
	double next_check;
};

#endif // ENEMY_AVATAR_WANDER_CHARACTER_ROLE_H
