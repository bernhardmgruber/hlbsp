#include "Camera.h"

#include "global.h"
#include "mathlib.h"

Camera::Camera(PlayerMove& pmove) : pmove(pmove) {}

auto Camera::viewVector() const -> glm::vec3 {
	return pmove.forward;
}

auto Camera::viewMatrix() const -> glm::mat4 {
	auto& pitch = pmove.angles.x;
	auto& yaw = pmove.angles.y;
	auto& position = pmove.origin;

	// in BSP v30 the z axis points up and we start looking parallel to x axis
	glm::mat4 mat = glm::eulerAngleXZ(degToRad(pitch - 90.0f), degToRad(-yaw + 90.0f));
	mat = glm::translate(mat, -position); // move
	return mat;
}

auto Camera::projectionMatrix() const -> glm::mat4 {
	return glm::perspective(degToRad(fovy), static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight), 1.0f, 4000.0f);
}
