#version 130
#extension GL_ARB_explicit_attrib_location : require

uniform bool unit1Enabled;
uniform bool unit2Enabled;

uniform bool flashlight;
uniform bool nightvision;

uniform mat4 matrix;

out vec4 diffuse, ambientGlobal, ambient;
out vec3 normal, lightDir, halfVector;
out float dist;
out vec2 texCoord;
out vec2 lightmapCoord;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inLightmapCoord;

void main()
{
	if (flashlight)
	{
		vec4 ecPos;
		vec3 aux;

		/* first transform the normal into eye space and normalize the result */
		normal = normalize(gl_NormalMatrix * inNormal);

		/* now normalize the light's direction. Note that according to the
		OpenGL specification, the light is stored in eye space.*/
		ecPos = gl_ModelViewMatrix * vec4(inPosition, 1.0);
		aux = vec3(gl_LightSource[0].position-ecPos);
		lightDir = normalize(aux);

		/* compute the distance to the light source to a varying variable*/
		dist = length(aux);

		/* Normalize the halfVector to pass it to the fragment shader */
		halfVector = normalize(gl_LightSource[0].halfVector.xyz);

		/* Compute the diffuse, ambient and globalAmbient terms */
		diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
		ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
		ambientGlobal = gl_LightModel.ambient * gl_FrontMaterial.ambient;
	}

	gl_Position = matrix * vec4(inPosition, 1.0);

	if(unit1Enabled)
		texCoord = inTexCoord;
	if(unit2Enabled)
		lightmapCoord = inLightmapCoord;
}
