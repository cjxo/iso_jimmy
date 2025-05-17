global_variable WCHAR *global_vshader_files[] =
{
  [R_VShaderType_Game] = L"..\\code\\shaders\\game.hlsl",
  [R_VShaderType_GameShadows] = L"..\\code\\shaders\\game.hlsl",
  [R_VShaderType_GameSSAO_Accum] = L"..\\code\\shaders\\forward-ssao.hlsl",
  [R_VShaderType_GameSSAO] = L"..\\code\\shaders\\forward-ssao.hlsl",
  [R_VShaderType_GameSSAO_Blur] = L"..\\code\\shaders\\forward-ssao-blur.hlsl",
  [R_VShaderType_UI] = L"..\\code\\shaders\\ui.hlsl",
};

global_variable char *global_vshader_entries[] =
{
  [R_VShaderType_Game] = "vs_main",
  [R_VShaderType_GameShadows] = "vs_shadow_only",
  [R_VShaderType_GameSSAO_Accum] = "vs_forward_ssao_buffer_accum",
  [R_VShaderType_GameSSAO] = "vs_forward_ssao",
  [R_VShaderType_GameSSAO_Blur] = "vs_main",
  [R_VShaderType_UI] = "vs_main",
};

global_variable WCHAR *global_pshader_files[] =
{
  [R_PShaderType_GameWithLight] = L"..\\code\\shaders\\game.hlsl",
  [R_PShaderType_GameWithoutLight] = L"..\\code\\shaders\\game.hlsl",
  [R_PShaderType_GameSSAO_Accum] = L"..\\code\\shaders\\forward-ssao.hlsl",
  [R_PShaderType_GameSSAO] = L"..\\code\\shaders\\forward-ssao.hlsl",
  [R_PShaderType_GameSSAO_Blur] = L"..\\code\\shaders\\forward-ssao-blur.hlsl",
  [R_PShaderType_UI] = L"..\\code\\shaders\\ui.hlsl",
};

global_variable char *global_pshader_entries[] =
{
  [R_PShaderType_GameWithLight] = "ps_main",
  [R_PShaderType_GameWithoutLight] = "ps_nolight",
  [R_PShaderType_GameSSAO_Accum] = "ps_forward_ssao_buffer_accum",
  [R_PShaderType_GameSSAO] = "ps_forward_ssao",
  [R_PShaderType_GameSSAO_Blur] = "ps_main",
  [R_PShaderType_UI] = "ps_main",
};

struct { UINT size; UINT count; } global_variable global_sbuffer_configs[] =
{
  [R_SBufferType_Game] = { sizeof(R_Model_Instance), R_MaxModelInstances },
  [R_SBufferType_UI] = { sizeof(R_UI_Rect), R_MaxUIRects }
};

global_variable UINT global_cbuffer_sizes[] =
{
  [R_CBufferType_Game_VShader0] = sizeof(DX11_VertexShader_Constants),
  [R_CBufferType_Game_PShader0] = sizeof(DX11_PixelShader_Constants),
  [R_CBufferType_GameSSAO1] = sizeof(DX11_SSAO_Constants1),
  [R_CBufferType_UI_VShader0] = sizeof(DX11_UI_VShader_Constants),
};

