/* date = May 3rd 2025 7:58 pm */

#ifndef RENDERER_H
#define RENDERER_H

typedef struct
{
  f32 advance;
  f32 clip_x;
  f32 clip_y;
  f32 clip_width;
  f32 clip_height;
} R_Glyph;

typedef struct
{
  R_Glyph glyphs[128];
  f32 ascent, descent;
  ID3D11ShaderResourceView *srv;
  u32 sheet_width, sheet_height;
} R_Font;

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
  R_LightType_DirectionalLight,
  R_LightType_Count,
};

typedef struct
{
  v3f p;//world coordinate
  u32 type;
  v4f colour;
  v3f dir;
  f32 z_far;
} R_Light;

typedef struct
{
  v3f p;
  v3f scale;
  v3f rotation;
  v4f colour;
} R_Model_Instance;

typedef u64 R_PassType;
enum
{
  R_PassType_Game,
  R_PassType_Game_Shadow,
  R_PassType_Game_SSAO,
  R_PassType_UI,
  R_PassType_Count,
};

#define R_MaxModelInstances 2048*24

typedef struct
{
  b32 is_ortho;
  v3f p;
  f32 z_near, z_far;
  f32 aspect_height_over_width;
  f32 left, right, bottom, top;
  f32 fov_rad;
  v3f look_at;
} R_CameraConfig;

typedef struct
{  
  R_CameraConfig world_camera;
  b32 wire_frame;
} R_Pass_Game;

typedef struct
{
  v2f p;
  v2f dims;
  v4f vertex_colour;
  v2f uvs[4];
  u32 tex_id;
} R_UI_Rect;

#define R_MaxUIRects 512
typedef struct
{
  R_UI_Rect rects[R_MaxUIRects];
  u64 count;
  
  R_Font *font;
} R_Pass_UI;

typedef struct R_Pass R_Pass;
struct R_Pass
{
  R_PassType type;
  
  union
  {
    R_Pass_Game game;
    R_Pass_UI ui;
  };
  R_Pass *next;
};

typedef struct
{
  m44 proj;
  m44 world_to_camera;
  m44 light_proj;
  m44 world_to_light;
} DX11_VertexShader_Constants;

__declspec(align(16)) typedef struct
{
	R_Light light;
  v3f eye_p_in_world;
  f32 pad_a[1];
} DX11_PixelShader_Constants;

typedef struct
{
  m44 proj;
} DX11_UI_VShader_Constants;

#define R_SSAO_SampleCount 32
__declspec(align(16)) typedef struct
{
  v4f far_plane_ptr[4], samples_for_ssao[R_SSAO_SampleCount];
} DX11_SSAO_Constants1;

typedef enum
{
  R_VShaderType_Game,
  R_VShaderType_GameShadows,
  R_VShaderType_GameCubeShadows,
  R_VShaderType_GameSSAO_Accum,
  R_VShaderType_GameSSAO,
  R_VShaderType_GameSSAO_Blur,
  R_VShaderType_UI,
  R_VShaderType_Count,
} R_VShaderType;

typedef enum
{
  R_PShaderType_Game,
  R_PShaderType_GameSSAO_Accum,
  R_PShaderType_GameCubeShadows,
  R_PShaderType_GameSSAO,
  R_PShaderType_GameSSAO_Blur,
  R_PShaderType_UI,
  R_PShaderType_Count,
} R_PShaderType;

typedef enum
{
  R_SBufferType_Game,
  R_SBufferType_UI,
  R_SBufferType_Count,
} R_SBufferType;

typedef enum
{
  R_CBufferType_Game_VShader0,
  R_CBufferType_Game_PShader0,
  R_CBufferType_GameSSAO1,
  R_CBufferType_UI_VShader0,
  R_CBufferType_Count,
} R_CBufferType;

typedef enum
{
  R_InputLayoutType_Game,
  R_InputLayoutType_Count,
} R_InputLayoutType;

