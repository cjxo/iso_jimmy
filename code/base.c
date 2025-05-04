function M_Arena *
m_arena_reserve(u64 cap)
{
  M_Arena *result = 0;
  void *block = VirtualAlloc(0, cap, MEM_RESERVE, PAGE_NOACCESS);
  Assert(block);
  
  VirtualAlloc(block, sizeof(M_Arena), MEM_COMMIT, PAGE_READWRITE);
  result = block;
  result->base = block;
  result->capacity = cap;
  result->stack_ptr = sizeof(M_Arena);
  result->commit_ptr = sizeof(M_Arena);
  return(result);
}

function void *
m_arena_push(M_Arena *arena, u64 size)
{
  void *result = 0;
  size = Align(size, 16);
  u64 desired_stack_ptr = arena->stack_ptr + size;
  if (desired_stack_ptr <= arena->capacity)
  {
    u64 desired_commit_ptr = arena->commit_ptr;
    void *result_block_on_success = arena->base + arena->stack_ptr;
    if (desired_stack_ptr >= arena->commit_ptr)
    {
      u64 new_commit_ptr = Align(desired_stack_ptr, M_Arena_DefaultCommit());
      u64 new_commit_ptr_clamped = Minimum(new_commit_ptr, arena->capacity);
      
      if (new_commit_ptr_clamped > arena->commit_ptr)
      {
        void *meow = VirtualAlloc(arena->base + arena->commit_ptr,
                                  new_commit_ptr_clamped - arena->commit_ptr,
                                  MEM_COMMIT, PAGE_READWRITE);
        
        Assert(meow);
        
        desired_commit_ptr = new_commit_ptr_clamped;
      }
    }
    
    if (desired_stack_ptr < desired_commit_ptr)
    {
      arena->stack_ptr = desired_stack_ptr;
      arena->commit_ptr = desired_commit_ptr;
      result = result_block_on_success;
    }
  }
  
  Assert(result);
  return(result);
}

function void
m_arena_pop(M_Arena *arena, u64 size)
{
  size = Align(size, 16);
  Assert(size <= (arena->stack_ptr + sizeof(M_Arena)));
  arena->stack_ptr -= size;
  
  u64 new_commit_ptr = Align(arena->stack_ptr, M_Arena_DefaultCommit());
  if (new_commit_ptr < arena->commit_ptr)
  {
    BOOL meow = VirtualFree(arena->base + new_commit_ptr, arena->commit_ptr - new_commit_ptr, MEM_DECOMMIT);
    Assert(meow != 0);
    arena->commit_ptr = new_commit_ptr;
  }
}

inline function void
m_arena_clear(M_Arena *arena)
{
  u64 arena_size = sizeof(M_Arena);
  if (arena->stack_ptr > arena_size)
  {
    m_arena_pop(arena, arena->stack_ptr - arena_size);
  }
}

#define M_MaxTransientArena() 4
global_variable thread_variable M_Arena *g_transient_arena[M_MaxTransientArena()];

function M_Arena *
m_get_for_transient_purposes(M_Arena **conflict, u64 count)
{
  if (!g_transient_arena[0])
  {
    for (u64 arena_idx = 0; arena_idx < M_MaxTransientArena(); ++arena_idx)
    {
      g_transient_arena[arena_idx] = m_arena_reserve(MB(4));
    }
  }
  
  M_Arena *result = 0;
  for (u64 arena_idx = 0; arena_idx < M_MaxTransientArena(); ++arena_idx)
  {
    b32 accept_selection = true;
    
    for (u64 conflict_idx = 0; conflict_idx < count; ++conflict_idx)
    {
      if (g_transient_arena[arena_idx] == conflict[conflict_idx])
      {
        accept_selection = false;
        break;
      }
    }
    
    if (accept_selection)
    {
      result = g_transient_arena[arena_idx];
      break;
    }
  }
  
  Assert(result);
  return(result);
}

inline function M_Temp
m_temp_begin(M_Arena *arena)
{
  M_Temp result;
  result.arena =arena;
  result.stack_ptr = arena->stack_ptr;
  return(result);
}

inline function void
m_temp_end(M_Temp temp)
{
  Assert(temp.arena);
  Assert(temp.arena->stack_ptr >= temp.stack_ptr);
  if (temp.arena->stack_ptr > temp.stack_ptr)
  {
    m_arena_pop(temp.arena, temp.arena->stack_ptr - temp.stack_ptr);
  }
}

function String_U8_Const
str8_format_va(M_Arena *arena, String_U8_Const str, va_list args0)
{
  va_list args1;
  va_copy(args1, args0);
  
  s32 total_chars = vsnprintf(0, 0, (char *)str.s, args1);
  String_U8_Const result = {0};
  if (total_chars)
  {
    result.count = (u64)total_chars;
    result.capacity = result.count;
    result.s = M_Arena_PushArray(arena, u8, result.capacity + 1);
    vsnprintf((char *)result.s, result.count + 1, (char *)str.s, args1);
    
    result.s[result.count] = 0;
  }
  
  va_end(args1);
  return(result);
}