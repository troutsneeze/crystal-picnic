local last_attack

function start(this_id)
	id = this_id
	load_sample("sfx/acorn_drop.ogg", false)
	load_sample("sfx/grenade_tink.ogg", false)
	load_sample("sfx/bomb.ogg", false)
	load_bitmap("misc_graphics/particles/grenade.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/1.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/2.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/3.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/4.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/5.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/6.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/7.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/8.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/9.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/10.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/11.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/12.png")
	load_bitmap("misc_graphics/particles/mushroom_cloud/13.png")
	load_bitmap("misc_graphics/particles/fire16-1.png")
	load_bitmap("misc_graphics/particles/fire16-2.png")
	load_bitmap("misc_graphics/particles/fire16-3.png")
	load_bitmap("misc_graphics/particles/smoke16-1.png")
	load_bitmap("misc_graphics/particles/smoke16-2.png")
	load_bitmap("misc_graphics/particles/smoke16-3.png")
	set_hp(id, 15)

	last_attack = get_time()
end

function stop()
	destroy_sample("sfx/acorn_drop.ogg")
	destroy_sample("sfx/grenade_tink.ogg")
	destroy_sample("sfx/bomb.ogg")
	destroy_bitmap("misc_graphics/particles/grenade.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/1.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/2.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/3.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/4.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/5.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/6.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/7.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/8.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/9.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/10.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/11.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/12.png")
	destroy_bitmap("misc_graphics/particles/mushroom_cloud/13.png")
	destroy_bitmap("misc_graphics/particles/fire16-1.png")
	destroy_bitmap("misc_graphics/particles/fire16-2.png")
	destroy_bitmap("misc_graphics/particles/fire16-3.png")
	destroy_bitmap("misc_graphics/particles/smoke16-1.png")
	destroy_bitmap("misc_graphics/particles/smoke16-2.png")
	destroy_bitmap("misc_graphics/particles/smoke16-3.png")
end

function get_attack_sound()
	return ""
end

function get_should_auto_attack()
	return false
end

local MIN_DIST = 50
local THROW_DIST = 110

function logic()
	local x, y = get_entity_position(id)
	local h = get_battle_height()
	if (y >= h) then
		remove_entity(id)
	end
end

function decide()
	local player_id = ai_get(id, "A_PLAYER")

	if (player_id == nil) then
		return "rest " .. (LOGIC_MILLIS/1000) .. " nostop"
	end

	local dist = ai_get(id, "entity_distance " .. player_id)
	local x, y, px, py =
		ai_get(id, "entity_positions_by_id " .. id .. " " .. player_id)
	local hp = ai_get(id, "hp " .. id)

	if (dist < MIN_DIST) then
		local ret = "seek pos "
		if (px < x) then
			return ret .. (px + THROW_DIST) .. " " .. y
		else
			return ret .. (px - THROW_DIST) .. " " .. y
		end
	elseif (hp > 0 and dist < THROW_DIST and get_time() > last_attack+3) then
		local player_id = ai_get(id, "A_PLAYER")

		if (player_id == nil) then
			return "rest " .. (LOGIC_MILLIS/1000) .. " nostop"
		end

		local x, y, px, py = ai_get(id, "entity_positions_by_id " .. id .. " " .. player_id)
		if (x < px) then
			right = true
		else
			right = false
		end

		set_entity_right(id, right)

		local hp = ai_get(id, "hp " .. id)
				
		play_sample("sfx/acorn_drop.ogg", 1, 0, 1)
		
		local x, y = ai_get(id, "entity_positions_by_id " .. id)
		
		-- add grenade
		local pgid = add_particle_group("grenade", 0, PARTICLE_HURT_PLAYER, "grenade")
		local grenade = add_particle(
			pgid,
			8, 8,
			1, 1, 1, 1,
			0,
			0,
			right,
			true
		)
		set_particle_damage(grenade, 3)
		local offs
		if (right) then
			offs = 10
		else
			offs = -10
		end
		set_particle_position(grenade, x + offs, y - 12)
		local vx
		if (right) then
			vx = 4.0
			set_particle_blackboard(grenade, 5, 1)
		else
			vx = -4.0
			set_particle_blackboard(grenade, 5, -1)
		end
		vy = -2.5

		set_particle_blackboard(grenade, 0, vx)
		set_particle_blackboard(grenade, 1, vy)
		set_particle_blackboard(grenade, 2, 0) -- num bounces
		set_particle_blackboard(grenade, 3, 3) -- time till explode
		set_particle_blackboard(grenade, 4, 0) -- angle

		last_attack = get_time()

		return "attack nostop"
	else
		local r = (rand(1000)/1000)
		return "seek within " ..
			THROW_DIST .. " 20 A_PLAYER nostop rest " ..
			r .. " nostop"
	end
end

function die()
	if (rand(3) == 0) then
		local coin = add_battle_enemy("coin1")
		local x, y = get_entity_position(id)
		set_entity_position(coin, x, y-5)
	end
end
