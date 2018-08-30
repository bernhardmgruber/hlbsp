#pragma once

#include <string>
#include <vector>

#include "mathlib.h"

class Camera;
class Timer;

struct Text {
	int x = 0;
	int y = 0;
	std::string text;
};

class Hud {
public:
	Hud(const Camera& camera, const Timer& timer);

	void print(std::string text);

	auto texts() const -> std::vector<Text>;
	auto fontHeight() const -> int;
	auto fontColor() const -> glm::vec3;

private:
	const Camera& m_camera;
	const Timer& m_timer;

	std::vector<std::string> m_console;
};
