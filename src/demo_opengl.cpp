#pragma comment(lib, "opengl32.lib")

#include "peasycamera.h"

#include <Windows.h>
#include <windowsx.h> // for GET_X_LPARAM and GET_Y_LPARAM
#include <DirectXMath.h>
#include "gl3w.h"
#include <assert.h>
#include <stdint.h>

struct Int2 {
   int x, y;
};

struct DigitalButton {
   bool m_down;
   bool m_press;
   bool m_release;

   void Update(bool down) {
      m_press = false;
      m_release = false;

      if (m_down != down) {
         m_press = down;
         m_release = !down;
         m_down = down;
      }
   }
};

struct Mouse {
   Int2 position;
   Int2 positionDelta;
   int wheelDelta;

   DigitalButton lmb;
   DigitalButton mmb;
   DigitalButton rmb;
};

struct Keyboard {
   DigitalButton shift;
   DigitalButton q;
   DigitalButton w;
   DigitalButton e;
   DigitalButton r;
};

struct Win32State {
   WINDOWPLACEMENT windowPlacement;
   HWND window;
   HDC dc;
   HGLRC ctx;

   Mouse mouse;
   Keyboard keyboard;
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

      case WM_MOUSEMOVE:
      case WM_MOUSEWHEEL:
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

bool Win32MessagePump(Win32State& state) {
   state.mouse.wheelDelta = 0;
   state.mouse.positionDelta = { };
   state.mouse.lmb.Update((GetKeyState(VK_LBUTTON) & (1 << 15)) != 0);
   state.mouse.mmb.Update((GetKeyState(VK_MBUTTON) & (1 << 15)) != 0);
   state.mouse.rmb.Update((GetKeyState(VK_RBUTTON) & (1 << 15)) != 0);
   
   state.keyboard.shift.Update(((GetKeyState(VK_LSHIFT) & (1 << 15)) != 0) || ((GetKeyState(VK_RSHIFT) & (1 << 15)) != 0));
   state.keyboard.q.Update((GetKeyState('Q') & (1 << 15)) != 0);
   state.keyboard.w.Update((GetKeyState('W') & (1 << 15)) != 0);
   state.keyboard.e.Update((GetKeyState('E') & (1 << 15)) != 0);
   state.keyboard.r.Update((GetKeyState('R') & (1 << 15)) != 0);

   MSG msg = { };
   while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
      switch (msg.message) {
         case WM_QUIT:
            return false;

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
                  Win32ToggleFullscreen(state.window, state.windowPlacement);
               }
            }
            break;
         }

         case WM_MOUSEWHEEL: {
            state.mouse.wheelDelta += GET_WHEEL_DELTA_WPARAM(msg.wParam) / WHEEL_DELTA;
            break;
         }

         case WM_MOUSEMOVE: {
            const int x = GET_X_LPARAM(msg.lParam);
            const int y = GET_Y_LPARAM(msg.lParam);

            state.mouse.positionDelta.x += x - state.mouse.position.x;
            state.mouse.positionDelta.y += y - state.mouse.position.y;
            state.mouse.position = {x, y};
            break;
         }

         default:
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            break;
      }
   }

   return true;
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

uniform mat4 u_world_from_local_matrix;
uniform mat4 u_view_from_world_matrix;
uniform mat4 u_projection_from_view_matrix;

vec3 cube_vertex(int vertex_id) {
   int b = 1 << vertex_id;
   return vec3(float((0x287a & b) != 0), float((0x02af & b) != 0), float((0x31e3 & b) != 0)) - vec3(0.5);
}

