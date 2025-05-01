function OS_Input_KeyType
w32_map_wparam_to_keytype(WPARAM wParam)
{
  OS_Input_KeyType key = OS_Input_KeyType_Count;
  switch (wParam)
  {
    case VK_ESCAPE:
    {
      key = OS_Input_KeyType_Escape;
    } break;
    
    case VK_SPACE:
    {
      key = OS_Input_KeyType_Space;
    } break;
    
    default:
    {
      if ((wParam >= 'A') && (wParam <= 'Z'))
      {
        key = (OS_Input_KeyType)(OS_Input_KeyType_A + (wParam - 'A'));
      }
      else if ((wParam >= '0') && (wParam <= '9'))
      {
        key = (OS_Input_KeyType)(OS_Input_KeyType_0 + (wParam - '0'));
      }
    } break;
  }
  
  return(key);
}

function LRESULT
w32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;
  
  if (message == WM_NCCREATE)
  {
    SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)(((CREATESTRUCT *)lparam)->lpCreateParams));
    return DefWindowProc(window, message, wparam, lparam);
  }
  
  OS_Window *user_window = (OS_Window *)GetWindowLongPtrA(window, GWLP_USERDATA);
  if (!user_window)
  {
    return DefWindowProc(window, message, wparam, lparam);
  }
  
  switch (message)
  {
    case WM_CLOSE:
    {
      DestroyWindow(window);
    } break;
    
    case WM_DESTROY:
    {
      PostQuitMessage(0);
    } break;
    
    case WM_KEYDOWN:
    {
      OS_Input_KeyType key = w32_map_wparam_to_keytype(wparam);
      if (key != OS_Input_KeyType_Count)
      {
        user_window->input.key[key] |= OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Held;
      }
    } break;
    
    case WM_KEYUP:
    {
      OS_Input_KeyType key = w32_map_wparam_to_keytype(wparam);
      if (key != OS_Input_KeyType_Count)
      {
        user_window->input.key[key] &= ~(OS_Input_InteractFlag_Held);
        user_window->input.key[key] |= OS_Input_InteractFlag_Released;
      }
    } break;
    
    case WM_LBUTTONDOWN:
    {
      user_window->input.button[OS_Input_ButtonType_Left] |= OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Held;
    } break;
    
    case WM_LBUTTONUP:
    {
      user_window->input.button[OS_Input_ButtonType_Left] |= OS_Input_InteractFlag_Released;
      user_window->input.button[OS_Input_ButtonType_Left] &= ~OS_Input_InteractFlag_Held;
    } break;
    
    case WM_RBUTTONDOWN:
    {
      user_window->input.button[OS_Input_ButtonType_Right] |= OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Held;
    } break;
    
    case WM_RBUTTONUP:
    {
      user_window->input.button[OS_Input_ButtonType_Right] |= OS_Input_InteractFlag_Released;
      user_window->input.button[OS_Input_ButtonType_Right] &= ~OS_Input_InteractFlag_Held;
    } break;
    
    default:
    {
      result = DefWindowProc(window, message, wparam, lparam);
    } break;
  }
  
  return(result);
}

function void
os_create_window(OS_Window *window, char *name, s32 client_width, s32 client_height)
{
  WNDCLASS wnd_class =
  {
    .style = 0,
    .lpfnWndProc = &w32_window_proc,
    .cbClsExtra = 0,
    .cbWndExtra = 0,
    .hInstance = GetModuleHandleA(0),
    .hIcon = LoadIconA(0, IDI_APPLICATION),
    .hCursor = LoadCursorA(0, IDC_ARROW),
    .hbrBackground = GetStockObject(BLACK_BRUSH),
    .lpszMenuName = 0,
    .lpszClassName = "Game Project",
  };
  if (RegisterClassA(&wnd_class))
  {
    RECT client_rect =
    {
      .left = 0,
      .top = 0,
      .right = client_width,
      .bottom = client_height,
    };
    
    AdjustWindowRect(&client_rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    window->handle = CreateWindowExA(WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP, wnd_class.lpszClassName, name, WS_OVERLAPPEDWINDOW,
                                     0, 0, client_rect.right - client_rect.left, client_rect.bottom - client_rect.top,
                                     0, 0, wnd_class.hInstance, window);
    if (IsWindow(window->handle))
    {
      ShowWindow(window->handle, SW_SHOW);
      window->client_width = client_width;
      window->client_height = client_height;
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
w32_fill_input(OS_Window *window)
{
  OS_Input *input = &window->input;
  for (u32 key = 0; key < OS_Input_KeyType_Count; ++key)
  {
    input->key[key] &= ~(OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Released);
  }
  
  for (u32 button = 0; button < OS_Input_ButtonType_Count; ++button)
  {
    input->button[button] &= ~(OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Released);
  }
  
  MSG message;
  while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE) != 0)
  {
    switch (message.message)
    {
      case WM_QUIT:
      {
        ExitProcess(0);
      } break;
      
      default:
      {
        TranslateMessage(&message);
        DispatchMessage(&message);
      } break;
    }
  }
  
  //https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getkeystate
  SHORT lshift_state = GetKeyState(VK_LSHIFT);
  if (lshift_state & 0x8000)
  {
    window->input.key[OS_Input_KeyType_LShift] |= OS_Input_InteractFlag_Pressed | OS_Input_InteractFlag_Held;
  }
  else
  {
    window->input.key[OS_Input_KeyType_LShift] &= ~(OS_Input_InteractFlag_Held);
    window->input.key[OS_Input_KeyType_LShift] |= OS_Input_InteractFlag_Released;
  }
}