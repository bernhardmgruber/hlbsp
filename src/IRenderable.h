#pragma once

#include <glm/mat4x4.hpp>

struct RenderSettings {
	glm::mat4 projection;
	float pitch;
	float yaw;
	glm::mat4 view;
};

class IRenderable {
public:
	virtual ~IRenderable() = default;
	virtual void render(const RenderSettings& settings) = 0;
};
