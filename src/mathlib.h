#pragma once

#include <boost/math/constants/constants.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#define _USE_MATH_DEFINES
#include <cmath>

constexpr auto EPSILON = 0.03125f; // 1/32

template<typename T>
auto degToRad(T deg) -> T {
	return deg * boost::math::constants::pi<T>() / 180.0;
}

template<typename T>
auto radToDeg(T rad) -> T {
	return rad * 180.0 / boost::math::constants::pi<T>();
}

bool PointInBox(glm::vec3 point, const int16_t vMin[3], const int16_t vMax[3]);
bool PointInPlane(glm::vec3 point, glm::vec3 normal, float dist);

glm::vec3 RotateX(float a, glm::vec3 v);
glm::vec3 RotateY(float a, glm::vec3 v);
glm::vec3 RotateZ(float a, glm::vec3 v);
