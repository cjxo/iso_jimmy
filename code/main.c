#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#define NOMINMAX
#include <windows.h>
#include <timeapi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#if defined(ISO_DEBUG)
# include <dxgidebug.h>
#endif

#include <math.h>
#include "base.h"
#include "windows_stuff.h"
#include "my_math.h"

#include "base.c"
#include "windows_stuff.c"
#include "my_math.c"

typedef struct
{
  m44 proj;
  m44 world_to_camera;
} DX11_VertexShader_Constants;

typedef struct
{
  v3f v;
  v3f t, b, n;
  v2f uv;
} R_Game_Vertex;

typedef struct
{
  ID3D11VertexShader *vertex;
  ID3D11PixelShader *pixel;
} R_Shader;

typedef struct
{
  ID3D11Buffer *vertices;
  ID3D11Buffer *indices;
  UINT struct_size;
  UINT indices_count;
} R_Model;

typedef struct
{
  v3f p;
  v3f scale;
  // v4f quaterion;
  v4f colour;
} R_Model_Instance;

typedef struct
{
  R_Model_Instance *ins;
  u64 capacity;
  u64 count;
} R_Model_Instances;

typedef struct
{
  ID3D11Device *device;
  ID3D11Device1 *device1;
  ID3D11DeviceContext *device_context;
  
  s32 back_buffer_width, back_buffer_height;
  IDXGISwapChain1 *swap_chain;
  ID3D11RenderTargetView *back_buffer_rtv;
  ID3D11DepthStencilView *main_dsv;
  
  ID3D11RasterizerState *raster_fill_cull_ccw;
  ID3D11RasterizerState *raster_wire_cull_ccw;
  ID3D11DepthStencilState *ds_depth_only;
  ID3D11BlendState1 *blend_state_transparency;
  
  ID3D11Buffer *game_cbuffer;
  ID3D11Buffer *game_sbuffer;
  ID3D11ShaderResourceView *game_sbuffer_srv;
  
  R_Model cube_model;
  R_Shader game_shader;
  ID3D11InputLayout *game_input_layput;
  
  R_Model_Instances model_instances;
} R_State;

