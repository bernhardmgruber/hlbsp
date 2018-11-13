
cbuffer Whatever : register(b0) {
	float4x4 m;

	bool unit1Enabled;
	bool unit2Enabled;

	//bool flashlight;
	bool nightvision;

	bool alphaTest;
};

struct VertexShaderInput {
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD0;
	float2 lightmapCoord : TEXCOORD1;
};

struct PixelShaderInput {
	float4 pixelPosition : SV_Position;
	float2 texCoord : TEXCOORD0;
	float2 lightmapCoord : TEXCOORD1;
};

PixelShaderInput main_vs(VertexShaderInput vin) {
	PixelShaderInput r;
	r.pixelPosition = mul(m, float4(vin.position, 1.0f));
	r.texCoord = vin.texCoord;
	r.lightmapCoord = vin.lightmapCoord;
	return r;
}

SamplerState samp {
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Repeat;
	AddressV = Repeat;
};

Texture2D tex1 : register(t0);
Texture2D tex2 : register(t1);

float4 Nightvision(PixelShaderInput pin, float4 color) {
	float4 c1 = color / 2.0;
	
	c1 += tex1.Sample(samp, pin.texCoord + 0.01);
	c1 += tex1.Sample(samp, pin.texCoord + 0.02);
	c1 += tex1.Sample(samp, pin.texCoord + 0.03);

	float4 c2 = color / 2.0;
	c2 += tex2.Sample(samp, pin.texCoord + 0.01);
	c2 += tex2.Sample(samp, pin.texCoord + 0.02);
	c2 += tex2.Sample(samp, pin.texCoord + 0.03);

	float4 c = c1 * c2;
	c.r *= 0.2;
	c.b *= 0.2;

	return c;
}

float4 main_ps(PixelShaderInput pin) : SV_Target {
	float4 texel1 = 1.0f;
	float4 texel2 = 1.0f;

	if (unit1Enabled) {
		texel1 = tex1.Sample(samp, pin.texCoord);
		if (alphaTest && texel1.a < 0.25)
			discard;
	}

	if (unit2Enabled)
		texel2 = tex2.Sample(samp, pin.lightmapCoord);

	float4 color = float4(texel1.rgb * texel2.rgb, texel1.a);

	if (nightvision)
		color = Nightvision(pin, color);

	// brightness
	color *= 2;

	return color;
}
