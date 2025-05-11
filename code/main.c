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
#include <Windowsx.h>
#if defined(ISO_DEBUG)
# include <dxgidebug.h>
#endif

#include <math.h>
#include "base.h"
#include "prng.h"
#include "windows_stuff.h"
#include "my_math.h"
#include "renderer.h"

#include "base.c"
#include "prng.c"
#include "windows_stuff.c"
#include "my_math.c"
#include "renderer.c"

typedef u64 G_BlockType;
enum
{
  G_BlockType_Grass,
  G_BlockType_Dirt,
  G_BlockType_Count,
};

typedef u64 G_BlockFlag;
enum
{
  G_BlockFlag_Active = 0x1,
};

typedef struct
{
  G_BlockFlag flags;
  G_BlockType type;
} G_Block;

typedef struct
{
  v3f look_to;
  v3f look_from;
  f32 near_z, far_z;
  f32 fov_rad;
  f32 aspect;
  f32 lock_strength;
  
  b32 debug_cam;
  f32 debug_lock_strength;
  v3f debug_cam_p;
  f32 debug_cam_no, debug_cam_yes;
  
  v3f x, y, z;
  
  Plane near_plane;
  Plane far_plane;
  Plane right_plane;
  Plane left_plane;
  Plane top_plane;
  Plane bottom_plane;
} G_Camera;

#define Noise2D_TableSize 256
typedef struct
{
  f32 v[Noise2D_TableSize];
  u32 perm_x[Noise2D_TableSize];
  u32 perm_y[Noise2D_TableSize];
  PRNG32 prng;
} Noise2D;

#define G_ChunkDims_Z 64
#define G_ChunkDims_Y 16
#define G_ChunkDims_X 64
#define G_ChunkIndex(x,y,z) (((z)*G_ChunkDims_Y*G_ChunkDims_X)+((y)*G_ChunkDims_Y+(x)))
typedef struct
{
  M_Arena *arena;
  G_Block *blocks;
  
  v3f player_p;
  v3f player_requested_move;
  b32 player_can_move;
  f32 player_move_t;
  
  b32 debug_wire_frame;
  G_Camera camera;
  
  Noise2D noise2d;
} G_State;

function void
noise2d_create(Noise2D *noise, u64 seed)
{
  prng32_seed(&noise->prng, seed);
  for (u32 i = 0; i < Noise2D_TableSize; ++i)
  {
    noise->v[i] = prng32_nextf32(&noise->prng);
    noise->perm_x[i] = i;
    noise->perm_y[i] = i;
  }
  
  for (s32 i = Noise2D_TableSize-1; i > 0; --i)
  {
    u32 target = prng32_rangeu32(&noise->prng, 0, i);
    u32 temp = noise->perm_x[i];
    noise->perm_x[i] = noise->perm_x[target];
    noise->perm_x[target] = temp;
    
    target = prng32_rangeu32(&noise->prng, 0, i);
    temp = noise->perm_y[i];
    noise->perm_y[i] = noise->perm_y[target];
    noise->perm_y[target] = temp;
  }
}

function f32
noise2d_evaluate(Noise2D *noise, f32 x, f32 y)
{
  s32 xi = (s32)x;
  s32 yi = (s32)y;
  
  f32 tx = x - xi;
  f32 ty = y - yi;
  
  s32 rx0 = xi & (Noise2D_TableSize - 1);
  s32 rx1 = (rx0 + 1) & (Noise2D_TableSize - 1);
  s32 ry0 = yi & (Noise2D_TableSize - 1);
  s32 ry1 = (ry0 + 1) & (Noise2D_TableSize - 1);
  
#if 0
  f32 c00 = noise->v[ry0 * Noise2D_MaxVertices + rx0];
  f32 c10 = noise->v[ry0 * Noise2D_MaxVertices + rx1];
  f32 c01 = noise->v[ry1 * Noise2D_MaxVertices + rx0];
  f32 c11 = noise->v[ry1 * Noise2D_MaxVertices + rx1];
#else
  f32 c00 = noise->v[noise->perm_y[ry0] ^ noise->perm_x[rx0]];
  f32 c10 = noise->v[noise->perm_y[ry0] ^ noise->perm_x[rx1]];
  f32 c01 = noise->v[noise->perm_y[ry1] ^ noise->perm_x[rx0]];
  f32 c11 = noise->v[noise->perm_y[ry1] ^ noise->perm_x[rx1]];
#endif
  
  tx = tx * tx * (3.0f - 2.0f * tx);
  ty = ty * ty * (3.0f - 2.0f * ty);
  
  f32 A0 = c00 * (1.0f - tx) + c10 * tx;
  f32 A1 = c01 * (1.0f - tx) + c11 * tx;
  f32 B0 = A0 * (1.0f - ty) + A1 * ty;
  
  return(B0);
}

