#define LightType_PointLight 0
#define LightType_DirectionalLight 1
#define LightCount_Max 8
struct Light
{
	float3 p;
	uint type;
	float4 colour;
	float3 dir;
	float z_far;
};

cbuffer VertexShader_Constants : register(b0)
{
	row_major float4x4 g_proj;
	row_major float4x4 g_world_to_camera;
	row_major float4x4 g_light_proj;
	row_major float4x4 g_world_to_light;
};

cbuffer PixelShader_Constants : register(b1)
{
	Light g_light;
	float3 g_eye_p_in_world;
	float g_cb1_pad_a[1];
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
	float3 rotation;
	float4 colour;
};

struct VS_Output
{
	float4 p : SV_Position;
	float3 world_p : World_P;
	float4 colour : Colour;

	float4 p_light_proj : Light_Position_Proj;
	
	float3 normal : Normal;

	float2 uv : Tex_UV;
	float4 ssao_p : SSAO_P;
};

StructuredBuffer<Model_Instance> g_model_instances : register(t0);
Texture2D<float4> g_shadow_map : register(t1);
Texture2D<float> g_ssao_map : register(t2);
TextureCube<float> g_shadow_cubemap : register(t3);

SamplerState g_shadow_map_sampler : register(s0);
SamplerState g_point_sampler_clamp : register(s1);

float3x3 create_rotation(float3 xyz)
{
	float3x3 z = float3x3(
		cos(xyz.z), 0, -sin(xyz.z),
		0, 1, 0,
		sin(xyz.z), 0, cos(xyz.z)
	);

	float3x3 x = float3x3(
		1, 0, 0,
		0, cos(xyz.x), -sin(xyz.x),
		0, sin(xyz.x), cos(xyz.x)
	);

	float3x3 result = mul(x, z);

	return(result);
}

VS_Output vs_main(Vertex vertex, uint iid : SV_InstanceID)
{
	VS_Output result;
	
	Model_Instance instance = g_model_instances[iid];
	
	float3x3 rot = create_rotation(instance.rotation);
	float4 world_coordinate = float4(mul(rot, vertex.v) * instance.scale + instance.p, 1.0f);
	float4 camera_coordinate = mul(g_world_to_camera, world_coordinate);
	result.p = mul(g_proj, camera_coordinate);
	result.world_p = world_coordinate.xyz;
	result.colour = instance.colour;
	result.ssao_p = mul(g_proj, camera_coordinate);
	result.p_light_proj = mul(g_light_proj, mul(g_world_to_light, world_coordinate));
	result.normal = mul(rot, vertex.n);
	result.uv = vertex.uv;
	return(result);
}

