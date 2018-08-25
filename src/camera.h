#pragma once

#include "mathlib.h"

namespace {
	constexpr auto CAMERA_MOVE_SENS = 500.0f; //old: 192
	constexpr auto CAMERA_LOOK_SENS = 0.15f;

	enum Directions {
		Forward = 1,
		Backward = 2,
		Left = 4,
		Right = 8,
		Up = 16,
		Down = 32
	};
}

class Camera {
public:
	vec3 position;
	float pitch = 0; // up down rotation
	float yaw = 0;   // side to side rotation

	int viewportWidth = 0;
	int viewportHeight = 0;
	float fovy = 60;

	float moveSensitivity = CAMERA_MOVE_SENS;
	float lookSensitivity = CAMERA_LOOK_SENS;

	auto viewVector() const -> vec3;
	auto viewMatrix() const -> glm::mat4;
	auto projectionMatrix() const -> glm::mat4;
	void update(double t, float xDelta, float yDelta, uint8_t directions);
};