function void
g_cam_init(G_Camera *camera, v3f init_p, f32 camera_aspect)
{
  camera->look_to = init_p;
  camera->near_z = 1.0f;
  camera->far_z = 50.0f;
  camera->fov_rad = DegToRad(66.2f);
  camera->lock_strength = 0.03f;
  camera->aspect = camera_aspect;
  
  camera->debug_lock_strength = 0.15f;
  camera->debug_cam = false;
  camera->debug_cam_p = init_p;
  camera->debug_cam_no = 90;
  camera->debug_cam_yes = 90;
}

f32 noise_map[G_ChunkDims_Z*G_ChunkDims_X];
function void
g_init(G_State *state, M_Arena *arena, f32 camera_aspect)
{
  state->arena = arena;
  u64 block_volume = G_ChunkDims_Z * G_ChunkDims_Y * G_ChunkDims_X;
  state->blocks = M_Arena_PushArray(arena, G_Block, block_volume);
  
  noise2d_create(&state->noise2d, 64);
  f32 max_noise = 0.0f;
  for (u32 y = 0; y < G_ChunkDims_Z; ++y)
  {
    for (u32 x = 0; x < G_ChunkDims_X; ++x)
    {
      f32 frequency = 0.02f;
      f32 lacunarity = 2.2f;
      f32 amplitude = 1;
      f32 amplitude_mult = 0.35f;
      f32 noise_accum = 0.0f;
      for (u32 layer = 0; layer < 5; ++ layer)
      {
        f32 xx = (f32)x*frequency;
        f32 yy = (f32)y*frequency;
        
        noise_accum += noise2d_evaluate(&state->noise2d, xx, yy) * amplitude;
        // turbulence tech
        //noise_accum += fabsf(2*Noise2D_Evaluate(&noise2d, xx, yy)-1) * amplitude;
        
        frequency *= lacunarity;
        amplitude *= amplitude_mult;
      }
      
      if (noise_accum > max_noise)
      {
        max_noise = noise_accum;
      }
      
      noise_map[y * G_ChunkDims_X + x] = noise_accum;
      
      // marble
      //noise_map[y * 512 + x] = (sinf((x + noise_accum * 100.0f) * 2 * 3.14159f / 200.0f) + 1) / 2.0f;
    }
  }
  
  for (u64 block_z = 0; block_z < G_ChunkDims_Z; ++block_z)
  {
    for (u64 block_x = 0; block_x < G_ChunkDims_X; ++block_x)
    {
      f32 x = noise_map[block_z * G_ChunkDims_X + block_x] / max_noise;
      u32 Y = (u64)(x*G_ChunkDims_Y);
      
      if (Y >= G_ChunkDims_Y)
      {
        Y = G_ChunkDims_Y - 1;
      }
      
      for (u64 block_y = 0; block_y < Y; ++block_y)
      {
        G_Block *block = state->blocks + G_ChunkIndex(block_x, block_y, block_z);
        block->flags = G_BlockFlag_Active;
        block->type = (block_y == (Y - 1)) ? G_BlockType_Grass : G_BlockType_Dirt;
      }
    }
  }
  
  state->player_p = v3f_make(0, G_ChunkDims_Y, 0);
  state->player_requested_move = state->player_p;
  state->player_can_move = true;
  state->player_move_t = 0.0f;
  
  g_cam_init(&state->camera, state->player_p, camera_aspect);
}