global_variable D3D11_INPUT_ELEMENT_DESC global_il_game[] =
{
  { "IA_Vertex", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "IA_Tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "IA_Bitangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "IA_Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
  { "IA_UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

struct { D3D11_INPUT_ELEMENT_DESC *desc; UINT count; } global_variable global_il_descs[] =
{
  [R_InputLayoutType_Game] = { global_il_game, ArrayCount(global_il_game) }
};

function void
dx11_create_device(R_State *renderer_state)
{
  HRESULT hr;
  UINT device_flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(ISO_DEBUG)
  device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D_FEATURE_LEVEL feature_levels = D3D_FEATURE_LEVEL_11_0;
  
  hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, device_flags, &feature_levels, 1,
                         D3D11_SDK_VERSION, &renderer_state->device, 0, &renderer_state->device_context);
  if (SUCCEEDED(hr))
  {
    hr = ID3D11DeviceContext_QueryInterface(renderer_state->device, &IID_ID3D11Device1, &renderer_state->device1);
    if (SUCCEEDED(hr))
    {
#if defined(ISO_DEBUG)
      // https://walbourn.github.io/dxgi-debug-device/
      ID3D11InfoQueue *dx11_info_queue;
      hr = ID3D11Device_QueryInterface(renderer_state->device, &IID_ID3D11InfoQueue, &dx11_info_queue);
      Assert(SUCCEEDED(hr));
      
      ID3D11InfoQueue_SetBreakOnSeverity(dx11_info_queue, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
      ID3D11InfoQueue_SetBreakOnSeverity(dx11_info_queue, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
      ID3D11InfoQueue_Release(dx11_info_queue);
      
      typedef HRESULT (WINAPI*T_DXGIGetDebugInterface)(REFIID, void **);
      HMODULE module = GetModuleHandle("Dxgidebug.dll");
      Assert(module != 0);
      T_DXGIGetDebugInterface fn = (T_DXGIGetDebugInterface)GetProcAddress(module, "DXGIGetDebugInterface");
      Assert(fn);
      
      IDXGIInfoQueue *dxgi_info_queue = 0;
      hr = fn(&IID_IDXGIInfoQueue, &dxgi_info_queue);
      Assert(SUCCEEDED(hr));
      
      IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info_queue, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
      IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info_queue, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
      IDXGIInfoQueue_Release(dxgi_info_queue);
      FreeLibrary(module);
#endif
    }
    else
    {
      Assert(!"TODO: Logging");
    }
  }
  else
  {
    Assert(!"TODO: Logging");
  }
}

function void
dx11_create_swap_chain(R_State *renderer_state, OS_Window window)
{
  HRESULT hr;
  IDXGIDevice2 *dxgi_device;
  IDXGIAdapter *dxgi_adapter;
  IDXGIFactory2 *dxgi_factory;
  
  hr = ID3D11Device_QueryInterface(renderer_state->device, &IID_IDXGIDevice2, &dxgi_device);
  if (SUCCEEDED(hr))
  {
    hr = IDXGIDevice2_GetAdapter(dxgi_device, &dxgi_adapter);
    if (SUCCEEDED(hr))
    {
      hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, &dxgi_factory);
      if (SUCCEEDED(hr))
      {
        DXGI_SWAP_CHAIN_DESC1 dxgi_swapchain_desc =
        {
          .Width = window.client_width,
          .Height = window.client_height,
          .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
          .Stereo = FALSE,
          .SampleDesc = { 1, 0 },
          .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
          .BufferCount = 2, 
          .Scaling = DXGI_SCALING_STRETCH,
          .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
          .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
          .Flags = 0,
        };
        hr = IDXGIFactory2_CreateSwapChainForHwnd(dxgi_factory, (IUnknown *)renderer_state->device, window.handle,
                                                  &dxgi_swapchain_desc, 0, 0, &renderer_state->swap_chain);
        if (SUCCEEDED(hr))
        {
          IDXGIFactory2_MakeWindowAssociation(dxgi_factory, window.handle, DXGI_MWA_NO_ALT_ENTER);
          
          ID3D11Texture2D *back_buffer = 0;
          hr = IDXGISwapChain1_GetBuffer(renderer_state->swap_chain, 0, &IID_ID3D11Texture2D, &back_buffer);
          if (SUCCEEDED(hr))
          {
            D3D11_RENDER_TARGET_VIEW_DESC rtv_desc =
            {
              .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
              .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D,
              .Texture2D = { 0 }
            };
            hr = ID3D11Device_CreateRenderTargetView(renderer_state->device, (ID3D11Resource *)back_buffer,
                                                     &rtv_desc, &renderer_state->back_buffer_rtv);
            
            if (SUCCEEDED(hr))
            {
              renderer_state->back_buffer_width = window.client_width;
              renderer_state->back_buffer_height = window.client_height;
            }
            else
            {
              Assert(!"TODO: Logging");
            }
            ID3D11Texture2D_Release(back_buffer);
          }
          else
          {
            Assert(!"TODO: Logging");
          }
        }
        else
        {
          Assert(!"TODO: Logging");
        }
        
        IDXGIFactory2_Release(dxgi_factory);
      }
      
      IDXGIAdapter_Release(dxgi_adapter);
    }
    else
    {
      Assert(!"TODO: Logging");
    }
    
    IDXGIDevice2_Release(dxgi_device);
  }
  else
  {
    Assert(!"TODO: Logging");
  }
}

function void
dx11_create_rasterizer_states(R_State *state)
{
  D3D11_RASTERIZER_DESC desc;
  desc.FillMode = D3D11_FILL_SOLID;
  desc.CullMode = D3D11_CULL_BACK;
  desc.FrontCounterClockwise = TRUE;
  desc.DepthBias = 0;
  desc.DepthBiasClamp = 0.0f;
  desc.SlopeScaledDepthBias = 0.0f;
  desc.DepthClipEnable = TRUE;
  desc.ScissorEnable = FALSE;
  desc.MultisampleEnable = FALSE;
  desc.AntialiasedLineEnable = FALSE;
  HRESULT hr = ID3D11Device_CreateRasterizerState(state->device, &desc, &state->raster_fill_cull_ccw);
  if (SUCCEEDED(hr))
  {
    desc.FillMode = D3D11_FILL_WIREFRAME;
    hr = ID3D11Device_CreateRasterizerState(state->device, &desc, &state->raster_wire_cull_ccw);
    if (SUCCEEDED(hr))
    {
      desc.FillMode = D3D11_FILL_SOLID;
      desc.CullMode = D3D11_CULL_NONE;
      hr = ID3D11Device_CreateRasterizerState(state->device, &desc, &state->raster_fill_nocull_ccw);
      if (SUCCEEDED(hr))
      {
      }
      else
      {
        Assert(!"TODO: Logging");
      }
    }
    else
    {
      Assert(!"TODO: Logging");
    }
  }
  else
  {
    Assert(!"TODO: Logging");
  }
}

function void
dx11_create_depth_stencil_states(R_State *state)
{
  ID3D11Texture2D *tex = 0;
  D3D11_TEXTURE2D_DESC tex_desc;
  D3D11_DEPTH_STENCIL_DESC dsv_desc = {0};
  HRESULT hr;
  
  tex_desc.Width = state->back_buffer_width;
  tex_desc.Height = state->back_buffer_height;
  tex_desc.MipLevels = 1;
  tex_desc.ArraySize = 1;
  tex_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  tex_desc.SampleDesc.Count = 1;
  tex_desc.SampleDesc.Quality = 0;
  tex_desc.Usage = D3D11_USAGE_DEFAULT;
  tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  tex_desc.CPUAccessFlags = 0;
  tex_desc.MiscFlags = 0;
  hr = ID3D11Device_CreateTexture2D(state->device, &tex_desc, 0, &tex);
  
  if (SUCCEEDED(hr))
  {
    hr = ID3D11Device_CreateDepthStencilView(state->device, (ID3D11Resource *)tex, 0, &state->main_dsv);
    if (SUCCEEDED(hr))
    {
      dsv_desc.DepthEnable = TRUE;
      dsv_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
      dsv_desc.DepthFunc = D3D11_COMPARISON_LESS;
      hr = ID3D11Device_CreateDepthStencilState(state->device, &dsv_desc, &state->ds_depth_only);
      if (SUCCEEDED(hr))
      {
      }
      else
      {
        Assert(!"TODO: Logging");
      }
    }
    else
    {
      Assert(!"TODO: Logging");
    }
    
    ID3D11Device_Release(tex);
  }
  else
  {
    Assert(!"TODO: Logging");
  }
}

function void
dx11_create_blend_states(R_State *state)
{
  D3D11_BLEND_DESC1 desc = {0};
  HRESULT hr;
  
  desc.IndependentBlendEnable = FALSE;
  desc.RenderTarget->BlendEnable = TRUE;
  desc.RenderTarget->LogicOpEnable = FALSE;
  desc.RenderTarget->SrcBlend = D3D11_BLEND_SRC_ALPHA;
  desc.RenderTarget->DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  desc.RenderTarget->BlendOp = D3D11_BLEND_OP_ADD;
  desc.RenderTarget->SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
  desc.RenderTarget->DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
  desc.RenderTarget->BlendOpAlpha = D3D11_BLEND_OP_ADD;
  desc.RenderTarget->LogicOp = D3D11_LOGIC_OP_NOOP;
  desc.RenderTarget->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  hr = ID3D11Device1_CreateBlendState1(state->device1, &desc, &state->blend_state_transparency);
  if (SUCCEEDED(hr))
  {
  }
  else
  {
    Assert(!"TODO: Logging");
  }
}

function void
dx11_create_sampler_states(R_State *state)
{
  D3D11_SAMPLER_DESC samdesc;
  
  samdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  samdesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  samdesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  samdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  samdesc.MipLODBias = 0;
  samdesc.MaxAnisotropy = 1;
  samdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  samdesc.BorderColor[0] = 0.0f;
  samdesc.BorderColor[1] = 0.0f;
  samdesc.BorderColor[2] = 0.0f;
  samdesc.BorderColor[3] = 1e5f;
  samdesc.MinLOD = 0;
  samdesc.MaxLOD = D3D11_FLOAT32_MAX;
  
  samdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  samdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  samdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  
  ID3D11Device_CreateSamplerState(state->device, &samdesc, &state->point_sampler_clamp);
  ID3D11Device_CreateSamplerState(state->device, &samdesc, &state->point_sampler_wrap);
  
  samdesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
  samdesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  samdesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
  ID3D11Device_CreateSamplerState(state->device, &samdesc, &state->point_sampler_border_black);
}

#if ISO_DEBUG
# define DX11_ShaderFlags (D3DCOMPILE_SKIP_OPTIMIZATION|D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS)
#else
# define DX11_ShaderFlags (D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_ENABLE_STRICTNESS)
#endif
function void
dx11_create_shaders_with_input_layout(ID3D11Device *device, LPCWSTR file, char *vshader_entry, char *pshader_entry,
                                      D3D11_INPUT_ELEMENT_DESC *input_elements, u32 input_elements_count,
                                      ID3D11VertexShader **vshader_out, ID3D11PixelShader **pshader_out, ID3D11InputLayout **il_out)
{
  ID3D10Blob *bytecode, *error;
  HRESULT hr;
  
  hr = D3DCompileFromFile(file, null, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                          vshader_entry, "vs_5_0", DX11_ShaderFlags, 0, &bytecode, &error);
  if (SUCCEEDED(hr))
  {
    hr = ID3D11Device_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(bytecode), 
                                         ID3D10Blob_GetBufferSize(bytecode), 0, vshader_out);
    
    
    if (SUCCEEDED(hr))
    {
      hr = ID3D11Device_CreateInputLayout(device, input_elements, input_elements_count,
                                          ID3D10Blob_GetBufferPointer(bytecode), 
                                          ID3D10Blob_GetBufferSize(bytecode), il_out);
      ID3D10Blob_Release(bytecode);
      if (SUCCEEDED(hr))
      {
        hr = D3DCompileFromFile(file, null, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                pshader_entry, "ps_5_0", DX11_ShaderFlags, 0, &bytecode, &error);
        
        
        if (SUCCEEDED(hr))
        {
          hr = ID3D11Device_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(bytecode), 
                                              ID3D10Blob_GetBufferSize(bytecode), 0, pshader_out);
          
          ID3D10Blob_Release(bytecode);
        }
        else
        {
          OutputDebugStringA(ID3D10Blob_GetBufferPointer(error));
          Assert(!"TODO: Logging");
        }
      }
      else
      {
        Assert(!"TODO: Logging");
      }
    }
    else
    {
      Assert(!"TODO: Logging");
    }
  }
  else
  {
    OutputDebugStringA(ID3D10Blob_GetBufferPointer(error));
    Assert(!"TODO: Logging");
  }
}

function R_Model
dx11_create_model(ID3D11Device *device,
                  f32 *vertices, u32 vertices_size, u32 vertex_struct_size,
                  u32 *indices, u32 indices_count)
{
  R_Model result = {0};
  HRESULT hr;
  D3D11_BUFFER_DESC buffer_desc={0};
  D3D11_SUBRESOURCE_DATA subrec_data={0};
  
  buffer_desc.ByteWidth = vertices_size;
  buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
  buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  buffer_desc.CPUAccessFlags = 0;
  buffer_desc.MiscFlags = 0;
  buffer_desc.StructureByteStride = 0;
  
  subrec_data.pSysMem = vertices;
  subrec_data.SysMemPitch = vertex_struct_size;
  subrec_data.SysMemSlicePitch = 0;
  hr = ID3D11Device_CreateBuffer(device, &buffer_desc, &subrec_data, &result.vertices);
  
  if (SUCCEEDED(hr))
  {
    buffer_desc.ByteWidth = indices_count * sizeof(u32);
    buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
    buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;
    
    subrec_data.pSysMem = indices;
    subrec_data.SysMemPitch = sizeof(u32);
    subrec_data.SysMemSlicePitch = 0;
    
    hr = ID3D11Device_CreateBuffer(device, &buffer_desc, &subrec_data, &result.indices);
    if (SUCCEEDED(hr))
    {
      result.indices_count = indices_count;
      result.struct_size = vertex_struct_size;
    }
    else
    {
      Assert(!"TODO: Logging");
    }
  }
  else
  {
    Assert(!"TODO: Logging");
  }
  
  return(result);
}

function void
dx11_create_general_rendering_states(R_State *state)
{
  ID3D10Blob *bytecode, *error;
  HRESULT hr;
  D3D11_BUFFER_DESC buffer_desc;
  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
  
  for (R_VShaderType shader_type = R_VShaderType_Game;
       shader_type < R_VShaderType_Count;
       ++shader_type)
  {
    hr = D3DCompileFromFile(global_vshader_files[shader_type], null, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            global_vshader_entries[shader_type], "vs_5_0", DX11_ShaderFlags,
                            0, &bytecode, &error);
    if (SUCCEEDED(hr))
    {
      hr = ID3D11Device_CreateVertexShader(state->device, ID3D10Blob_GetBufferPointer(bytecode), 
                                           ID3D10Blob_GetBufferSize(bytecode), 0,
                                           state->vshaders + shader_type);
      
      
      if (SUCCEEDED(hr))
      {
        if (shader_type == R_VShaderType_Game)
        {
          hr = ID3D11Device_CreateInputLayout(state->device,
                                              global_il_descs[R_InputLayoutType_Game].desc,
                                              global_il_descs[R_InputLayoutType_Game].count,
                                              ID3D10Blob_GetBufferPointer(bytecode), 
                                              ID3D10Blob_GetBufferSize(bytecode),
                                              state->input_layouts + R_InputLayoutType_Game);
          if (!SUCCEEDED(hr))
          {
            Assert(!"TODO: LOGGING");
          }
        }
      }
      else
      {
        Assert(!"TODO: LOGGING");
      }
      
      ID3D10Blob_Release(bytecode);
    }
    else
    {
      OutputDebugStringA(ID3D10Blob_GetBufferPointer(error));
      Assert(!"TODO: LOGGING");
    }
  }
  
  for (R_PShaderType shader_type = R_PShaderType_GameWithLight;
       shader_type < R_PShaderType_Count;
       ++shader_type)
  {
    hr = D3DCompileFromFile(global_pshader_files[shader_type], null, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            global_pshader_entries[shader_type], "ps_5_0", DX11_ShaderFlags,
                            0, &bytecode, &error);
    if (SUCCEEDED(hr))
    {
      hr = ID3D11Device_CreatePixelShader(state->device, ID3D10Blob_GetBufferPointer(bytecode), 
                                          ID3D10Blob_GetBufferSize(bytecode), 0,
                                          state->pshaders + shader_type);
      if (!SUCCEEDED(hr))
      {
        Assert(!"TODO: LOGGING");
      }
      
      ID3D10Blob_Release(bytecode);
    }
    else
    {
      OutputDebugStringA(ID3D10Blob_GetBufferPointer(error));
      Assert(!"TODO: LOGGING");
    }
  }
  
  for (R_SBufferType sbuffer_type = R_SBufferType_Game;
       sbuffer_type < R_SBufferType_Count;
       ++sbuffer_type)
  {
    UINT size = global_sbuffer_configs[sbuffer_type].size;
    UINT count = global_sbuffer_configs[sbuffer_type].count;
    
    buffer_desc.ByteWidth = size*count;
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.StructureByteStride = size;
    
    hr = ID3D11Device_CreateBuffer(state->device, &buffer_desc, 0, state->sbuffers + sbuffer_type);
    if (SUCCEEDED(hr))
    {
      srv_desc.Format = DXGI_FORMAT_UNKNOWN;
      srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
      srv_desc.Buffer.ElementOffset = 0;
      srv_desc.Buffer.NumElements = count;
      hr = ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)(state->sbuffers[sbuffer_type]), &srv_desc, state->sbuffer_srvs + sbuffer_type);
      if (!SUCCEEDED(hr))
      {
        Assert(!"TODO: LOGGING");
      }
    }
    else
    {
      Assert(!"TODO: LOGGING");
    }
  }
  
  for (R_CBufferType cbuffer_type = R_CBufferType_Game_VShader0;
       cbuffer_type < R_CBufferType_Count;
       ++cbuffer_type)
  {
    buffer_desc.ByteWidth = (global_cbuffer_sizes[cbuffer_type]+15)&(~15);
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags = 0;
    buffer_desc.StructureByteStride = 0;
    
    hr = ID3D11Device_CreateBuffer(state->device, &buffer_desc, 0, state->cbuffers + cbuffer_type);
    if (!SUCCEEDED(hr))
    {
      Assert(!"TODO: LOGGING");
    }
  }
}