float4 ps_main(VS_Output ps_inp) : SV_Target
{
	float3 c_final = float3(0, 0, 0);
	float3 c_accum = float3(0, 0, 0);
	
	float3 N = normalize(ps_inp.normal);
	float3 V = g_eye_p_in_world - ps_inp.world_p;
	float V_d = length(V);
	V /= V_d;

	float M_diffuse = 1.0f;
  	float M_specular = 1.0f;
  	float M_shininess = 64.0f;

	Light light = g_light;
	float shadow_multiplier = 0.0f;
	{
		float4 light_p = ps_inp.p_light_proj;
		light_p.xyz /= light_p.w;

		if (light_p.z > 1.0f)
		{
			shadow_multiplier = 1.0f;
		}
		else
		{
			float dimsx, dimsy;
			g_shadow_map.GetDimensions(dimsx, dimsy);
			float dx = 1.0f / dimsx;
			//float bias = max(0.0005 * (1.0 - dot(N, -light.dir)), 0.00005);
			float current_depth = light_p.z;
			float2 shadow_tex_p = float2(light_p.x * 0.5f + 0.5f, 1.0f - (light_p.y * 0.5f + 0.5f));
			if (g_light.type == LightType_PointLight)
			{
				float3 to_hit = ps_inp.world_p - g_light.p;
				float to_hit_len = length(to_hit);
				to_hit /= to_hit_len;
				float depth = g_shadow_cubemap.Sample(g_shadow_map_sampler, to_hit) * g_light.z_far;
				float bias = 0.05 * (to_hit_len * 0.075f) + max(0.05 * (1.0 - dot(N, -to_hit)), 0.005);
				shadow_multiplier = (to_hit_len - bias < depth) ? 1.0f : 0.2f;
			}
			else if (g_light.type == LightType_DirectionalLight)
			{
				float depth = g_shadow_map.Sample(g_shadow_map_sampler, shadow_tex_p).r;
				shadow_multiplier = (current_depth < depth) ? 1.0f : 0.2f;
			}
			else
			{
				shadow_multiplier = 1.0f;
			}
		}
	}

	switch (light.type)
	{
		case LightType_PointLight:
		{
			float3 L = g_light.p - ps_inp.world_p;
			float L_d = length(L);
			L /= L_d;

			float r0 = 3.0f;
			float rmax = g_light.z_far;
			float win = pow(max(1.0f - pow(L_d / rmax, 4.0f), 0.0f), 2.0f);
			float atten = win*(r0*r0 / (L_d*L_d + 0.1f));

			float3 R = normalize(reflect(-L, N));

			float R_dot_V = max(dot(R, V), 0.0f);
			float N_dot_L = max(dot(N, L), 0.0f);

			float3 diffuse = M_diffuse * g_light.colour.xyz * N_dot_L * ps_inp.colour.xyz;
			float3 specular = M_specular * g_light.colour.xyz * pow(R_dot_V, M_shininess) * ps_inp.colour.xyz;
			c_accum = saturate((specular + diffuse) * atten * shadow_multiplier + c_accum);
		} break;

		case LightType_DirectionalLight:
		{
			float3 L = normalize(-g_light.dir);
			float3 R = normalize(reflect(-L, N));

			float R_dot_V = max(dot(R, V), 0.0f);
			float N_dot_L = max(dot(N, L), 0.0f);

			float3 diffuse = M_diffuse * g_light.colour.xyz * N_dot_L * ps_inp.colour.xyz;
			float3 specular = M_specular * g_light.colour.xyz * pow(R_dot_V, M_shininess) * ps_inp.colour.xyz;
			c_accum = saturate((specular + diffuse) * shadow_multiplier + c_accum);
		} break;
	}
	
	
	float3 proj_p = ps_inp.ssao_p.xyz / ps_inp.ssao_p.w;
	proj_p.x = proj_p.x * 0.5f + 0.5f;
	proj_p.y = -proj_p.y * 0.5f + 0.5f;
	float M_ambient = g_ssao_map.Sample(g_point_sampler_clamp, proj_p.xy);
	float3 c_ambient = float3(0.05,0.05,0.05)*M_ambient;
	c_final = saturate(c_ambient * ps_inp.colour.xyz + c_accum);
	
	return float4(c_final, 1.0f);
	//return float4(M_ambient, M_ambient, M_ambient, 1);
	//return float4(ps_inp.uv, 0, 1);
}

//[maxvertexcount()]

float4 vs_directional_shadow(Vertex vertex, uint iid : SV_InstanceID) : SV_Position
{
	Model_Instance instance = g_model_instances[iid];
	
	float3x3 rot = create_rotation(instance.rotation);
	float4 world_coordinate = float4(mul(rot, vertex.v) * instance.scale + instance.p, 1.0f);
	float4 camera_coordinate = mul(g_world_to_camera, world_coordinate);
	return mul(g_proj, camera_coordinate);
}

struct VS_Omnidirectional_Output
{
	float4 p : SV_Position;
	float3 pos : WorldP;
};

VS_Omnidirectional_Output vs_omnidirectional_shadow(Vertex vertex, uint iid : SV_InstanceID)
{
	Model_Instance instance = g_model_instances[iid];

	float3x3 rot = create_rotation(instance.rotation);
	float4 world_coordinate = float4(mul(rot, vertex.v) * instance.scale + instance.p, 1.0f);
	float4 camera_coordinate = mul(g_world_to_camera, world_coordinate);

	VS_Omnidirectional_Output result;
	result.p = mul(g_proj, camera_coordinate);
	result.pos = world_coordinate.xyz;
	return(result);
}

float ps_omnidirectional_shadow(VS_Omnidirectional_Output ps_inp) : SV_Depth
{
	float dist = length(g_light.p - ps_inp.pos);
	return dist / g_light.z_far;
}