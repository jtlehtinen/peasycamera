#pragma comment(lib, "opengl32.lib")

#include <Windows.h>
#include "gl3w.h"
#include <assert.h>
#include <stdint.h>

void Win32ToggleFullscreen(HWND window, WINDOWPLACEMENT& placement) {
   // Thanks Raymond Chen: https://blogs.msdn.microsoft.com/oldnewthing/20100412-00/?p=14353
   DWORD winStyle = GetWindowLong(window, GWL_STYLE);
   if (winStyle & WS_OVERLAPPEDWINDOW) {
      MONITORINFO monitorInfo = {sizeof(MONITORINFO)};
      if (GetWindowPlacement(window, &placement) && GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitorInfo)) {
         SetWindowLong(window, GWL_STYLE, winStyle & ~WS_OVERLAPPEDWINDOW);
         SetWindowPos(window, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
      }
   } else {
      SetWindowLong(window, GWL_STYLE, winStyle | WS_OVERLAPPEDWINDOW);
      SetWindowPlacement(window, &placement);
      SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
   }
}

LRESULT CALLBACK Win32WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
   LRESULT result = 0;

   switch (message) {
      case WM_CLOSE:
      case WM_DESTROY:
         PostQuitMessage(0);
         break;

      case WM_SYSKEYDOWN:
      case WM_KEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYUP: {
         assert(!"Nondispatched message ended up here. Windows? Why?");
         break;
      }

      default:
         result = DefWindowProcA(window, message, wparam, lparam);
         break;
   }

   return result;
}

HWND Win32CreateWindow(const char* windowTitle, int width, int height) {
   WNDCLASSA wc = { };
   wc.style = CS_OWNDC;
   wc.hInstance = GetModuleHandleA(nullptr);
   wc.lpfnWndProc = Win32WindowProc;
   wc.lpszClassName = "Class";
   RegisterClassA(&wc);

   RECT wr = {0, 0, width, height};
   constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW;
   AdjustWindowRect(&wr, windowStyle, FALSE);

   HWND window = CreateWindowExA(0, wc.lpszClassName, "peasycamera demo", windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);

   return window;
}

HGLRC Win32CreateGLContext(HDC dc) {
   PIXELFORMATDESCRIPTOR pfd = { };
   pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
   pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.cColorBits = 24;
   pfd.cAlphaBits = 8;

   int pixelFormatIndex = ChoosePixelFormat(dc, &pfd);
   assert(pixelFormatIndex != 0);

   DescribePixelFormat(dc, pixelFormatIndex, sizeof(pfd), &pfd);
   SetPixelFormat(dc, pixelFormatIndex, &pfd);

   HGLRC ctx = wglCreateContext(dc);

   return ctx;
}

bool Win32MessagePump(HWND window, WINDOWPLACEMENT& wp) {
   bool ok = true;

   MSG msg = { };
   while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
      switch (msg.message) {
         case WM_SYSKEYDOWN:
         case WM_KEYDOWN:
         case WM_SYSKEYUP:
         case WM_KEYUP: {
            const uint32_t vkCode = uint32_t(msg.wParam);
            const bool wasDown = ((msg.lParam & (1 << 30)) != 0);
            const bool isDown = ((msg.lParam & (1 << 31)) == 0);
            const bool altDown = (msg.lParam & (1 << 29)) != 0;

            if (isDown != wasDown) {
               if (isDown && altDown && vkCode == VK_RETURN) {
                  Win32ToggleFullscreen(window, wp);
               }
            }
            break;
         }

         case WM_QUIT:
            ok = false;
            break;

         default:
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            break;
      }
   }

   return ok;
}

int __stdcall WinMain(HINSTANCE instance, HINSTANCE ignored, LPSTR cmdLine, int showCode) {
   WINDOWPLACEMENT wp = { };
   wp.length = sizeof(wp);

   HWND window = Win32CreateWindow("peasycamera demo", 1280, 720);
   assert(window);

   HDC dc = GetDC(window);
   assert(dc);

   HGLRC ctx = Win32CreateGLContext(dc);
   assert(ctx);

   wglMakeCurrent(dc, ctx);
   gl3wInit();

   ShowWindow(window, showCode);

   int majorVersion = 0;
   int minorVersion = 0;
   glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
   glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
   assert((majorVersion >= 4 && minorVersion >= 5) || majorVersion > 4);

   while (Win32MessagePump(window, wp)) {
      const float clearColor[] = {0.0f, 0.3f, 0.5f, 1.0f};
      glClearBufferfv(GL_COLOR, 0, clearColor);

      SwapBuffers(dc);
   }

   return 0;
}
