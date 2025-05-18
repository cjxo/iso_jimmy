cbuffer VertexShader_Constants0 : register(b0)
{
	row_major float4x4 g_proj;
	row_major float4x4 g_world_to_camera;
	row_major float4x4 g_extra[2];
};

struct Vertex
{
	float3 v : IA_Vertex;
	float3 t : IA_Tangent;
	float3 b : IA_Bitangent;
	float3 n : IA_Normal;
	float2 uv : IA_UV;
};

struct VS_Output
{
	float4 p : SV_Position;
	float3 cam_p : CamP;
	float3 normal : Normal;
};

struct Model_Instance
{
	float3 p;
	float3 scale;
	float3 rotation;
	float4 colour;
};

// TODO: We need a deferred renderer
StructuredBuffer<Model_Instance> g_model_instances : register(t0);

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

VS_Output vs_forward_ssao_buffer_accum(Vertex vertex, uint iid : SV_InstanceID)
{
	VS_Output result;
	
	Model_Instance instance = g_model_instances[iid];
	float3x3 rot = create_rotation(instance.rotation);

	float4 world_coordinate = float4(mul(rot, vertex.v) * instance.scale + instance.p, 1.0f);
	float4 camera_coordinate = mul(g_world_to_camera, world_coordinate);
	result.p = mul(g_proj, camera_coordinate);
	result.cam_p = camera_coordinate.xyz;
	result.normal = mul(g_world_to_camera, float4(mul(rot, vertex.n), 0.0f)).xyz;

	return(result);
}

struct PS_Out
{
	float4 N : SV_Target0;
};

PS_Out ps_forward_ssao_buffer_accum(VS_Output ps_inp)
{
	PS_Out result;
	result.N = float4(normalize(ps_inp.normal), ps_inp.cam_p.z);
	return(result);
}

struct VS_Output_SSAO
{
	float4 p : SV_Position;
	float2 uv : TexCoord;
	float3 to_far_plane : ToFarPlane;
};

cbuffer VertexShader_Constants1 : register(b1)
{
	float4 g_far_plane_ptr[4];
	float4 g_samples_for_ssao[32];
};

VS_Output_SSAO vs_forward_ssao(uint vid : SV_VertexID)
{
	VS_Output_SSAO result = (VS_Output_SSAO)0;
	if (vid == 0)
	{
		result.p = float4(-1.0f, -1.0f, 0.0f, 1.0f);
		result.uv = float2(0, 0);
	}
	else if (vid == 1)
	{
		result.p = float4(-1.0f, +1.0f, 0.0f, 1.0f);
		result.uv = float2(0, 1);
	}
	else if (vid == 2)
	{
		result.p = float4(+1.0f, -1.0f, 0.0f, 1.0f);
		result.uv = float2(1, 0);
	}
	else
	{
		result.p = float4(+1.0f, +1.0f, 0.0f, 1.0f);
		result.uv = float2(1, 1);
	}

	result.to_far_plane = g_far_plane_ptr[vid].xyz;
	return result;
}

Texture2D<float4> g_normals_depths : register(t1);
Texture2D<float4> g_random_vec : register(t2);

SamplerState g_point_sampler_normal_depth : register(s0);
SamplerState g_point_sampler_random_vec : register(s1);

float ps_forward_ssao(VS_Output_SSAO ps_inp) : SV_Target0
{
	uint g_sample_count = 32;
	float g_occlusion_radius = 1.5f;
	float g_occlusion_fade_start = 0.2f;
	float g_occlusion_fade_end = 4.0f;
	float g_surface_epsilon = 0.085f;
	
	float2 uv = float2(ps_inp.uv.x, ps_inp.uv.y);
	// p -- the point we are computing the ambient occlusion for.
	// n -- normal vector at p
	// q -- a random offset from p.
	// r -- a potential occluder that might occlude p.
	float4 normal_depth = g_normals_depths.Sample(g_point_sampler_normal_depth, uv);

	float3 v = ps_inp.to_far_plane;
	float3 n = normal_depth.xyz;
	float p_z = normal_depth.w;

	float3 p = (p_z / v.z) * v;
	float3 rand_vec = g_random_vec.Sample(g_point_sampler_random_vec, float2(1280/4, 720/4)*uv).xyz;

	float3 normal = n;
	float3 tangent = normalize(rand_vec - normal * dot(normal, rand_vec));
	float3 bitangent = cross(normal, tangent);
	float3x3 tbn = float3x3(tangent, bitangent, normal);

	float occlusion_sum = 0.0f;
	for(uint i = 0; i < g_sample_count; ++i)
	{
		//float3 offset = g_samples_for_ssao[i].xyz;
		//float3 offset = reflect(g_samples_for_ssao[i].xyz, rand_vec);
		float3 offset = mul(tbn, g_samples_for_ssao[i].xyz);

		float flip = sign(dot(offset, n));

		float3 q = p + flip*g_occlusion_radius * offset;
		float4 proj_q = mul(g_proj, float4(q, 1.0f));
		proj_q.xyz /= proj_q.w;
		proj_q.x = proj_q.x * 0.5f + 0.5f;
		proj_q.y = (-proj_q.y * 0.5f + 0.5f);

		float r_z = g_normals_depths.Sample(g_point_sampler_normal_depth, proj_q.xy).a;
		float3 r = (r_z / q.z) * q;

		float dist_z = p.z - r.z;
		float dotp = max(dot(n, normalize(r - p)), 0.0f);

		float occlusion = 0.0f;
		if (dist_z > g_surface_epsilon)
		{
			float fade_length = g_occlusion_fade_end - g_occlusion_fade_start;
			occlusion = saturate((g_occlusion_fade_end - dist_z) / fade_length);
			//occlusion = smoothstep(0.0, 1.0, g_occlusion_radius / abs(p.z - r_z));
		}

		occlusion_sum += dotp * occlusion;
	}

	float result =  1.0f - (occlusion_sum / (float)g_sample_count);
	//return result;
	return saturate(pow(result, 6.0f));
}