#include "mathlib.h"

bool PointInBox(glm::vec3 point, const int16_t min[3], const int16_t max[3]) {
	if (((float)min[0] <= point.x && point.x <= (float)max[0] &&
			(float)min[1] <= point.y && point.y <= (float)max[1] &&
			(float)min[2] <= point.z && point.z <= (float)max[2]) ||
		((float)min[0] >= point.x && point.x >= (float)max[0] &&
			(float)min[1] >= point.y && point.y >= (float)max[1] &&
			(float)min[2] >= point.z && point.z >= (float)max[2]))
		return true;
	else
		return false;
}

bool PointInPlane(glm::vec3 point, glm::vec3 normal, float dist) {
	if (fabs(glm::dot(point, normal) - dist) < EPSILON)
		return true;
	else
		return false;
}

glm::vec3 RotateX(float a, glm::vec3 v) {
	a = degToRad(a);

	glm::vec3 res;
	res.x = v.x;
	res.y = v.y * cos(a) + v.z * -sin(a);
	res.z = v.y * sin(a) + v.z * cos(a);
	return res;
}

glm::vec3 RotateY(float a, glm::vec3 v) {
	a = degToRad(a);

	glm::vec3 res;
	res.x = v.x * cos(a) + v.z * sin(a);
	res.y = v.y;
	res.z = v.x * -sin(a) + v.z * cos(a);
	return res;
}

glm::vec3 RotateZ(float a, glm::vec3 v) {
	a = degToRad(a);

	glm::vec3 res;
	res.x = v.x * cos(a) + v.y * -sin(a);
	res.y = v.x * sin(a) + v.y * cos(a);
	res.z = v.z;
	return res;
}
