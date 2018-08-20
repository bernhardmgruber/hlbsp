#pragma once

#include <boost/math/constants/constants.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#define _USE_MATH_DEFINES
#include <cmath>

using glm::vec2;
using glm::vec3;

constexpr auto EPSILON = 0.03125f; // 1/32

template<typename T>
auto degToRad(T deg) -> T {
	return deg * boost::math::constants::pi<T>() / 180.0;
}

template<typename T>
auto radToDeg(T rad) -> T {
	return rad * 180.0 / boost::math::constants::pi<T>();
}

bool PointInBox(vec3 point, const int16_t vMin[3], const int16_t vMax[3]);
bool PointInPlane(vec3 point, vec3 normal, float dist);

vec3 RotateX(float a, vec3 v);
vec3 RotateY(float a, vec3 v);
vec3 RotateZ(float a, vec3 v);
