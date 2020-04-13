#pragma once

#include <glm/vec3.hpp>

struct Hull;

auto move(const Hull& hull, glm::vec3 start, glm::vec3 end) -> glm::vec3;