function void
dx11_create_game_state(R_State *state)
{
  // p <-> tangent <-> bitangent <-> normal <-> uv
  f32 cube_vbuffer[] =
  {
    // front face
    -0.5f, +0.5f, -0.5f,      +1.0f, +0.0f, +0.0f,    +0.0f, 1.0f, 0.0f,     +0.0f, +0.0f, -1.0f,         0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,      +1.0f, +0.0f, +0.0f,    +0.0f, 1.0f, 0.0f,     +0.0f, +0.0f, -1.0f,         0.0f, 1.0f,
    +0.5f, -0.5f, -0.5f,      +1.0f, +0.0f, +0.0f,    +0.0f, 1.0f, 0.0f,     +0.0f, +0.0f, -1.0f,         1.0f, 1.0f,
    +0.5f, +0.5f, -0.5f,      +1.0f, +0.0f, +0.0f,    +0.0f, 1.0f, 0.0f,     +0.0f, +0.0f, -1.0f,         1.0f, 0.0f,
    
    // right face
    +0.5f, +0.5f, -0.5f,     +0.0f, +0.0f, +1.0f,     +0.0f, +1.0f, 0.0f,    +1.0f, +0.0f, +0.0f,     0.0f, 0.0f,
    +0.5f, -0.5f, -0.5f,     +0.0f, +0.0f, +1.0f,     +0.0f, +1.0f, 0.0f,    +1.0f, +0.0f, +0.0f,     0.0f, 1.0f,
    +0.5f, -0.5f, +0.5f,     +0.0f, +0.0f, +1.0f,     +0.0f, +1.0f, 0.0f,    +1.0f, +0.0f, +0.0f,     1.0f, 1.0f,
    +0.5f, +0.5f, +0.5f,     +0.0f, +0.0f, +1.0f,     +0.0f, +1.0f, 0.0f,    +1.0f, +0.0f, +0.0f,     1.0f, 0.0f,
    
    // back face
    +0.5f, +0.5f, +0.5f,     -1.0f, +0.0f, +0.0f,     +0.0f, +1.0f, +0.0f,   +0.0f, +0.0f, +1.0f,     0.0f, 0.0f,
    +0.5f, -0.5f, +0.5f,     -1.0f, +0.0f, +0.0f,     +0.0f, +1.0f, +0.0f,   +0.0f, +0.0f, +1.0f,     0.0f, 1.0f,
    -0.5f, -0.5f, +0.5f,     -1.0f, +0.0f, +0.0f,     +0.0f, +1.0f, +0.0f,   +0.0f, +0.0f, +1.0f,     1.0f, 1.0f,
    -0.5f, +0.5f, +0.5f,     -1.0f, +0.0f, +0.0f,     +0.0f, +1.0f, +0.0f,   +0.0f, +0.0f, +1.0f,     1.0f, 0.0f,
    
    // left face
    -0.5f, +0.5f, +0.5f,     +0.0f, +0.0f, -1.0f,     +0.0f, +1.0f, +0.0f,   -1.0f, +0.0f, +0.0f,     0.0f, 0.0f,
    -0.5f, -0.5f, +0.5f,     +0.0f, +0.0f, -1.0f,     +0.0f, +1.0f, +0.0f,   -1.0f, +0.0f, +0.0f,     0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,     +0.0f, +0.0f, -1.0f,     +0.0f, +1.0f, +0.0f,   -1.0f, +0.0f, +0.0f,     1.0f, 1.0f,
    -0.5f, +0.5f, -0.5f,     +0.0f, +0.0f, -1.0f,     +0.0f, +1.0f, +0.0f,   -1.0f, +0.0f, +0.0f,     1.0f, 0.0f,
    
    // top face
    -0.5f, +0.5f, +0.5f,     +1.0f, +0.0f, +0.0f,     +0.0f, +0.0f, +1.0f,   +0.0f, +1.0f, +0.0f,     0.0f, 0.0f,
    -0.5f, +0.5f, -0.5f,     +1.0f, +0.0f, +0.0f,     +0.0f, +0.0f, +1.0f,   +0.0f, +1.0f, +0.0f,     0.0f, 1.0f,
    +0.5f, +0.5f, -0.5f,     +1.0f, +0.0f, +0.0f,     +0.0f, +0.0f, +1.0f,   +0.0f, +1.0f, +0.0f,     1.0f, 1.0f,
    +0.5f, +0.5f, +0.5f,     +1.0f, +0.0f, +0.0f,     +0.0f, +0.0f, +1.0f,   +0.0f, +1.0f, +0.0f,     1.0f, 0.0f,
    
    // bottom face
    -0.5f, -0.5f, -0.5f,     +1.0f, +0.0f, +0.0f,     +0.0f, +0.0f, -1.0f,   +0.0f, -1.0f, +0.0f,     0.0f, 0.0f,
    -0.5f, -0.5f, +0.5f,     +1.0f, +0.0f, +0.0f,     +0.0f, +0.0f, -1.0f,   +0.0f, -1.0f, +0.0f,     0.0f, 1.0f,
    +0.5f, -0.5f, +0.5f,     +1.0f, +0.0f, +0.0f,     +0.0f, +0.0f, -1.0f,   +0.0f, -1.0f, +0.0f,     1.0f, 1.0f,
    +0.5f, -0.5f, -0.5f,     +1.0f, +0.0f, +0.0f,     +0.0f, +0.0f, -1.0f,   +0.0f, -1.0f, +0.0f,     1.0f, 0.0f,
  };
  
  u32 cube_ibuffer[] = {
    0, 1, 2,
    2, 3, 0,
    
    4, 5, 6,
    6, 7, 4,
    
    8, 9, 10,
    10, 11, 8,
    
    12, 13, 14,
    14, 15, 12,
    
    16, 17, 18,
    18, 19, 16,
    
    20, 21, 22,
    22, 23, 20,
  };
  
  state->cube_model = dx11_create_model(state->device,
                                        cube_vbuffer, sizeof(cube_vbuffer), sizeof(R_Game_Vertex),
                                        cube_ibuffer, ArrayCount(cube_ibuffer));
}

