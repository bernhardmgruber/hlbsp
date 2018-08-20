#version 330

uniform samplerCube cubeSampler;

in vec3 cubeCoord;

out vec4 color;

void main() {
	color = texture(cubeSampler, cubeCoord);
}
