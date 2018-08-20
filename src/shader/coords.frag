#version 330

in vec3 coordColor;

out vec4 color;

void main() {
	color = vec4(coordColor, 1.0);
}
