
cbuffer TransformConstants : register(b0) {
	float4x4 mv;
};

static const float3 points[12] = {
	float3( 0.0,  0.0,  0.0),
	float3( 1.0,  0.0,  0.0),
	float3( 0.0,  0.0,  0.0),
	float3(-1.0,  0.0,  0.0),
	float3( 0.0,  0.0,  0.0),
	float3( 0.0,  1.0,  0.0),
	float3( 0.0,  0.0,  0.0),
	float3( 0.0, -1.0,  0.0),
	float3( 0.0,  0.0,  0.0),
	float3( 0.0,  0.0,  1.0),
	float3( 0.0,  0.0,  0.0),
	float3( 0.0,  0.0, -1.0)
};

static const float3 colors[6] = {
	float3(1.0, 0.0, 0.0),
	float3(0.5, 0.0, 0.0),
	float3(0.0, 1.0, 0.0),
	float3(0.0, 0.5, 0.0),
	float3(0.0, 0.0, 1.0),
	float3(0.0, 0.0, 0.5)
};

struct PixelShaderInput {
	float4 pixelPosition : SV_POSITION;
	float3 coordColor : COLOR;
};

PixelShaderInput main_vs(uint vertexID : SV_VertexID) {
	const float axesLength = 4000;

	PixelShaderInput r;
	r.pixelPosition = mul(mv, float4(points[vertexID] * axesLength, 1.0f));
	r.coordColor = colors[vertexID / 2];
	return r;
}

float4 main_ps(PixelShaderInput pin) : SV_Target {
	return float4(pin.coordColor, 1.0);
}