function void
dx11_create_font_atlas(R_State *state)
{
  s32 point_size = 18;
  state->font_handle = CreateFontA(-(point_size * 96)/72, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                                   DEFAULT_PITCH | FF_DONTCARE, "Arial");
  
  if (state->font_handle)
  {
#define MaxBitmapSize 512
    s32 bitmap_width = MaxBitmapSize;
    s32 bitmap_height = MaxBitmapSize;
    
    HDC dc = CreateCompatibleDC(0);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, bitmap_width, bitmap_height);
    
    RECT r;
    r.top = 0;
    r.left = 0;
    r.right = bitmap_width;
    r.bottom = bitmap_height;
    SelectObject(dc, bitmap);
    FillRect(dc, &r, GetStockObject(BLACK_BRUSH));
    SelectObject(dc, state->font_handle);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255,255,255));
    
    TEXTMETRIC text_metrics;
    GetTextMetrics(dc, &text_metrics);
    
    state->font.ascent = (f32)text_metrics.tmAscent;
    state->font.descent = (f32)text_metrics.tmDescent;
    
    s32 gap = 4;
    s32 pen_x = gap;
    s32 pen_y = gap;
    SetTextAlign(dc, TA_TOP|TA_LEFT);
    for (u8 codepoint = 32; codepoint < 128; ++codepoint)
    {
      SIZE glyph_dims;
      char c = (char)codepoint;
      GetTextExtentPoint32A(dc, &c, 1, &glyph_dims);
      glyph_dims.cx = glyph_dims.cx;
      
      if ((pen_x + glyph_dims.cx + gap) >= bitmap_width)
      {
        pen_x = gap;
        pen_y += text_metrics.tmHeight + gap;
      }
      
      // https://learn.microsoft.com/en-us/windows/win32/gdi/character-widths
      ABCFLOAT abc;
      GetCharABCWidthsFloatA(dc, codepoint, codepoint, &abc);
      f32 advance_x = (abc.abcfA + abc.abcfB + abc.abcfC);
      
      TextOut(dc, pen_x, pen_y, &c, 1);
      
      state->font.glyphs[codepoint].advance = advance_x;
      state->font.glyphs[codepoint].clip_x = (f32)pen_x;
      state->font.glyphs[codepoint].clip_y = (f32)pen_y;
      state->font.glyphs[codepoint].clip_width = (f32)(glyph_dims.cx);
      state->font.glyphs[codepoint].clip_height = (f32)(glyph_dims.cy);
      pen_x += glyph_dims.cx + gap;
    }
    
    BITMAPINFO bitmap_info = {0};
    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = bitmap_width;
    bitmap_info.bmiHeader.biHeight = -bitmap_height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    
    M_Temp temp_mem = m_temp_begin(m_get_for_transient_purposes(0,0));
    u64 bitmap_area = bitmap_width*bitmap_height*4;
    u8 *source_buffer = m_arena_push(temp_mem.arena, bitmap_area);
    u8 *dest_buffer = m_arena_push(temp_mem.arena, bitmap_area);
    GetDIBits(dc, bitmap, 0, bitmap_height, source_buffer, &bitmap_info, DIB_RGB_COLORS);
    
    u8 *src = source_buffer;
    u8 *dest = dest_buffer;
    for (s32 y_pix = 0; y_pix < bitmap_height; ++y_pix)
    {
      u8 *dest_row = dest;
      u8 *src_row = src;
      for (s32 x_pix = 0; x_pix < bitmap_width; ++x_pix)
      {
        *dest_row++ = src_row[0];
        *dest_row++ = src_row[1];
        *dest_row++ = src_row[2];
        *dest_row++ = src_row[0];
        src_row += 4;
      }
      
      dest += bitmap_width * 4;
      src += bitmap_width * 4;
    }
    
    D3D11_TEXTURE2D_DESC atlas_desc =
    {
      .Width = bitmap_width,
      .Height = bitmap_height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .SampleDesc = { 1, 0 },
      .Usage = D3D11_USAGE_IMMUTABLE,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = 0,
      .MiscFlags = 0,
    };
    
    D3D11_SUBRESOURCE_DATA atlas_subrec =
    {
      .pSysMem = dest_buffer,
      .SysMemPitch = bitmap_width*4,
    };
    
    ID3D11Texture2D *atlas_font_tex;
    if (SUCCEEDED(ID3D11Device_CreateTexture2D(state->device, &atlas_desc, &atlas_subrec, &atlas_font_tex)))
    {
      ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)atlas_font_tex, 0, &state->font.srv);
      state->font.sheet_width = bitmap_width;
      state->font.sheet_height = bitmap_height;
      ID3D11Texture2D_Release(atlas_font_tex);
    }
    else
    {
      Assert(!"Log Soon");
    }
    
    m_temp_end(temp_mem);
    DeleteObject(bitmap);
    DeleteDC(dc);
  }
  else
  {
    Assert(!"Log Soon");
  }
}