function void
dx11_create_device(R_State *renderer_state)
{
  HRESULT hr;
  UINT device_flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
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
          .Scaling = DXGI_SCALING_NONE,
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

#if ISO_DEBUG
# define DX11_ShaderFlags (D3DCOMPILE_SKIP_OPTIMIZATION|D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS)
#else
# define DX11_ShaderFlags (D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_ENABLE_STRICTNESS)
#endif
function R_Shader
dx11_create_shaders_with_input_layout(ID3D11Device *device, LPCWSTR file, char *vshader_entry, char *pshader_entry,
                                      D3D11_INPUT_ELEMENT_DESC *input_elements, u32 input_elements_count,
                                      ID3D11InputLayout **il_out)
{
  ID3D10Blob *bytecode, *error;
  HRESULT hr;
  
  R_Shader result={0};
  hr = D3DCompileFromFile(file, null, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                          vshader_entry, "vs_5_0", DX11_ShaderFlags, 0, &bytecode, &error);
  if (SUCCEEDED(hr))
  {
    hr = ID3D11Device_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(bytecode), 
                                         ID3D10Blob_GetBufferSize(bytecode), 0, &result.vertex);
    
    
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
                                              ID3D10Blob_GetBufferSize(bytecode), 0, &result.pixel);
          
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
  
  return(result);
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
dx11_create_game_state(R_State *state, M_Arena *arena)
{
  HRESULT hr;
  D3D11_BUFFER_DESC buffer_desc;
  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
  
  D3D11_INPUT_ELEMENT_DESC il_desc[] =
  {
    { "IA_Vertex", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "IA_Tangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "IA_Bitangent", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "IA_Normal", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "IA_UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };
  
  state->game_shader = dx11_create_shaders_with_input_layout(state->device, L"..\\code\\shaders\\game.hlsl",
                                                             "vs_main", "ps_main",
                                                             il_desc,
                                                             ArrayCount(il_desc),
                                                             &state->game_input_layput);
  
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
  
  buffer_desc.ByteWidth = (sizeof(DX11_VertexShader_Constants)+15)&(~15);
  buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
  buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  buffer_desc.MiscFlags = 0;
  buffer_desc.StructureByteStride = 0;
  
  hr = ID3D11Device_CreateBuffer(state->device, &buffer_desc, 0, &state->game_cbuffer);
  if (SUCCEEDED(hr))
  {
    state->model_instances.capacity = 2048;
    state->model_instances.count = 0;
    state->model_instances.ins = M_Arena_PushArray(arena, R_Model_Instance, state->model_instances.capacity);
    
    buffer_desc.ByteWidth = (UINT)(sizeof(R_Model_Instance)*state->model_instances.capacity);
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.StructureByteStride = sizeof(R_Model_Instance);
    
    hr = ID3D11Device_CreateBuffer(state->device, &buffer_desc, 0, &state->game_sbuffer);
    if (SUCCEEDED(hr))
    {
      srv_desc.Format = DXGI_FORMAT_UNKNOWN;
      srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
      srv_desc.Buffer.ElementOffset = 0;
      srv_desc.Buffer.NumElements = (UINT)state->model_instances.capacity;
      hr = ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *)state->game_sbuffer, &srv_desc, &state->game_sbuffer_srv);
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
r_init(R_State *renderer_state, OS_Window window, M_Arena *arena)
{
  dx11_create_device(renderer_state);
  dx11_create_swap_chain(renderer_state, window);
  dx11_create_rasterizer_states(renderer_state);
  dx11_create_depth_stencil_states(renderer_state);
  dx11_create_blend_states(renderer_state);
  dx11_create_game_state(renderer_state, arena);
}

function R_Model_Instance *
r_model_add_instance(R_Model_Instances *instances, v3f p, v3f scale, v4f colour)
{
  Assert(instances->count < instances->capacity);
  R_Model_Instance *result = instances->ins + instances->count++;
  result->p = p;
  result->scale = scale;
  result->colour = colour;
  return(result);
}

int __stdcall
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmd, int nShowCmd)
{
  (void)hInstance;
  (void)hPrevInstance;
  (void)lpCmd;
  (void)nShowCmd;
  
  SetProcessDPIAware();
  
  timeBeginPeriod(1);
  
  DEVMODE dev_mode;
  EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &dev_mode);
  u64 refresh_rate = dev_mode.dmDisplayFrequency;
  f32 seconds_per_frame = 1.0f / (f32)refresh_rate;
  
  OS_Window window;
  os_create_window(&window, "Iso Jimmy", 1280, 720);
  
  OS_Input *input = &(window.input);
  
  M_Arena *arena = m_arena_reserve(MB(4));
  R_State renderer_state;
  r_init(&renderer_state, window, arena);
  
  v3f player_p = v3f_make(0, 1, 0);
  b32 can_move_now = true;
  f32 t_move = 0.0f;
  v3f requested_move = player_p;
  
  v3f camera_look_to = player_p;
  
  while (true)
  {
    w32_fill_input(&window);
    
    if (OS_KeyReleased(input, OS_Input_KeyType_Escape))
    {
      ExitProcess(0);
    }
    
    if (can_move_now)
    {
      f32 move_units = 1.0f;
      if (OS_KeyHeld(input, OS_Input_KeyType_W))
      {
        requested_move.z += move_units;
        can_move_now = false;
      }
      
      if (OS_KeyHeld(input, OS_Input_KeyType_S))
      {
        requested_move.z -= move_units;
        can_move_now = false;
      }
      
      if (OS_KeyHeld(input, OS_Input_KeyType_A))
      {
        requested_move.x -= move_units;
        can_move_now = false;
      }
      
      if (OS_KeyHeld(input, OS_Input_KeyType_D))
      {
        requested_move.x += move_units;
        can_move_now = false;
      }
    }
    else
    {
      if (t_move >= 0.99f)
      {
        player_p = requested_move;
        t_move = 0.0f;
        can_move_now = true;
      }
      else
      {
        player_p.x = player_p.x * (1.0f - t_move) + t_move * requested_move.x;
        f32 hop_height = 1.0f / 10.0f;
        player_p.y = player_p.y * (1.0f - t_move) + t_move * requested_move.y - hop_height*t_move*t_move + hop_height;
        player_p.z = player_p.z * (1.0f - t_move) + t_move * requested_move.z;
        
        t_move += seconds_per_frame*3;
      }
    }
    
    for (u32 block_z = 0; block_z < 30; ++block_z)
    {
      for (u32 block_x = 0; block_x < 30; ++block_x)
      {
        r_model_add_instance(&renderer_state.model_instances,
                             v3f_make((f32)block_x, 0.0f, (f32)block_z),
                             v3f_make(1.0f, 1.0f, 1.0f),
                             v4f_make(1.0f, 0.3f, 0.4f, 1.0f));
      }
    }
    
    r_model_add_instance(&renderer_state.model_instances,
                         player_p,
                         v3f_make(1.0f, 1.0f, 1.0f),
                         v4f_make(1.0f, 1.0f, 1.0f, 1.0f));
    
    D3D11_VIEWPORT viewport;
    viewport.Width = (f32)renderer_state.back_buffer_width;
    viewport.Height = (f32)renderer_state.back_buffer_height;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    
    f32 clear_colour[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    ID3D11DeviceContext_ClearRenderTargetView(renderer_state.device_context, renderer_state.back_buffer_rtv, clear_colour);
    ID3D11DeviceContext_ClearDepthStencilView(renderer_state.device_context, renderer_state.main_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    
    D3D11_MAPPED_SUBRESOURCE mapped_subrec;
    ID3D11DeviceContext_Map(renderer_state.device_context, (ID3D11Resource *)renderer_state.game_sbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
    CopyMemory(mapped_subrec.pData, renderer_state.model_instances.ins, mapped_subrec.RowPitch);
    ID3D11DeviceContext_Unmap(renderer_state.device_context, (ID3D11Resource *)renderer_state.game_sbuffer, 0);
    
    f32 camera_lock_strength = 0.03f;
    camera_look_to.x = camera_look_to.x * (1.0f - camera_lock_strength) + camera_lock_strength * player_p.x;
    camera_look_to.y = 0;
    camera_look_to.z = camera_look_to.z * (1.0f - camera_lock_strength) + camera_lock_strength * player_p.z;
    v3f camera_look_from = v3f_make(camera_look_to.x + 10, 10, camera_look_to.z - 10);
    v3f camera_look = v3f_sub(camera_look_to, camera_look_from);
    v3f camera_up = v3f_make(0, 1, 0);
    
    v3f camera_x, camera_y, camera_z;
    {
      v3f a;
      a = v3f_scale(v3f_dot(camera_up, camera_look) / v3f_dot(camera_look, camera_look), camera_look);
      camera_z = v3f_normalize_or_zero(camera_look);
      camera_y = v3f_sub_and_normalize_or_zero(camera_up, a);
      camera_x = v3f_normalize_or_zero(v3f_cross(camera_y, camera_z));
    }
    
    DX11_VertexShader_Constants init_cbuffer =
    {
      .proj = m44_perspective_dx11((f32)renderer_state.back_buffer_height/(f32)renderer_state.back_buffer_width,
                                   DegToRad(66.2f), 1.0f, 100.0f),
      .world_to_camera = (m44)
      {
        camera_x.x, camera_x.y, camera_x.z, -v3f_dot(camera_look_from, camera_x),
        camera_y.x, camera_y.y, camera_y.z, -v3f_dot(camera_look_from, camera_y),
        camera_z.x, camera_z.y, camera_z.z, -v3f_dot(camera_look_from, camera_z),
        0.0f, 0.0f, 0.0f, 1.0f,
      },
    };
    
    ID3D11DeviceContext_Map(renderer_state.device_context, (ID3D11Resource *)renderer_state.game_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subrec);
    CopyMemory(mapped_subrec.pData, &init_cbuffer, mapped_subrec.RowPitch);
    ID3D11DeviceContext_Unmap(renderer_state.device_context, (ID3D11Resource *)renderer_state.game_cbuffer, 0);
    
    ID3D11DeviceContext_IASetPrimitiveTopology(renderer_state.device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT offset = 0;
    ID3D11DeviceContext_IASetVertexBuffers(renderer_state.device_context, 0, 1, &renderer_state.cube_model.vertices, &renderer_state.cube_model.struct_size, &offset);
    ID3D11DeviceContext_IASetIndexBuffer(renderer_state.device_context, renderer_state.cube_model.indices, DXGI_FORMAT_R32_UINT, 0);
    ID3D11DeviceContext_IASetInputLayout(renderer_state.device_context, renderer_state.game_input_layput);
    
    ID3D11DeviceContext_VSSetShader(renderer_state.device_context, renderer_state.game_shader.vertex, null, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(renderer_state.device_context, 0, 1, &renderer_state.game_cbuffer);
    ID3D11DeviceContext_VSSetShaderResources(renderer_state.device_context, 0, 1, &renderer_state.game_sbuffer_srv);
    
    ID3D11DeviceContext_RSSetState(renderer_state.device_context, renderer_state.raster_wire_cull_ccw);
    ID3D11DeviceContext_RSSetViewports(renderer_state.device_context, 1, &viewport);
    
    ID3D11DeviceContext_PSSetShader(renderer_state.device_context, renderer_state.game_shader.pixel, null, 0);
    
    ID3D11DeviceContext_OMSetDepthStencilState(renderer_state.device_context, renderer_state.ds_depth_only, 0);
    ID3D11DeviceContext_OMSetRenderTargets(renderer_state.device_context, 1, &renderer_state.back_buffer_rtv, renderer_state.main_dsv);
    ID3D11DeviceContext_OMSetBlendState(renderer_state.device_context, (ID3D11BlendState *)renderer_state.blend_state_transparency, null, 0xffffffff);
    
    // NOTE(cj): Cube instance
    if (renderer_state.model_instances.count)
    {
      ID3D11DeviceContext_DrawIndexedInstanced(renderer_state.device_context,
                                               renderer_state.cube_model.indices_count,
                                               (UINT)renderer_state.model_instances.count,
                                               0, 0, 0);
    }
    
    IDXGISwapChain_Present(renderer_state.swap_chain, 1, 0);
    ID3D11DeviceContext_ClearState(renderer_state.device_context);
    
    renderer_state.model_instances.count = 0;
  }
  
  ExitProcess(0);
}