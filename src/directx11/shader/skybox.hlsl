
cbuffer TransformConstants : register(b0) {
	float4x4 m;
};

static const float3 points[36] = {
	// x pos
	float3( 1000.0, -1000.0,  1000.0),
	float3( 1000.0, -1000.0, -1000.0),
	float3( 1000.0,  1000.0,  1000.0),
	float3( 1000.0,  1000.0, -1000.0),
	float3( 1000.0,  1000.0,  1000.0),
	float3( 1000.0, -1000.0, -1000.0),

	// x neg
	float3(-1000.0, -1000.0, -1000.0),
	float3(-1000.0, -1000.0,  1000.0),
	float3(-1000.0,  1000.0, -1000.0),
	float3(-1000.0,  1000.0,  1000.0),
	float3(-1000.0,  1000.0, -1000.0),
	float3(-1000.0, -1000.0,  1000.0),

	// y pos
	float3( 1000.0,  1000.0, -1000.0),
	float3(-1000.0,  1000.0, -1000.0),
	float3( 1000.0,  1000.0,  1000.0),
	float3(-1000.0,  1000.0,  1000.0),
	float3( 1000.0,  1000.0,  1000.0),
	float3(-1000.0,  1000.0, -1000.0),

	// y neg
	float3(-1000.0, -1000.0,  1000.0),
	float3(-1000.0, -1000.0, -1000.0),
	float3( 1000.0, -1000.0, -1000.0),
	float3(-1000.0, -1000.0,  1000.0),
	float3( 1000.0, -1000.0, -1000.0),
	float3( 1000.0, -1000.0,  1000.0),

	// z pos
	float3(-1000.0,  1000.0,  1000.0),
	float3(-1000.0, -1000.0,  1000.0),
	float3( 1000.0,  1000.0,  1000.0),
	float3( 1000.0, -1000.0,  1000.0),
	float3( 1000.0,  1000.0,  1000.0),
	float3(-1000.0, -1000.0,  1000.0),

	// z neg
	float3(-1000.0, -1000.0, -1000.0),
	float3(-1000.0,  1000.0, -1000.0),
	float3( 1000.0, -1000.0, -1000.0),
	float3( 1000.0,  1000.0, -1000.0),
	float3( 1000.0, -1000.0, -1000.0),
	float3(-1000.0,  1000.0, -1000.0)
};

struct PixelShaderInput {
	float4 pixelPosition : SV_POSITION;
	float3 cubeCoord : TEXCOORD;
};

PixelShaderInput main_vs(uint vertexID : SV_VertexID) {
	PixelShaderInput r;
	r.pixelPosition = mul(m, float4(points[vertexID], 1.0));
	r.cubeCoord = saturate(points[vertexID]);
	r.cubeCoord.z = -r.cubeCoord.z;
	return r;
}

TextureCube cubeTexture : register(t0);

SamplerState cubeSampler {
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

float4 main_ps(PixelShaderInput pin) : SV_TARGET {
	return cubeTexture.SampleLevel(cubeSampler, pin.cubeCoord, 0);
}
