cbuffer VertexShader_Constants : register(b0)
{
	row_major float4x4 g_proj;
	row_major float4x4 g_world_to_camera;
};

cbuffer PixelShader_Constants : register(b1)
{
	float3 g_eye_p_in_world;
};

struct Vertex
{
	float3 v : IA_Vertex;
	float3 t : IA_Tangent;
	float3 b : IA_Bitangent;
	float3 n : IA_Normal;
	float2 uv : IA_UV;
};

struct Model_Instance
{
	float3 p;
	float3 scale;
	float4 colour;
};

struct VS_Output
{
	float4 p : SV_Position;
	float3 world_p : World_P;
	float4 colour : Colour;
};

StructuredBuffer<Model_Instance> g_model_instances : register(t0);

VS_Output vs_main(Vertex vertex, uint iid : SV_InstanceID)
{
	VS_Output result;
	
	Model_Instance instance = g_model_instances[iid];
	
	float4 world_coordinate = float4(vertex.v * instance.scale + instance.p, 1.0f);
	float4 camera_coordinate = mul(g_world_to_camera, world_coordinate);
	result.p = mul(g_proj, camera_coordinate);
	result.world_p = world_coordinate.xyz;
	result.colour = instance.colour;
	return(result);
}

float4 ps_main(VS_Output ps_inp) : SV_Target
{
	return(ps_inp.colour);
}