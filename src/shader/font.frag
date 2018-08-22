#version 330

uniform sampler2D tex;
uniform vec3 textColor;

in vec2 texCoord;

out vec4 color;

void main() {
	color = vec4(textColor, texture(tex, texCoord).r);
}
