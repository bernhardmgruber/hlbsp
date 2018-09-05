#include "Camera.h"

#include "mathlib.h"

auto Camera::viewVector() const -> glm::vec3 {
	glm::vec3 v;
	v.x = 1;
	v.y = 0;
	v.z = 0;

	// rotate pitch along -y
	v = glm::rotateY(v , degToRad(-pitch));

	// rotate yaw along z
	v = glm::rotateZ(v, degToRad(yaw));

	return v;
}

void Camera::update(double t, float xDelta, float yDelta, uint8_t directions) {
	yaw += lookSensitivity * xDelta;
	yaw = std::fmod(yaw, 360.0f);

	pitch += lookSensitivity * yDelta;
	pitch = std::clamp(pitch, -90.0f, 90.0f);

	double fTmpMoveSens = moveSensitivity * t;

	if (directions & Up)
		position.z += fTmpMoveSens;

	if (directions & Down)
		position.z -= fTmpMoveSens;

	// TODO: If strafing and moving reduce speed to keep total move per frame constant
	if (directions & Forward) {
		position.x += std::cos(degToRad(yaw)) * fTmpMoveSens;
		position.y += std::sin(degToRad(yaw)) * fTmpMoveSens;
	}

	if (directions & Backward) {
		position.x -= std::cos(degToRad(yaw)) * fTmpMoveSens;
		position.y -= std::sin(degToRad(yaw)) * fTmpMoveSens;
	}

	if (directions & Left) {
		position.x += std::cos(degToRad(yaw + 90.0f)) * fTmpMoveSens;
		position.y += std::sin(degToRad(yaw + 90.0f)) * fTmpMoveSens;
	}

	if (directions & Right) {
		position.x += std::cos(degToRad(yaw - 90.0f)) * fTmpMoveSens;
		position.y += std::sin(degToRad(yaw - 90.0f)) * fTmpMoveSens;
	}
}

auto Camera::viewMatrix() const -> glm::mat4 {
	// in BSP v30 the z axis points up and we start looking parallel to x axis
	glm::mat4 mat = glm::eulerAngleXZ(degToRad(-pitch - 90.0f), degToRad(-yaw + 90.0f));
	mat = glm::translate(mat, -position); // move
	return mat;
}

auto Camera::projectionMatrix() const -> glm::mat4 {
	return glm::perspective(degToRad(fovy), static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight), 1.0f, 4000.0f);
}
