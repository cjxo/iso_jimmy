#ifndef MY_MATH_H
#define MY_MATH_H

#define DegToRad(deg) (0.01745329251f*(f32)(deg))
#define RadToDeg(rad) (57.2957795131f*(f32)(rad))

inline function b32 close_enough_zero(f32 x, f32 epsilon);

typedef union
{
  struct
  {
    f32 x, y;
  };
  f32 v[2];
} v2f;

typedef union
{
  struct
  {
    f32 x, y, z;
  };
  struct
  {
    v2f xy;
    f32 __unused_a;
  };
  struct
  {
    f32 __unused_b;
    v2f yz;
  };
  f32 v[3];
} v3f;

inline function v3f v3f_make(f32 x, f32 y, f32 z);
inline function v3f v3f_sub(v3f a, v3f b);
inline function v3f v3f_add(v3f a, v3f b);
inline function v3f v3f_sub_and_normalize_or_zero(v3f a, v3f b);
inline function v3f v3f_normalize_or_zero(v3f a);
inline function f32 v3f_dot(v3f a, v3f b);
inline function v3f v3f_additive_inverse(v3f a);

typedef union
{
  struct
  {
    f32 x, y, z, w;
  };
  struct
  {
    f32 r, g, b, a;
  };
  struct
  {
    v3f xyz;
    f32 __unused_a;
  };
  struct
  {
    f32 __unused_b;
    v3f yzw;
  };
  struct
  {
    v2f xz;
    v2f yz;
  };
  f32 v[4];
} v4f;

#define RGBA(r,g,b,a) v4f_make((f32)(r)/255.0f,(f32)(g)/255.0f,(f32)(b)/255.0f,(a))
inline function v4f v4f_make(f32 x, f32 y, f32 z, f32 w);

typedef union
{
  v4f rows[4];
  f32 m[4][4];
} m44;

// NOTE(cj): So... for dx11, we convert from view frustum
// to x, y, z limits to [-1,1], [-1,1], [0, 1], respectively.
inline function m44 m44_perspective_dx11(f32 height_over_width_aspect, f32 fov_rad, f32 z_near, f32 z_far);
inline function m44 m44_identity(void);

typedef struct
{
  v3f n;
  f32 d; // d = (ax_0 + by_0 + cz_0)
} Plane;

inline function Plane plane_create(v3f p, v3f n);

#endif
