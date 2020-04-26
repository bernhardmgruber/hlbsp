#pragma once

#include <glm/vec3.hpp>

struct Hull;

constexpr auto IN_JUMP = 1 << 1;

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
};

void playerMove(const Hull& hull, PlayerMove& pmove);