function void
dx11_create_shadowmap(R_State *state)
{
  f32 dims = 2048;
  D3D11_VIEWPORT vp;
  D3D11_TEXTURE2D_DESC tdesc;
  D3D11_DEPTH_STENCIL_VIEW_DESC dsvdesc;
  D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
  D3D11_SAMPLER_DESC samdesc;
  D3D11_RASTERIZER_DESC rasterdesc;
  ID3D11Texture2D *tex;
  HRESULT hr;
  
  vp.TopLeftX = 0.0f;
  vp.TopLeftY = 0.0f;
  vp.Width = dims;
  vp.Height = dims;
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  
  tdesc.Width = (s32)dims;
  tdesc.Height = (s32)dims;
  tdesc.MipLevels = 1;
  tdesc.ArraySize = 1;
  tdesc.Format = DXGI_FORMAT_R32_TYPELESS;
  tdesc.SampleDesc.Count = 1;
  tdesc.SampleDesc.Quality = 0;
  tdesc.Usage = D3D11_USAGE_DEFAULT;
  tdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
  tdesc.CPUAccessFlags = 0;
  tdesc.MiscFlags = 0;
  
  dsvdesc.Format = DXGI_FORMAT_D32_FLOAT;
  dsvdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  dsvdesc.Flags = 0;
  dsvdesc.Texture2D.MipSlice = 0;
  
  srvdesc.Format = DXGI_FORMAT_R32_FLOAT;
  srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvdesc.Texture2D.MostDetailedMip = 0;
  srvdesc.Texture2D.MipLevels = 1;
  
  samdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  samdesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
  samdesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
  samdesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
  samdesc.MipLODBias = 0;
  samdesc.MaxAnisotropy = 1;
  samdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  samdesc.BorderColor[0] = 1.0f;
  samdesc.BorderColor[1] = 1.0f;
  samdesc.BorderColor[2] = 1.0f;
  samdesc.BorderColor[3] = 1.0f;
  samdesc.MinLOD = 0;
  samdesc.MaxLOD = D3D11_FLOAT32_MAX;
  
  hr = ID3D11Device_CreateTexture2D(state->device, &tdesc, 0, &tex);
  if (SUCCEEDED(hr))
  {
    hr = ID3D11Device_CreateDepthStencilView(state->device, (ID3D11Resource *)tex,
                                             &dsvdesc,  &state->shadow_map_dsv);
    if (SUCCEEDED(hr))
    {
      hr = ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)tex,
                                                 &srvdesc, &state->shadow_map_srv);
      if (SUCCEEDED(hr))
      {
        hr = ID3D11Device_CreateSamplerState(state->device, &samdesc, &state->shadow_map_sampler);
        if (SUCCEEDED(hr))
        {
          rasterdesc.FillMode = D3D11_FILL_SOLID;
          rasterdesc.CullMode = D3D11_CULL_BACK;
          rasterdesc.FrontCounterClockwise = TRUE;
          rasterdesc.DepthBias = 10000;
          rasterdesc.DepthBiasClamp = 0.0f;
          rasterdesc.SlopeScaledDepthBias = 1.0f;
          rasterdesc.DepthClipEnable = TRUE;
          rasterdesc.ScissorEnable = FALSE;
          rasterdesc.MultisampleEnable = FALSE;
          rasterdesc.AntialiasedLineEnable = FALSE;
          hr = ID3D11Device_CreateRasterizerState(state->device, &rasterdesc, &state->shadow_map_raster);
          if (SUCCEEDED(hr))
          {
          }
          else
          {
            Assert(!"TODO: Logging");
          }
        }
        else
        {
          Assert(!"TODO: Logging");
        }
      }
      else
      {
        Assert(!"TODO: Logging");
      }
    }
    else
    {
      Assert(!"TODO: Logging");
    }
    
    ID3D11Texture2D_Release(tex);
  }
  else
  {
    Assert(!"TODO: Logging");
  }
  
  state->shadow_map_vp = vp;
}

function void
dx11_create_gbuffer(R_State *state)
{
  ID3D11Texture2D *tex;
  D3D11_TEXTURE2D_DESC tex_desc;
  D3D11_SUBRESOURCE_DATA subrec_data;
  //D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
  //D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
  HRESULT hr;
  
  tex_desc.Width = state->back_buffer_width;
  tex_desc.Height = state->back_buffer_height;
  tex_desc.MipLevels = 1;
  tex_desc.ArraySize = 1;
  tex_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
  tex_desc.SampleDesc.Count = 1;
  tex_desc.SampleDesc.Quality = 0;
  tex_desc.Usage = D3D11_USAGE_DEFAULT;
  tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
  tex_desc.CPUAccessFlags = 0;
  tex_desc.MiscFlags = 0;
  hr = ID3D11Device_CreateTexture2D(state->device, &tex_desc, null, &tex);
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  hr = ID3D11Device_CreateRenderTargetView(state->device, (ID3D11Resource *)tex,
                                           null,
                                           &state->scene_normals_rtv);
  
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  hr = ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)tex,
                                             null,
                                             &state->scene_normals_srv);
  
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  ID3D11Texture2D_Release(tex);
  
#if 0
  tex_desc.Width = state->back_buffer_width;
  tex_desc.Height = state->back_buffer_height;
  tex_desc.MipLevels = 1;
  tex_desc.ArraySize = 1;
  tex_desc.Format = DXGI_FORMAT_R32_TYPELESS;
  tex_desc.SampleDesc.Count = 1;
  tex_desc.SampleDesc.Quality = 0;
  tex_desc.Usage = D3D11_USAGE_DEFAULT;
  tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
  tex_desc.CPUAccessFlags = 0;
  tex_desc.MiscFlags = 0;
  
  hr = ID3D11Device_CreateTexture2D(state->device, &tex_desc, null, &tex);
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
  dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  dsv_desc.Flags = 0;
  dsv_desc.Texture2D.MipSlice = 0;
  hr = ID3D11Device_CreateDepthStencilView(state->device, (ID3D11Resource *)tex,
                                           &dsv_desc, &state->scene_depth_dsv);
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
  srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.MipLevels = 1;
  hr = ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)tex,
                                             &srv_desc,
                                             &state->scene_depth_srv);
  
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  ID3D11Texture2D_Release(tex);
#endif
  
  tex_desc.Width = state->back_buffer_width;
  tex_desc.Height = state->back_buffer_height;
  tex_desc.MipLevels = 1;
  tex_desc.ArraySize = 1;
  tex_desc.Format = DXGI_FORMAT_R32_FLOAT;
  tex_desc.SampleDesc.Count = 1;
  tex_desc.SampleDesc.Quality = 0;
  tex_desc.Usage = D3D11_USAGE_DEFAULT;
  tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
  tex_desc.CPUAccessFlags = 0;
  tex_desc.MiscFlags = 0;
  
  hr = ID3D11Device_CreateTexture2D(state->device, &tex_desc, null, &tex);
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  hr = ID3D11Device_CreateRenderTargetView(state->device, (ID3D11Resource *)tex,
                                           null,
                                           &state->scene_ssao_rtv);
  
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  hr = ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)tex,
                                             null,
                                             &state->scene_ssao_srv);
  
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  ID3D11Texture2D_Release(tex);

  hr = ID3D11Device_CreateTexture2D(state->device, &tex_desc, null, &tex);

  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }

  hr = ID3D11Device_CreateRenderTargetView(state->device, (ID3D11Resource *)tex,
                                           null,
                                           &state->scene_ssao_blur_rtv);
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }

  hr = ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)tex,
                                             null,
                                             &state->scene_ssao_blur_srv);
  
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  ID3D11Texture2D_Release(tex);

  PRNG32 rng;
  prng32_seed(&rng, 37);
  
  v4f ssao_noise[16];
  for (u32 i = 0; i < 16; ++i)
  {
    ssao_noise[i] = v4f_make(prng32_nextf32(&rng) * 2.0f - 1.0f,
                             prng32_nextf32(&rng) * 2.0f - 1.0f,
                             0.0f, 0.0f);
  }
  subrec_data.pSysMem = ssao_noise;
  subrec_data.SysMemPitch = sizeof(v4f);
  subrec_data.SysMemSlicePitch = 0;
  
  tex_desc.Width = 4;
  tex_desc.Height = 4;
  tex_desc.MipLevels = 1;
  tex_desc.ArraySize = 1;
  tex_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  tex_desc.SampleDesc.Count = 1;
  tex_desc.SampleDesc.Quality = 0;
  tex_desc.Usage = D3D11_USAGE_IMMUTABLE;
  tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  tex_desc.CPUAccessFlags = 0;
  tex_desc.MiscFlags = 0;
  
  hr = ID3D11Device_CreateTexture2D(state->device, &tex_desc, &subrec_data, &tex);
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  hr = ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)tex,
                                             null,
                                             &state->scene_random_vec);
  
  if (!SUCCEEDED(hr))
  {
    Assert(!"TODO: Logging");
  }
  
  ID3D11Texture2D_Release(tex);
  
  v4f *offsets = state->ssao_offsets;
  for (u64 i = 0; i < R_SSAO_SampleCount; ++i)
  {
    offsets[i] = v4f_make(prng32_nextf32(&rng) * 2.0f - 1.0f,
                          prng32_nextf32(&rng) * 2.0f - 1.0f,
                          prng32_nextf32(&rng), 0.0f);
    f32 random_t = prng32_nextf32(&rng);
    f32 scale = 1.0f / (f32)i;
    scale = 0.2f + scale*scale*(0.8f);
    offsets[i].xyz = v3f_scale(scale*random_t, v3f_normalize_or_zero(offsets[i].xyz));
  }
}

