cbuffer per_object : register(b0)
{
	float4x4 world_matrix;
	float4x4 WVP_matrix;
}
cbuffer per_frame : register(b1)
{
	float4x4 view_matrix;
	float4x4 projectmatrix;
	float4x4 invview_matrix;
	float4 view_position;
}
texture2D texture_test[] : register(t0);
//texture2D texture_test : register(t0);
SamplerState g_sampler : register(s0);
struct VSInput 
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
	float3 tangent  : TANGENT;
	uint4  tex_id   : TEXID;
	float4 tex_uv   : TEXUV;
};
struct PSInput
{
	float4 position : SV_POSITION;
	float4 pos_out  :POSITION;
	float4 color    : COLOR;
	uint4  tex_id   : TEXID;
	float4 tex_uv   : TEXUV;
};
PSInput VSMain(VSInput vinput)
{
	PSInput result;

	result.position = float4(vinput.position,1.0f);
	result.color = float4(vinput.normal,1.0f);
	result.pos_out = mul(float4(vinput.position, 1.0), world_matrix);
	result.pos_out = mul(result.pos_out, view_matrix);
	result.tex_id = vinput.tex_id;
	result.tex_uv = vinput.tex_uv;
	return result;
}
float4 PSMain(PSInput input) : SV_TARGET
{
	//float4 color_1 = texture_test[0].Sample(g_sampler, input.tex_uv.xy);
	return float4(1,0,0,1);
}