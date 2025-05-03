#define LightType_PointLight 0
#define LightCount_Max 8
struct Light
{
	float3 p;
	uint type;
	float4 colour;
};

cbuffer VertexShader_Constants : register(b0)
{
	row_major float4x4 g_proj;
	row_major float4x4 g_world_to_camera;
};

cbuffer PixelShader_Constants : register(b1)
{
	float3 g_eye_p_in_world;
	uint g_lights_count;
	Light g_lights[LightCount_Max];
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
	
	float3 normal : Normal;
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
	result.normal = vertex.n;
	return(result);
}

float4 ps_main(VS_Output ps_inp) : SV_Target
{
	float4 c_final = float4(0, 0, 0, 0);
	float4 c_accum = float4(0, 0, 0, 0);
	
	float3 N = normalize(ps_inp.normal);
	float3 V = normalize(g_eye_p_in_world - ps_inp.world_p);

	Light light = g_lights[0];
	switch (light.type)
	{
		case LightType_PointLight:
		{
			float3 L = light.p - ps_inp.world_p;
			float L_d = length(L);
			L /= L_d;
			float3 R = normalize(reflect(-L, N));

			float rmax = 50.0f;
			float r0 = 3.7;
			float win = pow(max(1.0f - pow(L_d / rmax, 4.0f), 0.0f), 2.0f);
			float atten = win*(r0*r0 / (L_d*L_d*0.5f + L_d*4+ 1));

			float N_dot_L = max(dot(N, L), 0);
			//float R_dot_V = max(dot(normalize(L + V), N), 0);
			float R_dot_V = max(dot(R, V), 0);
			float4 specular = pow(R_dot_V, 4) * light.colour;
			float4 diffuse = N_dot_L * light.colour * ps_inp.colour;
			c_accum = saturate((diffuse + specular)*atten + c_accum);
		} break;
	}

	float4 c_ambient = float4(0.05,0.05,0.05,1);
	c_final = saturate(c_ambient * ps_inp.colour + c_accum);
	
	return(c_final);
}

float4 ps_nolight(VS_Output ps_inp) : SV_Target
{
	return(ps_inp.colour);
}