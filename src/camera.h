#pragma once

#include "mathlib.h"

namespace {
	constexpr auto CAMERA_MOVE_SENS = 500.0f; //old: 192
	constexpr auto CAMERA_LOOK_SENS = 0.15f;

	enum Directions {
		Forward  = 1,
		Backward = 2,
		Left     = 4,
		Right    = 8,
		Up       = 16,
		Down     = 32
	};
}

class Camera {
public:
	auto position() const -> vec3;
	void setPosition(vec3 v);

	auto viewAngles() const -> vec2;
	auto viewVector() const -> vec3;

	void setViewAngles(vec2 v);

	auto moveSensitivity() const -> float;
	void setMoveSens(float newMoveSens);

	void update(double t, float xDelta, float yDelta, uint8_t directions);

	auto viewMatrix() const -> glm::mat4;

private:
	vec3 m_position;

	float m_pitch = 0; // up down rotation
	float m_yaw   = 0; // side to side rotation

	float m_moveSensitivity = CAMERA_MOVE_SENS;
	float m_lookSensitivity = CAMERA_LOOK_SENS;
};
