#include "camera.h"

auto Camera::position() const -> vec3 {
	return m_position;
}

void Camera::setPosition(vec3 v) {
	m_position = v;
}

auto Camera::viewAngles() const -> vec2 {
	vec2 vec;
	vec.x = m_pitch;
	vec.y = m_yaw;
	return vec;
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

void Camera::setViewAngles(vec2 v) {
	m_pitch = v.x;
	m_yaw   = v.y;
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
	m_pitch = std::fmod(m_pitch + 90.0f, 180.0f) - 90.0f;

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
	glm::mat4 mat{ 1 };
	mat = glm::rotate(mat, degToRad(-m_pitch - 90.0f), { 1.0f, 0.0f, 0.0f }); // look up/down
	mat = glm::rotate(mat, degToRad(-m_yaw   + 90.0f), { 0.0f, 0.0f, 1.0f }); // look left/right
	mat = glm::translate(mat, -m_position); // move
	return mat;
}
