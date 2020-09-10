#pragma comment(lib, "opengl32.lib")

#include <Windows.h>
#include "gl3w.h"
#include <assert.h>

LRESULT CALLBACK Win32WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
   LRESULT result = 0;

   switch (message) {
      case WM_CLOSE:
      case WM_DESTROY:
         PostQuitMessage(0);
         break;

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

int __stdcall WinMain(HINSTANCE instance, HINSTANCE ignored, LPSTR cmdLine, int showCode) {
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

   MSG msg = { };
   for (;;) {
      if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
         if (msg.message == WM_QUIT) {
            break;
         }

         TranslateMessage(&msg);
         DispatchMessageA(&msg);
      } else {
         glClearColor(0.0f, 0.3f, 0.5f, 1.0f);
         glClear(GL_COLOR_BUFFER_BIT);

         SwapBuffers(dc);
      }
   }

   return 0;
}