function void
g_update_camera(G_Camera *cam, OS_Window window, OS_Input *input, v3f look_to_update, f32 game_update_secs)
{
  v3f camera_look, camera_up;
  v3f a;
  
  if (cam->debug_cam)
  {
    POINT middle;
    middle.x = window.client_width / 2;
    middle.y = window.client_height / 2;
    
    POINT middle2 = middle;
    ClientToScreen(window.handle, &middle2);
    SetCursorPos(middle2.x, middle2.y);
    
    f32 camera_sens = 6.0f;
    f32 mouse_delta_x = (f32)((f32)middle.x - input->mouse_x) * camera_sens * game_update_secs;
    f32 mouse_delta_y = (f32)(input->mouse_y - (f32)middle.y) * camera_sens * game_update_secs;
    
    cam->debug_cam_yes += mouse_delta_y;
    cam->debug_cam_no += mouse_delta_x;
    if (cam->debug_cam_yes < -179.0f)
    {
      cam->debug_cam_yes = -179.0f;
    }
    
    if (cam->debug_cam_yes > 179.0f)
    {
      cam->debug_cam_yes = 179.0f;
    }
    
    if (cam->debug_cam_no > 360.0f)
    {
      cam->debug_cam_no = 0.0f;
    }
    
    if (cam->debug_cam_no < 0.0f)
    {
      cam->debug_cam_no = 360.0f;
    } 
    
    f32 yz = DegToRad(cam->debug_cam_yes);
    f32 xz = DegToRad(cam->debug_cam_no);
    f32 y = cosf(yz);
    f32 x = cosf(xz)*sinf(yz);
    f32 z = sinf(yz)*sinf(xz);
    
    cam->look_from.x = cam->look_from.x * (1.0f-cam->debug_lock_strength) + cam->debug_lock_strength*cam->debug_cam_p.x;
    cam->look_from.y = cam->look_from.y * (1.0f-cam->debug_lock_strength) + cam->debug_lock_strength*cam->debug_cam_p.y;
    cam->look_from.z = cam->look_from.z * (1.0f-cam->debug_lock_strength) + cam->debug_lock_strength*cam->debug_cam_p.z;
    
    cam->look_to.x = cam->look_to.x * (1.0f-cam->debug_lock_strength) + cam->debug_lock_strength*(cam->look_from.x + x);
    cam->look_to.y = cam->look_to.y * (1.0f-cam->debug_lock_strength) + cam->debug_lock_strength*(cam->look_from.y + y);
    cam->look_to.z = cam->look_to.z * (1.0f-cam->debug_lock_strength) + cam->debug_lock_strength*(cam->look_from.z + z);
    
    camera_look = v3f_normalize_or_zero(v3f_make(x,y,z));
    camera_up = v3f_make(0, 1, 0);
  }
  else
  {
    cam->look_to.x = cam->look_to.x * (1.0f - cam->lock_strength) + cam->lock_strength * look_to_update.x;
    cam->look_to.y = cam->look_to.y * (1.0f - cam->lock_strength) + cam->lock_strength * look_to_update.y;
    cam->look_to.z = cam->look_to.z * (1.0f - cam->lock_strength) + cam->lock_strength * look_to_update.z;
    
    //v3f desired_look_from = v3f_make(cam->look_to.x + 15, 15, cam->look_to.z - 15);
    cam->look_from = v3f_make(cam->look_to.x + 15, cam->look_to.y + 15, cam->look_to.z - 15);
#if 0
    cam->look_from.x = desired_look_from.x*(1.0f - cam->lock_strength) + cam->lock_strength*desired_look_from.x;
    cam->look_from.y = desired_look_from.y*(1.0f - cam->lock_strength) + cam->lock_strength*desired_look_from.y;
    cam->look_from.z = desired_look_from.z*(1.0f - cam->lock_strength) + cam->lock_strength*desired_look_from.z;
#endif
    
    camera_look = v3f_sub(cam->look_to, cam->look_from);
    camera_up = v3f_make(0, 1, 0);
  }
  
  a = v3f_scale(v3f_dot(camera_up, camera_look) / v3f_dot(camera_look, camera_look), camera_look);
  cam->z = v3f_normalize_or_zero(camera_look);
  cam->y = v3f_sub_and_normalize_or_zero(camera_up, a);
  cam->x = v3f_normalize_or_zero(v3f_cross(cam->y, cam->z));
  
  f32 right = cam->near_z * tanf(cam->fov_rad * 0.5f);
  f32 top = right * cam->aspect;
  
  cam->near_plane = plane_create(v3f_add(cam->look_from, v3f_scale(cam->near_z, cam->z)),
                                 cam->z);
  
  cam->far_plane = plane_create(v3f_add(cam->look_from, v3f_scale(cam->far_z, cam->z)),
                                v3f_additive_inverse(cam->z));
  
  // TODO(cj): These normals are messed up.
  v3f right_n = v3f_cross(cam->y, v3f_add(cam->z, v3f_scale(-right, cam->x)));
  right_n = v3f_normalize_or_zero(right_n);
  cam->right_plane = plane_create(cam->look_from, right_n);
  
  v3f left_n = v3f_cross(v3f_add(cam->z, v3f_scale(right, cam->x)), cam->y);
  left_n = v3f_normalize_or_zero(left_n);
  cam->left_plane = plane_create(cam->look_from, left_n);
  
  v3f top_n = v3f_cross(cam->x, v3f_add(cam->z, v3f_scale(top, cam->y)));
  top_n = v3f_normalize_or_zero(top_n);
  cam->top_plane = plane_create(cam->look_from, top_n);
  
  v3f bottom_n = v3f_cross(v3f_add(cam->z, v3f_scale(-top, cam->y)), cam->x);
  bottom_n = v3f_normalize_or_zero(bottom_n);
  cam->bottom_plane = plane_create(cam->look_from, bottom_n);
  
  if (cam->debug_cam)
  {
    f32 move_units = 5.0f*game_update_secs;
    if (OS_KeyHeld(input, OS_Input_KeyType_W))
    {
      v3f_add_eq(&cam->debug_cam_p, v3f_scale(move_units, cam->z));
    }
    
    if (OS_KeyHeld(input, OS_Input_KeyType_A))
    {
      v3f_sub_eq(&cam->debug_cam_p, v3f_scale(move_units, cam->x));
    }
    
    if (OS_KeyHeld(input, OS_Input_KeyType_S))
    {
      v3f_sub_eq(&cam->debug_cam_p, v3f_scale(move_units, cam->z));
    }
    
    if (OS_KeyHeld(input, OS_Input_KeyType_D))
    {
      v3f_add_eq(&cam->debug_cam_p, v3f_scale(move_units, cam->x));
    }
    
    if (OS_KeyHeld(input, OS_Input_KeyType_E))
    {
      v3f_add_eq(&cam->debug_cam_p, v3f_scale(move_units, cam->y));
    }
    
    if (OS_KeyHeld(input, OS_Input_KeyType_Q))
    {
      v3f_sub_eq(&cam->debug_cam_p, v3f_scale(move_units, cam->y));
    }
  }
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
  
  OS_Window window = {0};
  os_create_window(&window, "Iso Jimmy", 1280, 720);
  
  OS_Input *input = &(window.input);
  
  M_Arena *arena = m_arena_reserve(MB(4));
  R_State renderer_state = {0};
  r_init(&renderer_state, window);
  
  G_State game_state = {0};
  g_init(&game_state, arena,
         (f32)renderer_state.back_buffer_height/(f32)renderer_state.back_buffer_width);
  
  while (true)
  {
    w32_fill_input(&window);
    
    if (OS_KeyReleased(input, OS_Input_KeyType_Escape))
    {
      ExitProcess(0);
    }
    
    if (game_state.player_can_move)
    {
      if (!game_state.camera.debug_cam)
      {
        f32 move_units = 1.0f;
        if (OS_KeyHeld(input, OS_Input_KeyType_W))
        {
          game_state.player_requested_move.z += move_units;
          game_state.player_can_move = false;
        }
        
        if (OS_KeyHeld(input, OS_Input_KeyType_S))
        {
          game_state.player_requested_move.z -= move_units;
          game_state.player_can_move = false;
        }
        
        if (OS_KeyHeld(input, OS_Input_KeyType_A))
        {
          game_state.player_requested_move.x -= move_units;
          game_state.player_can_move = false;
        }
        
        if (OS_KeyHeld(input, OS_Input_KeyType_D))
        {
          game_state.player_requested_move.x += move_units;
          game_state.player_can_move = false;
        }
      }
    }
    else
    {
      if (game_state.player_move_t >= 0.99f)
      {
        game_state.player_p = game_state.player_requested_move;
        game_state.player_move_t = 0.0f;
        game_state.player_can_move = true;
      }
      else
      {
        f32 move_t = game_state.player_move_t;
        game_state.player_p.x = game_state.player_p.x * (1.0f - move_t) + move_t * game_state.player_requested_move.x;
        f32 hop_height = 1.0f / 10.0f;
        game_state.player_p.y = game_state.player_p.y * (1.0f - move_t) + move_t * game_state.player_requested_move.y - hop_height*move_t*move_t + hop_height;
        game_state.player_p.z = game_state.player_p.z * (1.0f - move_t) + move_t * game_state.player_requested_move.z;
        
        game_state.player_move_t += seconds_per_frame*3;
      }
    }
    
    g_update_camera(&game_state.camera, window, input, game_state.player_p, seconds_per_frame);
    
    R_Pass *light_pass = r_acquire_game_with_light_pass(&renderer_state, 
                                                        game_state.camera.look_from,
                                                        m44_perspective_dx11(game_state.camera.aspect, game_state.camera.fov_rad,
                                                                             game_state.camera.near_z, game_state.camera.far_z),
                                                        (m44)
                                                        {
                                                          game_state.camera.x.x, game_state.camera.x.y, game_state.camera.x.z, -v3f_dot(game_state.camera.look_from, game_state.camera.x),
                                                          game_state.camera.y.x, game_state.camera.y.y, game_state.camera.y.z, -v3f_dot(game_state.camera.look_from, game_state.camera.y),
                                                          game_state.camera.z.x, game_state.camera.z.y, game_state.camera.z.z, -v3f_dot(game_state.camera.look_from, game_state.camera.z),
                                                          0.0f, 0.0f, 0.0f, 1.0f,
                                                        },
                                                        game_state.debug_wire_frame);
    
    //R_Light *point_light = r_game_with_light_add_point_light(light_pass, v3f_make(game_state.player_p.x, game_state.player_p.y + 2, game_state.player_p.z), v4f_make(1,1,1,1));
    r_game_with_light_add_directional_light(light_pass, v3f_make(0.0f, -1.0f, 1.0f), v4f_make(0.7f, 0.7f, 0.7f, 1.0f));
    {
      G_Camera cam = game_state.camera;
      for (u64 block_z = 0; block_z < G_ChunkDims_Z; ++block_z)
      {
        for (u64 block_y = 0; block_y < G_ChunkDims_Y; ++block_y)
        {
          for (u64 block_x = 0; block_x < G_ChunkDims_X; ++block_x)
          {
            // TODO: AABB PLANE INTERSECT.
            v3f p3 = v3f_make((f32)block_x, (f32)block_y, (f32)block_z);
            f32 signed_dist_near = (v3f_dot(cam.near_plane.n, p3) - cam.near_plane.d);
            f32 signed_dist_far = (v3f_dot(cam.far_plane.n, p3) - cam.far_plane.d);
            f32 signed_dist_right = (v3f_dot(cam.right_plane.n, p3) - cam.right_plane.d);
            f32 signed_dist_left = (v3f_dot(cam.left_plane.n, p3) - cam.left_plane.d);
            f32 signed_dist_top = (v3f_dot(cam.top_plane.n, p3) - cam.top_plane.d);
            f32 signed_dist_bottom = (v3f_dot(cam.bottom_plane.n, p3) - cam.bottom_plane.d);
            
            f32 radius = -1;
            b32 inside_camera_volume = ((signed_dist_near > radius) &&
                                        (signed_dist_far > radius) && 
                                        (signed_dist_right > radius) &&
                                        (signed_dist_left > radius)&&
                                        (signed_dist_top > radius) &&
                                        (signed_dist_bottom > radius)
                                        );
            
            G_Block *block = game_state.blocks + G_ChunkIndex(block_x, block_y, block_z);
            if ((block->flags & G_BlockFlag_Active) && inside_camera_volume)
            {
              b32 left_block_active = false;
              b32 right_block_active = false;
              b32 top_block_active = false;
              b32 bottom_block_active = false;
              b32 back_block_active = false;
              b32 front_block_active = false;
              G_Block *test_block;
              
              if (block_x > 0)
              {
                test_block = game_state.blocks + G_ChunkIndex(block_x - 1, block_y, block_z);
                left_block_active = !!(test_block->flags&G_BlockFlag_Active);
              }
              
              if ((block_x + 1) < G_ChunkDims_X)
              {
                test_block = game_state.blocks + G_ChunkIndex(block_x + 1, block_y, block_z);
                right_block_active = !!(test_block->flags&G_BlockFlag_Active);
              }
              
              if (block_y > 0)
              {
                test_block = game_state.blocks + G_ChunkIndex(block_x, block_y - 1, block_z);
                bottom_block_active = !!(test_block->flags&G_BlockFlag_Active);
              }
              
              if ((block_y + 1) < G_ChunkDims_Y)
              {
                test_block = game_state.blocks + G_ChunkIndex(block_x, block_y + 1, block_z);
                top_block_active = !!(test_block->flags&G_BlockFlag_Active);
              }
              
              if (block_z > 0)
              {
                test_block = game_state.blocks + G_ChunkIndex(block_x, block_y, block_z - 1);
                back_block_active = !!(test_block->flags&G_BlockFlag_Active);
              }
              
              if ((block_z + 1) < G_ChunkDims_Z)
              {
                test_block = game_state.blocks + G_ChunkIndex(block_x, block_y, block_z + 1);
                front_block_active = !!(test_block->flags&G_BlockFlag_Active);
              }
              
              // if there exist at least one neighboring block that isnt active,
              // then render this block. 
              // We need to do this soon in a granular level, such as plane mesh instead of cube mesh.
              if (!(left_block_active && right_block_active &&
                    top_block_active && bottom_block_active &&
                    back_block_active && front_block_active))
              {
                switch (block->type)
                {
                  case G_BlockType_Grass:
                  {
                    r_game_with_light_add_instance(light_pass,
                                                   p3,
                                                   v3f_make(1.0f, 1.0f, 1.0f),
                                                   RGBA(97, 154, 86, 1.0f));
                  } break;
                  
                  case G_BlockType_Dirt:
                  {
                    r_game_with_light_add_instance(light_pass,
                                                   p3,
                                                   v3f_make(1.0f, 1.0f, 1.0f),
                                                   RGBA(156, 122, 82, 1.0f));
                  } break;
                  
                  InvalidDefaultCase;
                }
              }
            }
          }
        }
      }
    }
    
    r_game_with_light_add_instance(light_pass,
                                   game_state.player_p,
                                   v3f_make(1.0f, 1.0f, 1.0f),
                                   RGBA(94.0f, 68.0f, 121.0f, 1.0f));
    
    
#if 0
    R_Pass *no_light_pass = r_acquire_game_without_light_pass(&renderer_state, 
                                                              m44_perspective_dx11(game_state.camera.aspect, game_state.camera.fov_rad,
                                                                                   game_state.camera.near_z, game_state.camera.far_z),
                                                              (m44)
                                                              {
                                                                game_state.camera.x.x, game_state.camera.x.y, game_state.camera.x.z, -v3f_dot(game_state.camera.look_from, game_state.camera.x),
                                                                game_state.camera.y.x, game_state.camera.y.y, game_state.camera.y.z, -v3f_dot(game_state.camera.look_from, game_state.camera.y),
                                                                game_state.camera.z.x, game_state.camera.z.y, game_state.camera.z.z, -v3f_dot(game_state.camera.look_from, game_state.camera.z),
                                                                0.0f, 0.0f, 0.0f, 1.0f,
                                                              },
                                                              game_state.debug_wire_frame);
    
    
    
    r_game_without_light_add_instance(no_light_pass,
                                      point_light->p,
                                      v3f_make(0.2f, 0.2f, 0.2f),
                                      point_light->colour);
#endif
    
    R_Pass *ui_pass = r_acquire_ui_pass(&renderer_state);
    r_ui_textf(ui_pass, v2f_make(0, 0), v4f_make(1,0,1,1), str8("----------------------------- DEBUG -----------------------------"));
    r_ui_textf(ui_pass, v2f_make(0, 28), v4f_make(1,1,1,1),
               str8("Player World Coord: <%.2f, %.2f, %.2f>"),
               game_state.player_p.x,
               game_state.player_p.y,
               game_state.player_p.z);
    
    r_ui_textf(ui_pass, v2f_make(0, 28*2), v4f_make(1,1,1,1),
               str8("Camera World Coord: <%.2f, %.2f, %.2f>"), 
               game_state.camera.look_from.x,
               game_state.camera.look_from.y,
               game_state.camera.look_from.z);
    
    r_ui_textf(ui_pass, v2f_make(0, 28*3), v4f_make(1,1,1,1),
               str8("Active Blocks: %llu"), 
               light_pass->withlight.instances_count);
    
    {
      String_U8_Const str = str8("Camera Roam (O): %s");
      v2f mouse_p = v2f_make((f32)input->mouse_x, (f32)input->mouse_y);
      v2f text_p = v2f_make(0, 28*4);
      v2f text_dims = r_ui_get_text_dims(ui_pass, str, game_state.camera.debug_cam ? "True" : "False");
      
      f32 max_x = text_p.x + text_dims.x;
      f32 max_y = text_p.y + text_dims.y;
      
      if (OS_KeyReleased(input, OS_Input_KeyType_O))
      {
        game_state.camera.debug_cam = !game_state.camera.debug_cam;
        ShowCursor(!game_state.camera.debug_cam);
      }
      
      if ((mouse_p.x <= max_x) && (mouse_p.x >= text_p.x) &&
          (mouse_p.y <= max_y) && (mouse_p.y >= text_p.y))
      {
        if (OS_ButtonReleased(input, OS_Input_ButtonType_Left))
        {
          game_state.camera.debug_cam = !game_state.camera.debug_cam;
          ShowCursor(!game_state.camera.debug_cam);
        }
        r_ui_textf(ui_pass, text_p, v4f_make(1,0,0,1), str, 
                   game_state.camera.debug_cam ? "True" : "False");
      }
      else
      {
        r_ui_textf(ui_pass, text_p, v4f_make(1,1,1,1), str, 
                   game_state.camera.debug_cam ? "True" : "False");
      }
    }
    
    {
      String_U8_Const str = str8("WireFrame (P): %s");
      v2f mouse_p = v2f_make((f32)input->mouse_x, (f32)input->mouse_y);
      v2f text_p = v2f_make(0, 28*5);
      v2f text_dims = r_ui_get_text_dims(ui_pass, str, game_state.debug_wire_frame ? "True" : "False");
      
      f32 max_x = text_p.x + text_dims.x;
      f32 max_y = text_p.y + text_dims.y;
      
      if (OS_KeyReleased(input, OS_Input_KeyType_P))
      {
        game_state.debug_wire_frame = !game_state.debug_wire_frame;
      }
      
      if ((mouse_p.x <= max_x) && (mouse_p.x >= text_p.x) &&
          (mouse_p.y <= max_y) && (mouse_p.y >= text_p.y))
      {
        if (OS_ButtonReleased(input, OS_Input_ButtonType_Left))
        {
          game_state.debug_wire_frame = !game_state.debug_wire_frame;
        }
        r_ui_textf(ui_pass, text_p, v4f_make(1,0,0,1), str, 
                   game_state.debug_wire_frame ? "True" : "False");
      }
      else
      {
        r_ui_textf(ui_pass, text_p, v4f_make(1,1,1,1), str, 
                   game_state.debug_wire_frame ? "True" : "False");
      }
    }
    
    r_submit(&renderer_state);
  }
  
  ExitProcess(0);
}