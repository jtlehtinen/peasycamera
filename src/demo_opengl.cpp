#include <Windows.h>
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

int __stdcall WinMain(HINSTANCE instance, HINSTANCE ignored, LPSTR cmdLine, int showCode) {
   WNDCLASSA wc = { };
   wc.style = CS_OWNDC;
   wc.hInstance = instance;
   wc.lpfnWndProc = Win32WindowProc;
   wc.lpszClassName = "Class";
   RegisterClassA(&wc);

   HWND window = CreateWindowExA(0, wc.lpszClassName, "peasycamera demo", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr, instance, nullptr);
   assert(window);

   MSG msg = { };
   for (;;) {
      if (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
         if (msg.message == WM_QUIT) {
            break;
         }

         TranslateMessage(&msg);
         DispatchMessageA(&msg);
      } else {
         // @TODO: render...
      }
   }

   return 0;
}
