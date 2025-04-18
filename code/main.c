#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#if defined(ISO_DEBUG)
# include <dxgidebug.h>
#endif

#include "base.h"
#include "base.c"

function LRESULT __stdcall
w32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;
  
  switch (message)
  {
    case WM_CLOSE:
    {
      DestroyWindow(window);
    } break;
    
    case WM_DESTROY:
    {
      ExitProcess(0);
    } break;
    
    default:
    {
      result = DefWindowProc(window, message, wparam, lparam);
    } break;
  }
  
  return(result);
}

typedef struct
{
  HWND handle;
  s32 client_width, client_height;
} W32_Window;

typedef struct
{
  ID3D11Device *device;
  ID3D11DeviceContext *device_context;
  IDXGISwapChain1 *swap_chain;
  ID3D11RenderTargetView *back_buffer_rtv;
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

function void
dx11_create_swap_chain(R_State *renderer_state, W32_Window window)
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
r_init(R_State *renderer_state, W32_Window window)
{
  dx11_create_device(renderer_state);
  dx11_create_swap_chain(renderer_state, window);
}

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmd, int nShowCmd)
{
  (void)hInstance;
  (void)hPrevInstance;
  (void)lpCmd;
  (void)nShowCmd;
  
  WNDCLASSEXA wnd_class =
  {
    .cbSize = sizeof(WNDCLASSEX),
    .style = CS_OWNDC,
    .lpfnWndProc = w32_window_proc,
    .cbClsExtra = 0,
    .cbWndExtra = 0,
    .hInstance = hInstance,
    .hIcon = LoadIconA(0, IDI_APPLICATION),
    .hCursor = LoadCursorA(0, IDC_ARROW),
    .hbrBackground = GetStockObject(BLACK_BRUSH),
    .lpszMenuName = 0,
    .lpszClassName = "iso_jimmy_class",
    .hIconSm = 0,
  };
  
  if (RegisterClassExA(&wnd_class))
  {
    RECT client_rect;
    client_rect.left = 0;
    client_rect.top = 0;
    client_rect.right = 1280;
    client_rect.bottom = 720;
    {
      b32 adjusted = AdjustWindowRectEx(&client_rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_NOREDIRECTIONBITMAP);
      Assert(adjusted != 0);
    }
    
    s32 window_width = client_rect.right - client_rect.left;
    s32 window_height = client_rect.bottom - client_rect.top;
    
    W32_Window window = 
    {
      .handle = CreateWindowExA(WS_EX_NOREDIRECTIONBITMAP, wnd_class.lpszClassName, "Iso Jimmy",
                                WS_OVERLAPPEDWINDOW, 0, 0, window_width, window_height, 0, 0, hInstance, 0),
      .client_width = 1280,
      .client_height = 720,
    };
    
    if (IsWindow(window.handle))
    {
      ShowWindow(window.handle, SW_SHOW);
      R_State renderer_state;
      r_init(&renderer_state, window);
      
      while (true)
      {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
          TranslateMessage(&message);
          DispatchMessage(&message);
        }
        
        f32 clear_colour[] = { 0.2f, 0.3f, 0.3f, 1.0f };
        ID3D11DeviceContext_ClearRenderTargetView(renderer_state.device_context, renderer_state.back_buffer_rtv, clear_colour);
        IDXGISwapChain_Present(renderer_state.swap_chain, 1, 0);
      }
    }
  }
  else
  {
    Assert(!"TODO: Logging");
  }
  
  ExitProcess(0);
}