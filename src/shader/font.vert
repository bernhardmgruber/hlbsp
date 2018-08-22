#version 330

uniform mat4 projection;

layout(location = 0) in vec2 vertex;
layout(location = 1) in vec2 inTexCoord;

out vec2 texCoord;

void main() {
	gl_Position = projection * vec4(vertex, 0.0, 1.0);
	texCoord = inTexCoord;
}
