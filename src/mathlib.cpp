#include "mathlib.h"

namespace {
	constexpr auto EPSILON = 1 / 32.0f;
}

auto PointInBox(glm::vec3 point, const int16_t min[3], const int16_t max[3]) -> bool {
	return (min[0] <= point.x && point.x <= max[0] && min[1] <= point.y && point.y <= max[1] && min[2] <= point.z && point.z <= max[2]) ||
		   (min[0] >= point.x && point.x >= max[0] && min[1] >= point.y && point.y >= max[1] && min[2] >= point.z && point.z >= max[2]);
}

auto PointInPlane(glm::vec3 point, glm::vec3 normal, float dist) -> bool {
	return std::abs(glm::dot(point, normal) - dist) < EPSILON;
}
