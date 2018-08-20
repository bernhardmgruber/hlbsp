#version 330

uniform bool unit1Enabled;
uniform bool unit2Enabled;

uniform bool nightvision;

uniform sampler2D tex1;
uniform sampler2D tex2;

in vec2 texCoord;
in vec2 lightmapCoord;

out vec4 color;

void Nightvision() {
	vec4 c1 = color / 2.0;
	c1 += texture2D(tex1, texCoord.st + 0.01);
	c1 += texture2D(tex1, texCoord.st + 0.02);
	c1 += texture2D(tex1, texCoord.st + 0.03);

	vec4 c2 = color / 2.0;
	c2 += texture2D(tex2, lightmapCoord.st + 0.01);
	c2 += texture2D(tex2, lightmapCoord.st + 0.02);
	c2 += texture2D(tex2, lightmapCoord.st + 0.03);

	vec4 c = c1 * c2;
	c.r *= 0.2;
	c.b *= 0.2;

	color = c;
}

void main() {
	vec4 texel1 = vec4(1.0);
	vec4 texel2 = vec4(1.0);

	if (unit1Enabled)
		texel1 = texture2D(tex1, texCoord);
	if (unit2Enabled)
		texel2 = texture2D(tex2, lightmapCoord);

	color = vec4(texel1.rgb * texel2.rgb, texel1.a);

	if (nightvision)
		Nightvision();

	// brightness
	color *= 2;
}