void main() {
   vec3 position = cube_vertex(gl_VertexID);
   gl_Position = u_projection_from_view_matrix * u_view_from_world_matrix * u_world_from_local_matrix * vec4(position, 1.0);

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
   Win32State state = { };
   state.windowPlacement.length = sizeof(WINDOWPLACEMENT);

   state.window = Win32CreateWindow("peasycamera demo", 1280, 720);
   assert(state.window);

   state.dc = GetDC(state.window);
   assert(state.dc);

   state.ctx = Win32CreateGLContext(state.dc);
   assert(state.ctx);

   wglMakeCurrent(state.dc, state.ctx);
   gl3wInit();

   ShowWindow(state.window, showCode);

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

   peasycamera::Camera camera(5.0f);

   const int64_t counterFrequency = Win32GetPerfFrequency();
   int64_t lastCounter = Win32GetPerfCounter();

   while (Win32MessagePump(state)) {
      const int64_t currentCounter = Win32GetPerfCounter();
      float deltaTime = float(double(currentCounter - lastCounter) / double(counterFrequency));
      lastCounter = currentCounter;

      if (state.keyboard.q.m_down) {
         camera.RotateX(deltaTime * 2.0f);
      }

      if (state.keyboard.w.m_down) {
         camera.RotateY(deltaTime * 2.0f);
      }

      if (state.keyboard.e.m_down) {
         camera.RotateZ(deltaTime * 2.0f);
      }

      if (state.keyboard.r.m_press) {
         camera.Reset(0.5f);
      }

      const Int2 sz = Win32GetClientAreaSize(state.window);

      peasycamera::Input input = { };
      input.shiftKeyDown = state.keyboard.shift.m_down;
      input.leftMouseButtonDown = state.mouse.lmb.m_down;
      input.middleMouseButtonDown = state.mouse.mmb.m_down;
      input.rightMouseButtonDown = state.mouse.rmb.m_down;
      input.mouseX = state.mouse.position.x;
      input.mouseY = state.mouse.position.y;
      input.mouseDX = state.mouse.positionDelta.x;
      input.mouseDY = state.mouse.positionDelta.y;
      input.mouseWheelDelta = state.mouse.wheelDelta;
      input.viewport[0] = 0;
      input.viewport[1] = 0;
      input.viewport[2] = sz.x;
      input.viewport[3] = sz.y;
      input.deltaTimeInSeconds = deltaTime;

      camera.Update(input);
      camera.CalculateViewMatrix();

      const float clearColor[] = {0.0f, 0.3f, 0.5f, 1.0f};
      glClearBufferfv(GL_COLOR, 0, clearColor);

      const float one = 1.0f;
      glClearBufferfv(GL_DEPTH, 0, &one);
      glViewport(0, 0, sz.x, sz.y);

      static float angle = 0.0f;
      angle += DirectX::XM_PI * deltaTime;
      DirectX::XMMATRIX localToWorldMatrix = DirectX::XMMatrixIdentity();
      DirectX::XMMATRIX viewToProjectionMatrix = DirectX::XMMatrixPerspectiveFovRH(0.8f, float(sz.x) / float(sz.y), 0.1f, 1000.0f);

      DirectX::XMFLOAT4X4 uploadLocalToWorldMatrix;
      DirectX::XMFLOAT4X4 uploadViewToProjectionMatrix;

      DirectX::XMStoreFloat4x4(&uploadLocalToWorldMatrix, localToWorldMatrix);
      DirectX::XMStoreFloat4x4(&uploadViewToProjectionMatrix, viewToProjectionMatrix);

      glUseProgram(program);

      glUniformMatrix4fv(glGetUniformLocation(program, "u_world_from_local_matrix"), 1, GL_FALSE, reinterpret_cast<GLfloat*>(&uploadLocalToWorldMatrix));
      glUniformMatrix4fv(glGetUniformLocation(program, "u_view_from_world_matrix"), 1, GL_FALSE, camera.m_viewMatrix);
      glUniformMatrix4fv(glGetUniformLocation(program, "u_projection_from_view_matrix"), 1, GL_FALSE, reinterpret_cast<GLfloat*>(&uploadViewToProjectionMatrix));

      glBindVertexArray(vao);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
      glBindVertexArray(0);
      glUseProgram(0);

      SwapBuffers(state.dc);
   }

   return 0;
}
