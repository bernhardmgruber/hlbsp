#version 330

uniform bool unit1Enabled;
uniform bool unit2Enabled;

uniform mat4 matrix;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inLightmapCoord;

out vec2 texCoord;
out vec2 lightmapCoord;

void main() {
	gl_Position = matrix * vec4(inPosition, 1.0);

	if (unit1Enabled)
		texCoord = inTexCoord;
	if (unit2Enabled)
		lightmapCoord = inLightmapCoord;
}
