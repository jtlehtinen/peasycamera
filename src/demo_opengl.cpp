#pragma comment(lib, "opengl32.lib")

#include "peasycamera.h"

#include <Windows.h>
#include "gl3w.h"
#include <assert.h>
#include <stdint.h>
#include <DirectXMath.h> // @TODO: temporary include to make progress

struct Int2 {
   int x, y;
};

Int2 Win32GetClientAreaSize(HWND window) {
   RECT r;
   GetClientRect(window, &r);
   return {r.right - r.left, r.bottom - r.top};
}

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

int64_t Win32GetPerfCounter() {
   LARGE_INTEGER li;
   QueryPerformanceCounter(&li);
   return li.QuadPart;
}

int64_t Win32GetPerfFrequency() {
   LARGE_INTEGER li;
   QueryPerformanceFrequency(&li);
   return li.QuadPart;
}

const char* vertexShaderSource = R"(
#version 450 core

out VS_OUT {
   vec3 color;
} vout;

uniform mat4 u_transform;

vec3 cube_vertex(int vertex_id) {
   int b = 1 << vertex_id;
   return vec3(float((0x287a & b) != 0), float((0x02af & b) != 0), float((0x31e3 & b) != 0)) - vec3(0.5);
}

void main() {
   vec3 position = cube_vertex(gl_VertexID);
   gl_Position = u_transform * vec4(position, 1.0);

   vout.color = position + vec3(0.5);
}
)";

const char* fragmentShaderSource = R"(
#version 450 core

in VS_OUT {
   vec3 color;
} fin;

out vec4 o_color;

void main() {
   o_color = vec4(fin.color, 1.0);
}
)";

uint32_t CreateShader(const char* source, uint32_t shaderType) {
   uint32_t result = glCreateShader(shaderType);
   glShaderSource(result, 1, &source, nullptr);
   glCompileShader(result);

   int status = 0;
   glGetShaderiv(result, GL_COMPILE_STATUS, &status);
   assert(status == GL_TRUE);

   return result;
}

uint32_t CreateShaderProgram(uint32_t vertexShader, uint32_t fragmentShader) {
   uint32_t result = glCreateProgram();
   glAttachShader(result, vertexShader);
   glAttachShader(result, fragmentShader);
   glLinkProgram(result);

   int status = 0;
   glGetProgramiv(result, GL_LINK_STATUS, &status);
   assert(status == GL_TRUE);

   return result;
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

   const uint32_t vs = CreateShader(vertexShaderSource, GL_VERTEX_SHADER);
   const uint32_t fs = CreateShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
   const uint32_t program = CreateShaderProgram(vs, fs);
   glDeleteShader(vs);
   glDeleteShader(fs);

   uint32_t vao = 0;
   glCreateVertexArrays(1, &vao);

   glEnable(GL_DEPTH_TEST);

   const int64_t counterFrequency = Win32GetPerfFrequency();
   int64_t lastCounter = Win32GetPerfCounter();

   while (Win32MessagePump(window, wp)) {
      const int64_t currentCounter = Win32GetPerfCounter();
      float deltaTime = float(double(currentCounter - lastCounter) / double(counterFrequency));
      lastCounter = currentCounter;

      const float clearColor[] = {0.0f, 0.3f, 0.5f, 1.0f};
      glClearBufferfv(GL_COLOR, 0, clearColor);

      const float one = 1.0f;
      glClearBufferfv(GL_DEPTH, 0, &one);

      const Int2 sz = Win32GetClientAreaSize(window);
      glViewport(0, 0, sz.x, sz.y);

      static float angle = 0.0f;
      angle += DirectX::XM_PI * deltaTime;
      DirectX::XMMATRIX localToWorldMatrix = DirectX::XMMatrixRotationAxis({0.0f, 1.0f, 0.0f}, angle);
      DirectX::XMMATRIX worldToViewMatrix = DirectX::XMMatrixLookAtRH({2.0f, 2.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
      DirectX::XMMATRIX viewToProjectionMatrix = DirectX::XMMatrixPerspectiveFovRH(0.8f, float(sz.x) / float(sz.y), 0.1f, 100.0f);
      DirectX::XMMATRIX localToProjectionMatrix = localToWorldMatrix * worldToViewMatrix * viewToProjectionMatrix;

      DirectX::XMFLOAT4X4 uploadProjectionFromLocalMatrix;
      DirectX::XMStoreFloat4x4(&uploadProjectionFromLocalMatrix, localToProjectionMatrix);

      glUseProgram(program);

      glUniformMatrix4fv(glGetUniformLocation(program, "u_transform"), 1, GL_FALSE, reinterpret_cast<GLfloat*>(&uploadProjectionFromLocalMatrix));

      glBindVertexArray(vao);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
      glBindVertexArray(0);
      glUseProgram(0);

      SwapBuffers(dc);
   }

   return 0;
}
