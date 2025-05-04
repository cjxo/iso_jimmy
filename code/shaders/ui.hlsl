cbuffer VShaderCBuffer0 : register(b0)
{
	row_major float4x4 proj;
};

struct UI_Rect
{
	float2 p;
	float2 dims;
	
	float4 vertex_colour;

	float2 uvs[4];
	uint tex_id;
};

StructuredBuffer<UI_Rect> g_instances : register(t0);
Texture2D<float4> g_texture1          : register(t1);

SamplerState g_sampler0 : register(s0);

static const float2 quad_vertices[] = 
{
	float2(0, 0),
	float2(0, 1),
	float2(1, 0),
	float2(1, 1),
};

struct VertexShader_Output
{
	float4 p : SV_Position;
	float4 vertex_colour : Colour_Mod;

	float2 uv : Tex_UV;
	uint tex_id : Tex_ID;
};

VertexShader_Output vs_main(uint vid : SV_VertexID, uint iid : SV_InstanceID)
{
	VertexShader_Output result = (VertexShader_Output)0;
	UI_Rect rect = g_instances[iid];
	
	float4 p = float4(quad_vertices[vid] * rect.dims + rect.p, 0.0f, 1.0f);
	result.p = mul(proj, p);

	result.vertex_colour = rect.vertex_colour;

	result.uv = rect.uvs[vid];
	result.tex_id = rect.tex_id;
	return(result);
}

float4 ps_main(VertexShader_Output ps_inp) : SV_Target
{
	float4 final_colour = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 rect_colour = ps_inp.vertex_colour;
	switch (ps_inp.tex_id)
	{
		case 1:
		{
			rect_colour *= g_texture1.Sample(g_sampler0, ps_inp.uv);
		} break;
	}
	
	final_colour = rect_colour;
	return(final_colour);
}
