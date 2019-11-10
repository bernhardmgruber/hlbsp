#pragma once

#include <boost/math/constants/constants.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cmath>

template<typename T>
auto degToRad(T deg) -> T {
	return deg * boost::math::constants::pi<T>() / T{180};
}

template<typename T>
auto radToDeg(T rad) -> T {
	return rad * T{180} / boost::math::constants::pi<T>();
}

auto PointInBox(glm::vec3 point, const int16_t vMin[3], const int16_t vMax[3]) -> bool;
auto PointInPlane(glm::vec3 point, glm::vec3 normal, float dist) -> bool;
