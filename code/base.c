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
    void *result_block_on_success = 0;
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
      g_transient_arena[arena_idx] = m_arena_reserve(KB(512));
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