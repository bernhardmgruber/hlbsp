#pragma once

#include "mathlib.h"
#include "move.h"

class Camera {
public:
	Camera();

	int viewportWidth = 0;
	int viewportHeight = 0;
	float fovy = 60;

	auto position() const { return pmove.origin; }
	auto& position() { return pmove.origin; }
	auto pitch() const { return pmove.angles.x; }
	auto& pitch() { return pmove.angles.x; }
	auto yaw() const { return pmove.angles.y; }
	auto& yaw() { return pmove.angles.y; }

	auto viewVector() const -> glm::vec3;
	auto viewMatrix() const -> glm::mat4;
	auto projectionMatrix() const -> glm::mat4;
	void update(const Hull& hull, UserCommand cmd);

private:
	PlayerMove pmove{};
};
