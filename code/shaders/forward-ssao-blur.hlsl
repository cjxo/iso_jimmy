struct VS_Output_SSAO
{
	float4 p : SV_Position;
	float2 uv : TexCoord;
};

VS_Output_SSAO vs_main(uint vid : SV_VertexID)
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

	return result;
}

Texture2D<float> g_ssao_map : register(t0);
SamplerState g_sampler : register(s0);

float ps_main(VS_Output_SSAO ps_inp) : SV_Target0
{
	float2 texel_sz = 1.0f / float2(1280.0f, 720.0f);
	float result = 0.0f;
	for (int x = -2; x < 2; ++x)
	{
		for (int y = -2; y < 2; ++y) 
		{
			float2 offset = float2((float)x, (float)y) * texel_sz;
			result += g_ssao_map.Sample(g_sampler, ps_inp.uv + offset);
		}
	}

	result /= 16.0f;
	return(result);
}