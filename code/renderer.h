/* date = May 3rd 2025 7:58 pm */

#ifndef RENDERER_H
#define RENDERER_H

typedef struct
{
  v3f v;
  v3f t, b, n;
  v2f uv;
} R_Game_Vertex;

typedef struct
{
  ID3D11Buffer *vertices;
  ID3D11Buffer *indices;
  UINT struct_size;
  UINT indices_count;
} R_Model;

#define R_LightCount_Max 8
typedef u32 R_LightType;
enum
{
  R_LightType_PointLight,
  R_LightType_Count,
};

typedef struct
{
  v3f p;//world coordinate
  R_LightType type;
  v4f colour;
} R_Light;

typedef struct
{
  v3f p;
  v3f scale;
  // v4f quaterion;
  v4f colour;
} R_Model_Instance;

typedef u64 R_PassType;
enum
{
  R_PassType_GameRenderWithLight,
  R_PassType_GameRenderWithoutLight,
  R_PassType_Count,
};

#define R_MaxModelInstances 2048*8
typedef struct
{
  R_Model_Instance instances[R_MaxModelInstances];
  u64 instances_count;
  
  R_Light lights[R_LightCount_Max];
  u64 lights_count;
  
  v3f camera_p;
  m44 perspective, world_to_camera;
} R_Pass_GameWithLight;

typedef struct
{
  R_Model_Instance instances[R_MaxModelInstances];
  u64 instances_count;
  
  m44 perspective, world_to_camera;
} R_Pass_GameWithoutLight;

typedef struct R_Pass R_Pass;
struct R_Pass
{
  R_PassType type;
  union
  {
    R_Pass_GameWithLight withlight;
    R_Pass_GameWithoutLight withoutlight;
  };
  R_Pass *next;
};

typedef struct
{
  m44 proj;
  m44 world_to_camera;
} DX11_VertexShader_Constants;

typedef struct
{
  v3f eye_p_in_world;
  u32 lights_count;
	R_Light lights[R_LightCount_Max];
} DX11_PixelShader_Constants;

typedef struct
{
  M_Arena *arena;
  
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
  
  ID3D11Buffer *game_vshad_cbuffer;
  ID3D11Buffer *game_pshad_cbuffer;
  ID3D11Buffer *game_sbuffer;
  ID3D11ShaderResourceView *game_sbuffer_srv;
  
  R_Model cube_model;
  ID3D11VertexShader *game_vshader_main;
  ID3D11PixelShader *game_pshader_main;
  ID3D11PixelShader *game_pshader_nolight;
  ID3D11InputLayout *game_input_layput;
  
  R_Pass *first_pass, *last_pass;
} R_State;

function void r_init(R_State *state, OS_Window window);
function void r_submit(R_State *state);

function R_Pass *r_acquire_pass(R_State *state, R_PassType type);

function R_Pass *r_acquire_game_with_light_pass(R_State *state, v3f camera_p, m44 perspective, m44 world_to_camera);
function R_Model_Instance *r_game_with_light_add_instance(R_Pass *pass, v3f p, v3f scale, v4f colour);

function R_Pass *r_acquire_game_without_light_pass(R_State *state, m44 perspective, m44 world_to_camera);
function R_Model_Instance *r_game_without_light_add_instance(R_Pass *pass, v3f p, v3f scale, v4f colour);

#endif //RENDERER_H
