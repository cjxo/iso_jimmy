#ifndef BASE_H
#define BASE_H
#include <stdint.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef s32 b32;
typedef float f32;
#define true 1
#define false 0
#define null 0

#define function static
#define global_variable static
#define local_variable static
#define thread_variable __declspec(thread)
#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))

#define Stmnt(s) do{s}while(0)
#if defined (ISO_DEBUG)
# define Assert(c) Stmnt(if(!(c)){__debugbreak();})
#else
# define Assert(c) (void)(c)
#endif

#define Minimum(a,b) (((a)<(b))?(a):(b))
#define Maximum(a,b) (((a)>(b))?(a):(b))
#define KB(v) ((u64)v*1024llu)
#define MB(v) (KB(v)*1024llu)
#define GB(v) (MB(v)*1024llu)
#define Align(a,b) ((a)+((b)-1))&(~((b)-1))

#define SLLPushBack_N(f,l,n,next) ((((f)==0)?((f)=(l)=(n)):((l)->next=(n),(l)=(n))),((n)->next=0))
#define SLLPushBack(f,l,n) SLLPushBack_N(f,l,n,next)

#define InvalidDefaultCase default:{Assert(0);}break

#define M_Arena_DefaultCommit() KB(8)
typedef struct
{
  u8 *base;
  u64 capacity;
  u64 stack_ptr;
  u64 commit_ptr;
} M_Arena;

typedef struct
{
  M_Arena *arena;
  u64 stack_ptr;
} M_Temp;

#define M_Arena_PushArray(a,T,c) (T*)m_arena_push((a),sizeof(T)*(c))
#define M_Arena_PushStruct(a,T) M_Arena_PushArray(a,T,1)
function M_Arena *   m_arena_reserve(u64 cap);
function void *      m_arena_push(M_Arena *arena, u64 size);
function void        m_arena_pop(M_Arena *arena, u64 size);
inline function void m_arena_clear(M_Arena *arena);
function M_Arena *   m_get_for_transient_purposes(M_Arena **conflict, u64 count);

#define str8(s) (String_Const_U8){(s),sizeof(s)-1,sizeof(s)-1}
typedef struct
{
  u8 *s;
  u64 count;
  u64 capacity;
} String_Const_U8;

#endif //BASE_H