function void
r_init(R_State *renderer_state, OS_Window window)
{
  renderer_state->arena = m_arena_reserve(MB(2));
  dx11_create_device(renderer_state);
  dx11_create_swap_chain(renderer_state, window);
  dx11_create_rasterizer_states(renderer_state);
  dx11_create_depth_stencil_states(renderer_state);
  dx11_create_blend_states(renderer_state);
  dx11_create_sampler_states(renderer_state);
  
  dx11_create_general_rendering_states(renderer_state);
  dx11_create_game_state(renderer_state);
  dx11_create_font_atlas(renderer_state);
  
  dx11_create_shadowmap(renderer_state);
  dx11_create_gbuffer(renderer_state);
}

function void
r_submit(R_State *state)
{
  D3D11_VIEWPORT viewport;
  viewport.Width = (f32)state->back_buffer_width;
  viewport.Height = (f32)state->back_buffer_height;
  viewport.MinDepth = 0;
  viewport.MaxDepth = 1;
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  
  f32 clear_colour[] = { 0.0f, 0.0f, 0.0f, 1.0f };
  ID3D11DeviceContext_ClearRenderTargetView(state->device_context, state->back_buffer_rtv, clear_colour);
  
  for (R_Pass *pass = state->first_pass;
       pass;
       pass = pass->next)
  {
    switch (pass->type)
    {
      case R_PassType_Game_WithLight:
      {
        R_Pass_Game *gwl = &pass->game;
        ID3D11DeviceContext_ClearRenderTargetView(state->device_context, state->scene_ssao_rtv, clear_colour);
        ID3D11DeviceContext_ClearRenderTargetView(state->device_context, state->scene_normals_rtv, clear_colour);
        ID3D11DeviceContext_ClearDepthStencilView(state->device_context, state->main_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        
        D3D11_MAPPED_SUBRESOURCE mapped_subrec;
        ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->sbuffers[R_SBufferType_Game], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
        CopyMemory(mapped_subrec.pData, gwl->instances, mapped_subrec.RowPitch);
        ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->sbuffers[R_SBufferType_Game], 0);
        
        {
          DX11_VertexShader_Constants init_cbuffer =
          {
            gwl->projection, gwl->world_to_camera,
            gwl->light_proj, gwl->world_to_light
          };
          
          ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_Game_VShader0], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
          CopyMemory(mapped_subrec.pData, &init_cbuffer, mapped_subrec.RowPitch);
          ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_Game_VShader0], 0);
        }
        
        {
          DX11_PixelShader_Constants init_cbuffer =
          {
            .eye_p_in_world = gwl->camera_p,
            .light = gwl->light,
          };
          
          //CopyMemory(init_cbuffer.lights, gwl->lights, sizeof(gwl->lights));
          
          ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_Game_PShader0], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
          CopyMemory(mapped_subrec.pData, &init_cbuffer, mapped_subrec.RowPitch);
          ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_Game_PShader0], 0);
        }
        
        ID3D11DeviceContext_IASetPrimitiveTopology(state->device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        UINT offset = 0;
        ID3D11DeviceContext_IASetVertexBuffers(state->device_context, 0, 1, &state->cube_model.vertices, &state->cube_model.struct_size, &offset);
        ID3D11DeviceContext_IASetIndexBuffer(state->device_context, state->cube_model.indices, DXGI_FORMAT_R32_UINT, 0);
        ID3D11DeviceContext_IASetInputLayout(state->device_context, state->input_layouts[R_InputLayoutType_Game]);
        
        ID3D11DeviceContext_VSSetConstantBuffers(state->device_context, 0, 1, &state->cbuffers[R_CBufferType_Game_VShader0]);
        ID3D11DeviceContext_VSSetShaderResources(state->device_context, 0, 1, &state->sbuffer_srvs[R_SBufferType_Game]);
        
        if (gwl->wire_frame)
        {
          ID3D11DeviceContext_RSSetState(state->device_context, state->raster_wire_cull_ccw);
        }
        else
        {
          ID3D11DeviceContext_RSSetState(state->device_context, state->raster_fill_cull_ccw);
        }
        ID3D11DeviceContext_RSSetViewports(state->device_context, 1, &viewport);
        
        ID3D11DeviceContext_OMSetDepthStencilState(state->device_context, state->ds_depth_only, 0);
        
        //
        // Accumulate Buffers for SSAO
        //
        ID3D11DeviceContext_VSSetShader(state->device_context, state->vshaders[R_VShaderType_GameSSAO_Accum], null, 0);
        ID3D11DeviceContext_PSSetShader(state->device_context, state->pshaders[R_PShaderType_GameSSAO_Accum], null, 0);
        ID3D11DeviceContext_OMSetRenderTargets(state->device_context, 1, &state->scene_normals_rtv, state->main_dsv);
        if (gwl->instances_count)
        {
          ID3D11DeviceContext_DrawIndexedInstanced(state->device_context,
                                                   state->cube_model.indices_count,
                                                   (UINT)gwl->instances_count,
                                                   0, 0, 0);
        }
        
        //
        // Create SSAO texture
        //
        f32 aspect = 720.0f / 1280.0f;
        f32 far_z = 100.0f;
        f32 fov_rad = DegToRad(66.2f);
        
        f32 right = far_z * tanf(fov_rad * 0.5f);
        f32 left = -right;
        f32 top = right * aspect;
        f32 bottom = -top;
        
        {
          DX11_SSAO_Constants1 constants;
          v4f *far_plane_ptr = constants.far_plane_ptr;
          far_plane_ptr[0] = v4f_make(left, top, far_z, 0.0f);
          far_plane_ptr[1] = v4f_make(left, bottom, far_z, 0.0f);
          far_plane_ptr[2] = v4f_make(right, top, far_z, 0.0f);
          far_plane_ptr[3] = v4f_make(right, bottom, far_z, 0.0f);
          CopyMemory(constants.samples_for_ssao, state->ssao_offsets, sizeof(state->ssao_offsets));
          
          ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_GameSSAO1], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
          CopyMemory(mapped_subrec.pData, &constants, mapped_subrec.RowPitch);
          ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_GameSSAO1], 0);
        }
        
        ID3D11DeviceContext_OMSetRenderTargets(state->device_context, 1, &state->scene_ssao_rtv, null);
        ID3D11DeviceContext_IASetPrimitiveTopology(state->device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        ID3D11DeviceContext_VSSetShader(state->device_context, state->vshaders[R_VShaderType_GameSSAO], null, 0);
        ID3D11DeviceContext_VSSetConstantBuffers(state->device_context, 1, 1, &state->cbuffers[R_CBufferType_GameSSAO1]);
        ID3D11DeviceContext_RSSetState(state->device_context, state->raster_fill_nocull_ccw);
        ID3D11DeviceContext_PSSetShader(state->device_context, state->pshaders[R_PShaderType_GameSSAO], null, 0);
        ID3D11DeviceContext_PSSetSamplers(state->device_context, 0, 1, &state->point_sampler_border_black);
        ID3D11DeviceContext_PSSetSamplers(state->device_context, 1, 1, &state->point_sampler_wrap);
        ID3D11DeviceContext_PSSetConstantBuffers(state->device_context, 0, 1, &state->cbuffers[R_CBufferType_Game_VShader0]);
        ID3D11DeviceContext_PSSetConstantBuffers(state->device_context, 1, 1, &state->cbuffers[R_CBufferType_GameSSAO1]);
        ID3D11DeviceContext_PSSetShaderResources(state->device_context, 1, 1, &state->scene_normals_srv);
        ID3D11DeviceContext_PSSetShaderResources(state->device_context, 2, 1, &state->scene_random_vec);
        ID3D11DeviceContext_Draw(state->device_context, 4, 0);

        // Blur the SSAO map

        ID3D11DeviceContext_OMSetRenderTargets(state->device_context, 1, &state->scene_ssao_blur_rtv, null);
        ID3D11DeviceContext_VSSetShader(state->device_context, state->vshaders[R_VShaderType_GameSSAO_Blur], null, 0);
        ID3D11DeviceContext_PSSetShader(state->device_context, state->pshaders[R_PShaderType_GameSSAO_Blur], null, 0);
        ID3D11DeviceContext_PSSetSamplers(state->device_context, 0, 1, &state->point_sampler_wrap);
        ID3D11DeviceContext_PSSetShaderResources(state->device_context, 0, 1, &state->scene_ssao_srv);
        ID3D11DeviceContext_Draw(state->device_context, 4, 0);

#if 1
        //
        // render the game
        //
        ID3D11DeviceContext_ClearDepthStencilView(state->device_context, state->main_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        ID3D11DeviceContext_OMSetRenderTargets(state->device_context, 1, &state->back_buffer_rtv, state->main_dsv);
        ID3D11DeviceContext_IASetPrimitiveTopology(state->device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D11DeviceContext_ClearDepthStencilView(state->device_context, state->main_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        ID3D11DeviceContext_VSSetShader(state->device_context, state->vshaders[R_VShaderType_Game], null, 0);
        ID3D11DeviceContext_PSSetShader(state->device_context, state->pshaders[R_PShaderType_GameWithLight], null, 0);
        ID3D11DeviceContext_PSSetConstantBuffers(state->device_context, 1, 1, &state->cbuffers[R_CBufferType_Game_PShader0]);
        ID3D11DeviceContext_PSSetSamplers(state->device_context, 0, 1, &state->shadow_map_sampler);
        ID3D11DeviceContext_PSSetSamplers(state->device_context, 1, 1, &state->point_sampler_clamp);
        ID3D11DeviceContext_PSSetShaderResources(state->device_context, 1, 1, &state->shadow_map_srv);
        ID3D11DeviceContext_PSSetShaderResources(state->device_context, 2, 1, &state->scene_ssao_blur_srv);
        
        if (gwl->wire_frame)
        {
          ID3D11DeviceContext_RSSetState(state->device_context, state->raster_wire_cull_ccw);
        }
        else
        {
          ID3D11DeviceContext_RSSetState(state->device_context, state->raster_fill_cull_ccw);
        }
        
        ID3D11DeviceContext_OMSetBlendState(state->device_context, (ID3D11BlendState *)state->blend_state_transparency, null, 0xffffffff);
        
        // NOTE(cj): Cube instance
        if (gwl->instances_count)
        {
          ID3D11DeviceContext_DrawIndexedInstanced(state->device_context,
                                                   state->cube_model.indices_count,
                                                   (UINT)gwl->instances_count,
                                                   0, 0, 0);
        }
#endif
      } break;
      
      case R_PassType_Game_Shadow:
      {
        R_Pass_Game *gwl = &pass->game;
        
        ID3D11DeviceContext_ClearDepthStencilView(state->device_context, state->shadow_map_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        
        D3D11_MAPPED_SUBRESOURCE mapped_subrec;
        ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->sbuffers[R_SBufferType_Game], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
        CopyMemory(mapped_subrec.pData, gwl->instances, mapped_subrec.RowPitch);
        ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->sbuffers[R_SBufferType_Game], 0);
        
        {
          DX11_VertexShader_Constants init_cbuffer =
          {
            gwl->light_proj, gwl->world_to_light,
          };
          
          ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_Game_VShader0], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
          CopyMemory(mapped_subrec.pData, &init_cbuffer, mapped_subrec.RowPitch);
          ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_Game_VShader0], 0);
        }
        
        ID3D11DeviceContext_IASetPrimitiveTopology(state->device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        UINT offset = 0;
        ID3D11DeviceContext_IASetVertexBuffers(state->device_context, 0, 1, &state->cube_model.vertices, &state->cube_model.struct_size, &offset);
        ID3D11DeviceContext_IASetIndexBuffer(state->device_context, state->cube_model.indices, DXGI_FORMAT_R32_UINT, 0);
        ID3D11DeviceContext_IASetInputLayout(state->device_context, state->input_layouts[R_InputLayoutType_Game]);
        
        ID3D11DeviceContext_VSSetShader(state->device_context, state->vshaders[R_VShaderType_GameShadows], null, 0);
        ID3D11DeviceContext_VSSetConstantBuffers(state->device_context, 0, 1, &state->cbuffers[R_CBufferType_Game_VShader0]);
        ID3D11DeviceContext_VSSetShaderResources(state->device_context, 0, 1, &state->sbuffer_srvs[R_SBufferType_Game]);
        
        ID3D11DeviceContext_PSSetShader(state->device_context, null, null, 0);
        
        ID3D11DeviceContext_RSSetViewports(state->device_context, 1, &state->shadow_map_vp);
        ID3D11DeviceContext_RSSetState(state->device_context, state->shadow_map_raster);
        
        ID3D11DeviceContext_OMSetDepthStencilState(state->device_context, state->ds_depth_only, 0);
        ID3D11DeviceContext_OMSetRenderTargets(state->device_context, 0, null, state->shadow_map_dsv);
        
        // NOTE(cj): Cube instance
        if (gwl->instances_count)
        {
          ID3D11DeviceContext_DrawIndexedInstanced(state->device_context,
                                                   state->cube_model.indices_count,
                                                   (UINT)gwl->instances_count,
                                                   0, 0, 0);
        }
      } break;
      
      case R_PassType_Game_WithoutLight:
      {
        R_Pass_Game *gwl = &pass->game;
        
        D3D11_MAPPED_SUBRESOURCE mapped_subrec;
        ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->sbuffers[R_SBufferType_Game], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
        CopyMemory(mapped_subrec.pData, gwl->instances, mapped_subrec.RowPitch);
        ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->sbuffers[R_SBufferType_Game], 0);
        
        {
          DX11_VertexShader_Constants init_cbuffer =
          {
            gwl->projection, gwl->world_to_camera,
          };
          
          ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_Game_VShader0], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
          CopyMemory(mapped_subrec.pData, &init_cbuffer, mapped_subrec.RowPitch);
          ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_Game_VShader0], 0);
        }
        
        ID3D11DeviceContext_IASetPrimitiveTopology(state->device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        UINT offset = 0;
        ID3D11DeviceContext_IASetVertexBuffers(state->device_context, 0, 1, &state->cube_model.vertices, &state->cube_model.struct_size, &offset);
        ID3D11DeviceContext_IASetIndexBuffer(state->device_context, state->cube_model.indices, DXGI_FORMAT_R32_UINT, 0);
        ID3D11DeviceContext_IASetInputLayout(state->device_context, state->input_layouts[R_InputLayoutType_Game]);
        
        ID3D11DeviceContext_VSSetShader(state->device_context, state->vshaders[R_VShaderType_Game], null, 0);
        ID3D11DeviceContext_VSSetConstantBuffers(state->device_context, 0, 1, &state->cbuffers[R_CBufferType_Game_VShader0]);
        ID3D11DeviceContext_VSSetShaderResources(state->device_context, 0, 1, &state->sbuffer_srvs[R_SBufferType_Game]);
        
        if (gwl->wire_frame)
        {
          ID3D11DeviceContext_RSSetState(state->device_context, state->raster_wire_cull_ccw);
        }
        else
        {
          ID3D11DeviceContext_RSSetState(state->device_context, state->raster_fill_cull_ccw);
        }
        ID3D11DeviceContext_RSSetViewports(state->device_context, 1, &viewport);
        
        ID3D11DeviceContext_PSSetShader(state->device_context, state->pshaders[R_PShaderType_GameWithoutLight], null, 0);
        
        ID3D11DeviceContext_OMSetDepthStencilState(state->device_context, state->ds_depth_only, 0);
        ID3D11DeviceContext_OMSetRenderTargets(state->device_context, 1, &state->back_buffer_rtv, state->main_dsv);
        ID3D11DeviceContext_OMSetBlendState(state->device_context, (ID3D11BlendState *)state->blend_state_transparency, null, 0xffffffff);
        
        // NOTE(cj): Cube instance
        if (gwl->instances_count)
        {
          ID3D11DeviceContext_DrawIndexedInstanced(state->device_context,
                                                   state->cube_model.indices_count,
                                                   (UINT)gwl->instances_count,
                                                   0, 0, 0);
        }
      } break;
      
      case R_PassType_UI:
      {
        R_Pass_UI *ui = &pass->ui;
        
        D3D11_MAPPED_SUBRESOURCE mapped_subrec;
        ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->sbuffers[R_SBufferType_UI], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
        CopyMemory(mapped_subrec.pData, ui->rects, mapped_subrec.RowPitch);
        ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->sbuffers[R_SBufferType_UI], 0);
        
        {
          DX11_UI_VShader_Constants init_cbuffer =
          {
            m44_make_orthographic_z01(0, viewport.Width, 0, viewport.Height, 0, 1),
          };
          
          ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_UI_VShader0], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
          CopyMemory(mapped_subrec.pData, &init_cbuffer, mapped_subrec.RowPitch);
          ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *)state->cbuffers[R_CBufferType_UI_VShader0], 0);
        }
        
        ID3D11DeviceContext_IASetPrimitiveTopology(state->device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        
        ID3D11DeviceContext_VSSetShader(state->device_context, state->vshaders[R_VShaderType_UI], null, 0);
        ID3D11DeviceContext_VSSetConstantBuffers(state->device_context, 0, 1, &state->cbuffers[R_CBufferType_UI_VShader0]);
        ID3D11DeviceContext_VSSetShaderResources(state->device_context, 0, 1, &state->sbuffer_srvs[R_SBufferType_UI]);
        
        ID3D11DeviceContext_RSSetState(state->device_context, state->raster_fill_nocull_ccw);
        ID3D11DeviceContext_RSSetViewports(state->device_context, 1, &viewport);
        
        ID3D11DeviceContext_PSSetShader(state->device_context, state->pshaders[R_PShaderType_UI], null, 0);
        ID3D11DeviceContext_PSSetShaderResources(state->device_context, 1, 1, &state->font.srv);
        
        ID3D11DeviceContext_OMSetRenderTargets(state->device_context, 1, &state->back_buffer_rtv, 0);
        ID3D11DeviceContext_OMSetBlendState(state->device_context, (ID3D11BlendState *)state->blend_state_transparency, null, 0xffffffff);
        
        if (ui->count)
        {
          ID3D11DeviceContext_DrawInstanced(state->device_context, 4,
                                            (UINT)ui->count,
                                            0, 0);
        }
      } break;
      
      InvalidDefaultCase;
    }
    
    ID3D11DeviceContext_ClearState(state->device_context);
  }
  
  IDXGISwapChain_Present(state->swap_chain, 1, 0);
  
  m_arena_clear(state->arena);
  state->first_pass = state->last_pass = 0;
}

