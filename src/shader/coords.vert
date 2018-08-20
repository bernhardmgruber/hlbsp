#version 330

uniform mat4 matrix;

const vec3 points[12] = vec3[](
	vec3( 0.0,  0.0,  0.0),
	vec3( 1.0,  0.0,  0.0),
	vec3( 0.0,  0.0,  0.0),
	vec3(-1.0,  0.0,  0.0),
	vec3( 0.0,  0.0,  0.0),
	vec3( 0.0,  1.0,  0.0),
	vec3( 0.0,  0.0,  0.0),
	vec3( 0.0, -1.0,  0.0),
	vec3( 0.0,  0.0,  0.0),
	vec3( 0.0,  0.0,  1.0),
	vec3( 0.0,  0.0,  0.0),
	vec3( 0.0,  0.0, -1.0)
);

const vec3 colors[6] = vec3[](
	vec3(1.0,  0.0, 0.0),
	vec3(0.5,  0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.5, 0.0),
	vec3(0.0, 0.0, 1.0),
	vec3(0.0, 0.0, 0.5)
);

out vec3 coordColor;

void main() {
	const float axesLength = 4000;

	gl_Position = matrix * vec4(points[gl_VertexID] * axesLength, 1.0);
	coordColor = colors[gl_VertexID / 2];
}
