inline function b32
close_enough_zero(f32 x, f32 epsilon)
{
  b32 result = -epsilon < x;
  result = result && (x < epsilon);
  return(result);
}

inline v2f
v2f_make(f32 x, f32 y)
{
  v2f result;
  result.x = x;
  result.y = y;
  return(result);
}

inline function v3f
v3f_make(f32 x, f32 y, f32 z)
{
  v3f result;
  result.x = x;
  result.y = y;
  result.z = z;
  return(result);
}

inline function v3f
v3f_sub(v3f a, v3f b)
{
  v3f result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;
  return(result);
}

inline function v3f
v3f_add(v3f a, v3f b)
{
  v3f result;
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  result.z = a.z + b.z;
  return(result);
}

inline function v3f
v3f_sub_and_normalize_or_zero(v3f a, v3f b)
{
  v3f result;
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  result.z = a.z - b.z;
  
  f32 length = sqrtf(result.x*result.x + result.y*result.y + result.z*result.z);
  if (!close_enough_zero(length, 0.001f))
  {
    result.x /= length;
    result.y /= length;
    result.z /= length;
  }
  else
  {
    result.x = result.y = result.z = 0.0f;
  }
  
  return(result);
}

inline function f32
v3f_dot(v3f a, v3f b)
{
  f32 result = a.x*b.x + a.y*b.y + a.z*b.z;
  return(result);
}

inline function v3f
v3f_cross(v3f a, v3f b)
{
  v3f result;
  result.x = a.y*b.z - a.z*b.y;
  result.y = a.z*b.x - a.x*b.z;
  result.z = a.x*b.y - a.y*b.x;
  return(result);
}

inline function v3f
v3f_normalize_or_zero(v3f a)
{
  v3f result;
  result.x = a.x;
  result.y = a.y;
  result.z = a.z;
  
  f32 length = sqrtf(result.x*result.x + result.y*result.y + result.z*result.z);
  if (!close_enough_zero(length, 0.001f))
  {
    result.x /= length;
    result.y /= length;
    result.z /= length;
  }
  else
  {
    result.x = result.y = result.z = 0.0f;
  }
  
  return(result);
}

inline function v3f
v3f_scale(f32 x, v3f a)
{
  v3f result;
  result.x = x*a.x;
  result.y = x*a.y;
  result.z = x*a.z;
  return(result);
}

inline function v3f
v3f_additive_inverse(v3f a)
{
  v3f result;
  result.x = -a.x;
  result.y = -a.y;
  result.z = -a.z;
  return(result);
}

inline function v4f
v4f_make(f32 x, f32 y, f32 z, f32 w)
{
  v4f result;
  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;
  return(result);
}

inline function m44
m44_perspective_dx11(f32 height_over_width_aspect, f32 fov_rad, f32 z_near, f32 z_far)
{
  f32 right = z_near * tanf(fov_rad * 0.5f);
  f32 left = -right;
  f32 top = right * height_over_width_aspect;
  f32 bottom = -top;
  
  f32 A = z_far / (z_far - z_near);
  f32 B = -A * z_near;
  
  f32 two_z_near = z_near*2.0f;
  f32 r_m_l = right - left;
  f32 t_m_b = top - bottom;
  
  m44 result;
  result.rows[0] = v4f_make(two_z_near/r_m_l, 0.0f, (right+left)/(r_m_l), 0.0f);
  result.rows[1] = v4f_make(0.0f, two_z_near/t_m_b, (top+bottom)/(t_m_b), 0.0f);
  result.rows[2] = v4f_make(0.0f, 0.0f, A, B);
  result.rows[3] = v4f_make(0.0f, 0.0f, 1.0f, 0.0f);
  return(result);
}

inline static m44
m44_make_orthographic_z01(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane)
{
  f32 rml = right - left;
  f32 tmb = top - bottom;
  
  m44 result =
  {
    2.0f / rml,        0.0f,      0.0f,                             -(right + left) / rml,
    0.0f,        2.0f / tmb,      0.0f,                             -(top + bottom) / tmb,
    0.0f,              0.0f,      1.0f / (far_plane - near_plane),  -near_plane / (far_plane - near_plane),
    0.0f,              0.0f,      0.0f,                             1.0f,
  };
  return(result);
}

inline function m44
m44_identity(void)
{
  m44 result = {0};
  result.m[0][0] = result.m[1][1] = result.m[2][2] = result.m[3][3] = 1.0f;
  return(result);
}

inline function Plane
plane_create(v3f p, v3f n)
{
  Plane result;
  result.n = n;
  result.d = v3f_dot(n, p);
  return(result);
}