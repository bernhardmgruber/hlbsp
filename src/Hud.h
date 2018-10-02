#pragma once

#include <string>
#include <vector>

#include "mathlib.h"

struct ImDrawData;
class Camera;
class Timer;

class Hud {
public:
	Hud(const Camera& camera, const Timer& timer);

	void print(std::string text);

	auto drawData() const -> ImDrawData*;
	auto fontHeight() const -> int;
	auto fontColor() const -> glm::vec3;

private:
	const Camera& m_camera;
	const Timer& m_timer;

	std::vector<std::string> m_console;
};
