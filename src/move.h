#pragma once

#include <glm/vec3.hpp>
#include <vector>
#include "Bsp.h"

constexpr auto IN_JUMP = 1 << 1;
constexpr auto IN_FORWARD = 1 << 3;
constexpr auto IN_BACK = 1 << 4;
constexpr auto IN_MOVELEFT = 1 << 9;
constexpr auto IN_MOVERIGHT = 1 << 10;

constexpr auto FL_DUCKING = (1 << 14); // Player flag -- Player is fully crouched

struct UserCommand {
	float forwardmove;
	float sidemove;
	float upmove;
	int buttons;
	float frameTime;
	glm::vec3 viewangles; // [pitch, yaw, roll]
};

enum class MoveType {
	walk,
	fly,
	noclip
};

struct PlayerMove {
	glm::vec3 angles; // [pitch, yaw, roll]

	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;

	glm::vec3 origin;
	glm::vec3 velocity;

	glm::vec3 view_ofs;

	float frametime;
	int onground;
	int waterlevel;
	float friction;
	float waterjumptime;
	bool dead;
	UserCommand cmd;
	int oldbuttons;
	MoveType movetype;
	float gravity;
	int flags; // FL_ONGROUND, FL_DUCKING, etc.

	int usehull; // 0 = regular player hull, 1 = ducked player hull, 2 = point hull

	std::vector<const Model*> physents; // entities to clip against
	std::vector<const Model*> ladders; // called moveents in pm_shared.c
};

void playerMove(PlayerMove& pmove);