function R_Pass *
r_acquire_pass(R_State *state, R_PassType type)
{
  R_Pass *result = M_Arena_PushStruct(state->arena, R_Pass);
  R_Pass zero_pass = {0};
  *result = zero_pass;
  SLLPushBack(state->first_pass, state->last_pass, result);
  result->type = type;
  return(result);
}

function R_Pass *
r_acquire_game_with_light_pass(R_State *state, v3f camera_p, m44 perspective, m44 world_to_camera,
                               m44 light_proj, m44 world_to_light, b32 wire_frame)
{
  R_Pass *result = r_acquire_pass(state, R_PassType_Game_WithLight);
  result->game.camera_p = camera_p;
  result->game.projection = perspective;
  result->game.world_to_camera = world_to_camera;
  result->game.wire_frame = wire_frame;
  result->game.light_proj = light_proj;
  result->game.world_to_light = world_to_light;
  return(result);
}

function R_Pass *
r_acquire_game_without_light_pass(R_State *state, m44 perspective, m44 world_to_camera, b32 wire_frame)
{
  R_Pass *result = r_acquire_pass(state, R_PassType_Game_WithoutLight);
  result->game.projection = perspective;
  result->game.world_to_camera = world_to_camera;
  result->game.wire_frame = wire_frame;
  return(result);
}