typedef struct
{
  M_Arena *arena, *temp_arena;
  
  ID3D11Device *device;
  ID3D11Device1 *device1;
  ID3D11DeviceContext *device_context;
  
  s32 back_buffer_width, back_buffer_height;
  IDXGISwapChain1 *swap_chain;
  ID3D11RenderTargetView *back_buffer_rtv;
  ID3D11DepthStencilView *main_dsv;
  
  ID3D11RasterizerState *raster_fill_cull_ccw;
  ID3D11RasterizerState *raster_wire_cull_ccw;
  ID3D11RasterizerState *raster_fill_nocull_ccw;
  ID3D11DepthStencilState *ds_depth_only;
  ID3D11BlendState1 *blend_state_transparency;
  
  ID3D11SamplerState *point_sampler_clamp;
  ID3D11SamplerState *point_sampler_wrap;
  ID3D11SamplerState *point_sampler_border_black;
  
  ID3D11VertexShader *vshaders[R_VShaderType_Count];
  ID3D11PixelShader *pshaders[R_PShaderType_Count];
  ID3D11Buffer *sbuffers[R_SBufferType_Count];
  ID3D11ShaderResourceView *sbuffer_srvs[R_SBufferType_Count];
  ID3D11Buffer *cbuffers[R_CBufferType_Count];
  ID3D11InputLayout *input_layouts[R_InputLayoutType_Count];
  
  // TODO(cj): We need this for deferred rendering.
  // for now, we want SSAO working!
  ID3D11RenderTargetView *scene_normals_rtv;
  ID3D11RenderTargetView *scene_ssao_rtv;
  ID3D11RenderTargetView *scene_ssao_blur_rtv;
  ID3D11ShaderResourceView *scene_normals_srv;
  ID3D11ShaderResourceView *scene_ssao_srv;
  ID3D11ShaderResourceView *scene_ssao_blur_srv;
  ID3D11ShaderResourceView *scene_random_vec;
  v4f ssao_offsets[R_SSAO_SampleCount];
  
  ID3D11RasterizerState *shadow_map_raster;
  ID3D11SamplerState *shadow_map_sampler;
  D3D11_VIEWPORT shadow_map_vp;
  ID3D11DepthStencilView *shadow_map_dsv;
  ID3D11ShaderResourceView *shadow_map_srv;

  D3D11_VIEWPORT shadow_cubemap_vp;
  ID3D11DepthStencilView *shadow_cubemap_dsv[6];
  ID3D11ShaderResourceView *shadow_cubemap_srv;
  
  R_Model cube_model;
  
  HFONT font_handle;
  R_Font font;
  
  R_CameraConfig light_camera;
  R_Light light;
  R_Model_Instance *instances;
  u64 instances_count;
  R_Pass *first_pass, *last_pass;
} R_State;

function void r_init(R_State *state, OS_Window window);
function void r_submit(R_State *state);

function R_Pass *r_acquire_pass(R_State *state, R_PassType type);

function R_Pass *r_acquire_game_pass(R_State *state, R_CameraConfig world_camera, b32 wire_frame);
function R_Pass *r_acquire_ssao_pass(R_State *state, R_CameraConfig world_camera);
function R_Pass *r_acquire_game_shadow_pass(R_State *state);

function R_Model_Instance *r_add_plane(R_State *state, v3f p, v3f scale, v3f rotation, v4f colour);
function R_Light          *r_set_directional_light(R_State *state, v3f p, v3f dir, v4f colour);
function R_Light          *r_set_point_light(R_State *state, v3f p, v4f colour);

inline function R_Pass *r_acquire_ui_pass(R_State *state);
inline function R_UI_Rect *r_ui_acquire_rect(R_Pass *pass);
function R_UI_Rect *r_ui_add_tex_clipped(R_Pass *pass, u32 tex_id, v2f p,
                                         v2f dims, v2f clip_p, v2f clip_dims,
                                         v4f colour);
function v2f r_ui_textf(R_Pass *pass, v2f p, v4f colour, String_U8_Const str, ...);
function v2f r_ui_get_text_dims(R_Pass *pass, String_U8_Const str, ...);

#endif //RENDERER_H