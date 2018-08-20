#include "camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

auto Camera::position() const -> vec3 {
	return m_position;
}

void Camera::setPosition(vec3 v) {
	m_position = v;
}

auto Camera::pitch() const -> float {
	return m_pitch;
}

auto Camera::yaw() const -> float {
	return m_yaw;
}

auto Camera::viewVector() const -> vec3 {
	vec3 v;
	v.x = 1;
	v.y = 0;
	v.z = 0;

	// rotate pitch along -y
	v = RotateY(-m_pitch, v);

	// rotate yaw along z
	v = RotateZ(m_yaw, v);

	return v;
}

void Camera::setPitch(float pitch) {
	m_pitch = pitch;
}

void Camera::setYaw(float yaw) {
	m_yaw = yaw;
}

auto Camera::moveSensitivity() const -> float {
	return m_moveSensitivity;
}

void Camera::setMoveSens(float fNewMoveSens) {
	m_moveSensitivity = fNewMoveSens;
}

void Camera::update(double t, float xDelta, float yDelta, uint8_t directions) {
	m_yaw += m_lookSensitivity * xDelta;
	m_yaw = std::fmod(m_yaw, 360.0f);

	m_pitch += m_lookSensitivity * yDelta;
	m_pitch = std::clamp(m_pitch, -90.0f, 90.0f);

	double fTmpMoveSens = m_moveSensitivity * t;

	if (directions & Up)
		m_position.z += fTmpMoveSens;

	if (directions & Down)
		m_position.z -= fTmpMoveSens;

	// TODO: If strafing and moving reduce speed to keep total move per frame constant
	if (directions & Forward) {
		m_position.x += std::cos(degToRad(m_yaw)) * fTmpMoveSens;
		m_position.y += std::sin(degToRad(m_yaw)) * fTmpMoveSens;
	}

	if (directions & Backward) {
		m_position.x -= std::cos(degToRad(m_yaw)) * fTmpMoveSens;
		m_position.y -= std::sin(degToRad(m_yaw)) * fTmpMoveSens;
	}

	if (directions & Left) {
		m_position.x += std::cos(degToRad(m_yaw + 90.0f)) * fTmpMoveSens;
		m_position.y += std::sin(degToRad(m_yaw + 90.0f)) * fTmpMoveSens;
	}

	if (directions & Right) {
		m_position.x += std::cos(degToRad(m_yaw - 90.0f)) * fTmpMoveSens;
		m_position.y += std::sin(degToRad(m_yaw - 90.0f)) * fTmpMoveSens;
	}
}

auto Camera::viewMatrix() const -> glm::mat4 {
	// in BSP v30 the z axis points up and we start looking parallel to x axis
	glm::mat4 mat = glm::eulerAngleXZ(degToRad(-m_pitch - 90.0f), degToRad(-m_yaw + 90.0f));
	mat = glm::translate(mat, -m_position); // move
	return mat;
}