function R_Light *
r_game_add_point_light(R_Pass *pass, v3f p, v4f colour)
{
  Assert((pass->type == R_PassType_Game_WithLight) ||
         (pass->type == R_PassType_Game_Shadow));
  
  R_Pass_Game *g = &(pass->game);
  
  R_Light *result = &g->light;
  result->p = p;
  result->type = R_LightType_PointLight;
  result->colour = colour;
  return(result);
}

function R_Light *
r_game_add_directional_light(R_Pass *pass, v3f p, v3f dir, v4f colour)
{
  Assert((pass->type == R_PassType_Game_WithLight) ||
         (pass->type == R_PassType_Game_Shadow));
  
  R_Pass_Game *g = &(pass->game);
  
  R_Light *result = &g->light;
  result->dir = dir;
  result->type = R_LightType_DirectionalLight;
  result->colour = colour;
  result->p = p;
  return(result);
}

function R_Pass *
r_acquire_game_shadow_pass(R_State *state, m44 projection, m44 world_to_light)
{
  R_Pass *result = r_acquire_pass(state, R_PassType_Game_Shadow);
  result->game.light_proj = projection;
  result->game.world_to_light = world_to_light;
  return(result);
}

function R_Model_Instance *
r_game_add_instance(R_Pass *pass, v3f p, v3f scale, v4f colour)
{
  Assert((pass->type == R_PassType_Game_WithLight) ||
         (pass->type == R_PassType_Game_Shadow) ||
         (pass->type == R_PassType_Game_WithoutLight));
  
  R_Pass_Game *g = &(pass->game);
  Assert(g->instances_count < ArrayCount(g->instances));
  
  R_Model_Instance *result = g->instances + g->instances_count++;
  result->p = p;
  result->scale = scale;
  result->colour = colour;
  return(result);
}

inline function R_Pass *
r_acquire_ui_pass(R_State *state)
{
  R_Pass *result = r_acquire_pass(state, R_PassType_UI);
  result->ui.font = &state->font;
  return(result);
}

inline function R_UI_Rect *
r_ui_acquire_rect(R_Pass *pass)
{
  Assert(pass->type == R_PassType_UI);
  R_Pass_UI *ui = &pass->ui;
  Assert(ui->count < R_MaxUIRects);
  R_UI_Rect *result = ui->rects + ui->count++;
  return(result);
}

function R_UI_Rect *
r_ui_add_tex_clipped(R_Pass *pass, u32 tex_id, v2f p,
                     v2f dims, v2f clip_p, v2f clip_dims,
                     v4f colour)
{
  R_UI_Rect *result = r_ui_acquire_rect(pass);
  result->tex_id = tex_id;
  result->p = p;
  result->dims = dims;
  result->vertex_colour = colour;
  
  f32 x_start = clip_p.x / (f32)pass->ui.font->sheet_width;
  f32 x_end = (clip_p.x + clip_dims.x) / (f32)pass->ui.font->sheet_width;
  f32 y_start = clip_p.y / (f32)pass->ui.font->sheet_height;
  f32 y_end = (clip_p.y + clip_dims.y) / (f32)pass->ui.font->sheet_height;
  
  result->uvs[0] = (v2f){ x_start, y_start };
  result->uvs[1] = (v2f){ x_start, y_end };
  result->uvs[2] = (v2f){ x_end, y_start };
  result->uvs[3] = (v2f){ x_end, y_end };
  
  return(result);
}

function v2f
r_ui_get_text_dims(R_Pass *pass, String_U8_Const str, ...)
{
  M_Arena *temp_arena = m_get_for_transient_purposes(0, 0);
  M_Temp temp = m_temp_begin(temp_arena);
  
  va_list args;
  va_start(args, str);
  String_U8_Const format = str8_format_va(temp_arena, str, args);
  va_end(args);
  
  v2f final_dims = {0};
  for (u64 char_idx = 0; char_idx < format.count; ++char_idx)
  {
    u8 char_val = format.s[char_idx];
    Assert((char_val >= 32) && (char_val < 128));
    
    R_Glyph glyph = pass->ui.font->glyphs[char_val];
    if (char_val != ' ')
    {
      if (glyph.clip_height > final_dims.y)
      {
        final_dims.y = glyph.clip_height;
      }
    }
    final_dims.x += glyph.advance;
  }
  
  m_temp_end(temp);
  return(final_dims);
}

function v2f
r_ui_textf(R_Pass *pass, v2f p, v4f colour, String_U8_Const str, ...)
{
  M_Arena *temp_arena = m_get_for_transient_purposes(0, 0);
  M_Temp temp = m_temp_begin(temp_arena);
  
  va_list args;
  va_start(args, str);
  String_U8_Const format = str8_format_va(temp_arena, str, args);
  va_end(args);
  
  v2f final_dims = {0};
  v2f pen_p = p;
  for (u64 char_idx = 0; char_idx < format.count; ++char_idx)
  {
    u8 char_val = format.s[char_idx];
    Assert((char_val >= 32) && (char_val < 128));
    
    R_Glyph glyph = pass->ui.font->glyphs[char_val];
    if (char_val != ' ')
    {
      if (glyph.clip_height > final_dims.y)
      {
        final_dims.y = glyph.clip_height;
      }
      
      v2f glyph_p = { pen_p.x, pen_p.y };
      v2f glyph_dims = { glyph.clip_width, glyph.clip_height, };
      v2f glyph_clip_p = { glyph.clip_x, glyph.clip_y };
      r_ui_add_tex_clipped(pass, 1, glyph_p,
                           glyph_dims, glyph_clip_p,
                           glyph_dims, colour);
    }
    final_dims.x += glyph.advance;
    pen_p.x += glyph.advance;
  }
  
  m_temp_end(temp);
  return(final_dims);
}