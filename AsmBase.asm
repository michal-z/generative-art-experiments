format PE64 GUI 4.0
entry F_Start

INFINITE = 0xffffffff
IDC_ARROW = 32512
WS_VISIBLE = 010000000h
WS_CAPTION = 000C00000h
WS_SYSMENU = 000080000h
WS_MINIMIZEBOX = 000020000h
CW_USEDEFAULT = 80000000h
PM_REMOVE = 0001h
WM_QUIT = 0012h
WM_KEYDOWN = 0100h
WM_DESTROY = 0002h
VK_ESCAPE = 01Bh
SRCCOPY = 0x00CC0020

K_WindowStyle equ WS_SYSMENU+WS_CAPTION+WS_MINIMIZEBOX
K_WindowPixelsPerSide equ 1024

virtual at rsp
  rept (1024/32) N:0 { yword#N: rb 32 }
end virtual

macro falign { align 16 }
macro icall Addr* { call [Addr] }
macro inline Name*, [Params] { common Name Params }

macro M_GetProcAddress Lib*, Proc* {
            mov         rcx, Lib
            lea         rdx, [.#Proc]
            icall       GetProcAddress
            mov         [Proc], rax }

section '.text' code readable executable

falign
F_GetTime:
.StackSize = 2*32+24
            sub         rsp, .StackSize
            mov         rax, [.Frequency]
            test        rax, rax
            jnz         .AfterInit
            lea         rcx, [.Frequency]
            icall       QueryPerformanceFrequency
            lea         rcx, [.StartCounter]
            icall       QueryPerformanceCounter
.AfterInit: lea         rcx, [yword1+0]
            icall       QueryPerformanceCounter
            mov         rcx, [yword1+0]
            sub         rcx, [.StartCounter]
            mov         rdx, [.Frequency]
            vxorps      xmm0, xmm0, xmm0
            vcvtsi2sd   xmm1, xmm0, rcx
            vcvtsi2sd   xmm2, xmm0, rdx
            vdivsd      xmm0, xmm1, xmm2
            add         rsp, .StackSize
            ret

falign
F_UpdateFrameStats:
.StackSize = 4*32+24
            sub         rsp, .StackSize
            mov         rax, [.PreviousTime]
            test        rax, rax
            jnz         .AfterInit
            call        F_GetTime
            vmovsd      [.PreviousTime], xmm0
            vmovsd      [.HeaderUpdateTime], xmm0
.AfterInit: call        F_GetTime
            vmovsd      [G_Time], xmm0
            vsubsd      xmm1, xmm0, [.PreviousTime]         ; xmm1 = DeltaTime
            vmovsd      [.PreviousTime], xmm0
            vxorps      xmm2, xmm2, xmm2
            vcvtsd2ss   xmm1, xmm2, xmm1                    ; xmm1 = (float)DeltaTime
            vmovss      [G_DeltaTime], xmm1
            vmovsd      xmm1, [.HeaderUpdateTime]
            vsubsd      xmm2, xmm0, xmm1                    ; xmm2 = Time - HeaderUpdateTime
            vmovsd      xmm3, [K_1_0]                       ; xmm3 = 1.0
            vcomisd     xmm2, xmm3
            jb          .AfterHeaderUpdate
            vmovsd      [.HeaderUpdateTime], xmm0
            mov         eax, [.FrameCount]
            vxorpd      xmm1, xmm1, xmm1
            vcvtsi2sd   xmm1, xmm1, eax                     ; xmm1 = FrameCount
            vdivsd      xmm0, xmm1, xmm2                    ; xmm0 = FrameCount / (Time - HeaderUpdateTime)
            vdivsd      xmm1, xmm2, xmm1
            vmulsd      xmm1, xmm1, [K_1000000_0]
            mov         [.FrameCount], 0
            lea         rcx, [yword2+0]
            lea         rdx, [.HeaderFormat]
            vcvtsd2si   r8, xmm0
            vcvtsd2si   r9, xmm1
            lea         rax, [G_ApplicationName]
            mov         [yword1+0], rax
            icall       wsprintf
            mov         rcx, [G_WindowHandle]
            lea         rdx, [yword2+0]
            icall       SetWindowText
.AfterHeaderUpdate:
            inc         [.FrameCount]
            add         rsp, .StackSize
            ret

falign
F_CheckAvx2Support:
            mov         eax, 1
            cpuid
            and         ecx, 0x58001000          ; check RDRAND, AVX, OSXSAVE, FMA
            cmp         ecx, 0x58001000
            jne         .NotSupported
            mov         eax, 0x7
            xor         ecx, ecx
            cpuid
            and         ebx, 0x20                ; check AVX2
            cmp         ebx, 0x20
            jne         .NotSupported
            xor         ecx, ecx
            xgetbv
            and         eax, 0x6                 ; check OS support
            cmp         eax, 0x6
            jne         .NotSupported
            mov         eax, 1
            jmp         .Return
.NotSupported:
            xor         eax, eax
.Return:    ret

falign
F_ProcessWindowMessage:
.StackSize = 1*32+8
            sub         rsp, .StackSize
            cmp         edx, WM_KEYDOWN
            je          .KeyDown
            cmp         edx, WM_DESTROY
            je          .Destroy
.Default:   icall       DefWindowProc
            jmp         .Return
.KeyDown:   cmp         r8d, VK_ESCAPE
            jne         .Default
            xor         ecx, ecx
            icall       PostQuitMessage
            xor         eax, eax
            jmp         .Return
.Destroy:   xor         ecx, ecx
            icall       PostQuitMessage
            xor         eax, eax
.Return:    add         rsp, .StackSize
            ret

falign
F_InitializeWindow:
.StackSize = 3*32+24
            sub         rsp, .StackSize
            mov         [yword0+0], rsi
            ; create window class
            lea         rax, [F_ProcessWindowMessage]
            lea         rcx, [G_ApplicationName]
            mov         [.WindowClass.lpfnWndProc], rax
            mov         [.WindowClass.lpszClassName], rcx
            xor         ecx, ecx
            icall       GetModuleHandle
            mov         [.WindowClass.hInstance], rax
            xor         ecx, ecx
            mov         edx, IDC_ARROW
            icall       LoadCursor
            mov         [.WindowClass.hCursor], rax
            lea         rcx, [.WindowClass]
            icall       RegisterClass
            test        eax, eax
            jz          .Return
            ; compute window size
            mov         eax, K_WindowPixelsPerSide
            mov         [.Rect.right], eax
            mov         [.Rect.bottom], eax
            lea         rcx, [.Rect]
            mov         edx, K_WindowStyle
            xor         r8d, r8d
            icall       AdjustWindowRect
            mov         r10d, [.Rect.right]
            mov         r11d, [.Rect.bottom]
            sub         r10d, [.Rect.left]                  ; r10d = window width
            sub         r11d, [.Rect.top]                   ; r11d = window height
            xor         esi, esi                            ; rsi = 0
            ; create window
            xor         ecx, ecx
            lea         rdx, [G_ApplicationName]
            mov         r8, rdx
            mov         r9d, WS_VISIBLE+K_WindowStyle
            mov         [yword1+0], dword CW_USEDEFAULT
            mov         [yword1+8], dword CW_USEDEFAULT
            mov         [yword1+16], r10
            mov         [yword1+24], r11
            mov         [yword2+0], rsi
            mov         [yword2+8], rsi
            mov         rax, [.WindowClass.hInstance]
            mov         [yword2+16], rax
            mov         [yword2+24], rsi
            icall       CreateWindowEx
            test        rax, rax
            jz          .Return
            mov         [G_WindowHandle], rax
            ; create bitmap
            mov         rcx, rax                            ; window handle
            icall       GetDC
            test        rax, rax
            jz          .Return
            mov         [G_WindowHdc], rax
            mov         rcx, rax
            lea         rdx, [.BitmapInfoHeader]
            xor         r8d, r8d
            lea         r9, [G_WindowPixels]
            mov         [yword1+0], r8                      ; 0
            mov         [yword1+8], r8                      ; 0
            icall       CreateDIBSection
            test        rax, rax
            jz          .Return
            mov         rsi, rax                            ; rsi = bitmap handle
            mov         rcx, [G_WindowHdc]                  ; rcx = window hdc
            icall       CreateCompatibleDC
            test        rax, rax
            jz          .Return
            mov         [G_BitmapHdc], rax
            mov         rcx, rax                            ; bitmap hdc
            mov         rdx, rsi                            ; bitmap handle
            icall       SelectObject
            test        eax, eax
            jz          .Return
            ; success
            mov         eax, 1
.Return:    mov         rsi, [yword0+0]
            add         rsp, .StackSize
            ret

falign
F_Update:
.StackSize = 2*32+24
            sub         rsp, .StackSize
            call        F_UpdateFrameStats
            add         rsp, .StackSize
            ret

falign
F_Initialize:
.StackSize = 2*32+24
            sub         rsp, .StackSize
            call        F_InitializeWindow
            add         rsp, .StackSize
            ret

falign
F_Start:
.StackSize = 4*32+24
            sub         rsp, .StackSize
            lea         rcx, [.Kernel32]
            icall       LoadLibrary
            mov         [yword3+0], rax                     ; [yword3+0] = kernel32.dll
            lea         rcx, [.User32]
            icall       LoadLibrary
            mov         [yword3+8], rax                     ; [yword3+8] = user32.dll
            lea         rcx, [.Gdi32]
            icall       LoadLibrary
            mov         [yword3+16], rax                    ; [yword3+16] = gdi32.dll
            inline      M_GetProcAddress, [yword3+0], ExitProcess
            inline      M_GetProcAddress, [yword3+0], GetModuleHandle
            inline      M_GetProcAddress, [yword3+0], QueryPerformanceFrequency
            inline      M_GetProcAddress, [yword3+0], QueryPerformanceCounter
            inline      M_GetProcAddress, [yword3+8], RegisterClass
            inline      M_GetProcAddress, [yword3+8], CreateWindowEx
            inline      M_GetProcAddress, [yword3+8], DefWindowProc
            inline      M_GetProcAddress, [yword3+8], PeekMessage
            inline      M_GetProcAddress, [yword3+8], DispatchMessage
            inline      M_GetProcAddress, [yword3+8], LoadCursor
            inline      M_GetProcAddress, [yword3+8], SetWindowText
            inline      M_GetProcAddress, [yword3+8], AdjustWindowRect
            inline      M_GetProcAddress, [yword3+8], PostQuitMessage
            inline      M_GetProcAddress, [yword3+8], GetDC
            inline      M_GetProcAddress, [yword3+8], wsprintf
            inline      M_GetProcAddress, [yword3+8], MessageBox
            inline      M_GetProcAddress, [yword3+8], SetProcessDPIAware
            inline      M_GetProcAddress, [yword3+16], CreateCompatibleDC
            inline      M_GetProcAddress, [yword3+16], CreateDIBSection
            inline      M_GetProcAddress, [yword3+16], SelectObject
            inline      M_GetProcAddress, [yword3+16], BitBlt
            icall       SetProcessDPIAware
            call        F_CheckAvx2Support
            test        eax, eax
            jnz         .CpuOk
            xor         ecx, ecx                ; hwnd
            lea         rdx, [.NoAvx2]
            lea         r8, [.NoAvx2Caption]
            mov         r9d, 0x10               ; MB_ICONERROR
            icall       MessageBox
            jmp         .Exit
.CpuOk:     call        F_Initialize
            test        eax, eax
            jz          .Exit
            ; PeekMessage, if queue empty jump to F_Update
.MainLoop:  lea         rcx, [.Message]
            xor         edx, edx
            xor         r8d, r8d
            xor         r9d, r9d
            mov         [yword1], dword PM_REMOVE
            icall       PeekMessage
            test        eax, eax
            jz          .Update
            ; DispatchMessage, if WM_QUIT received exit application
            lea         rcx, [.Message]
            icall       DispatchMessage
            cmp         [.Message.message], WM_QUIT
            je          .Exit
            jmp         .MainLoop               ; peek next message
.Update:    call        F_Update
            ; transfer image pixels to the window
            mov         rcx, [G_WindowHdc]
            xor         edx, edx
            xor         r8d, r8d
            mov         r9d, K_WindowPixelsPerSide
            mov         [yword1+0], r9d
            mov         rax, [G_BitmapHdc]
            mov         [yword1+8], rax
            mov         [yword1+16], rdx
            mov         [yword1+24], rdx
            mov         [yword2+0], dword SRCCOPY
            icall       BitBlt
            ; repeat
            jmp         .MainLoop
.Exit:      xor         ecx, ecx
            icall       ExitProcess
            ret

section '.data' data readable writeable

G_ApplicationName db 'AsmBase', 0

align 8
G_WindowPixels dq 0
G_WindowHandle dq 0
G_WindowHdc dq 0
G_BitmapHdc dq 0
G_Time dq 0
G_DeltaTime dd 0, 0

F_GetTime.StartCounter dq 0
F_GetTime.Frequency dq 0
F_UpdateFrameStats.PreviousTime dq 0
F_UpdateFrameStats.HeaderUpdateTime dq 0
F_UpdateFrameStats.FrameCount dd 0, 0
F_UpdateFrameStats.HeaderFormat db '[%d fps  %d us] %s', 0
F_Start.NoAvx2 db 'Program requires CPU with AVX2 support.', 0
F_Start.NoAvx2Caption db 'Unsupported CPU', 0

align 8
K_1_0 dq 1.0
K_1000000_0 dq 1000000.0

align 8
F_Start.Message:
  .hwnd dq 0
  .message dd 0, 0
  .wParam dq 0
  .lParam dq 0
  .time dd 0
  .pt.x dd 0
  .pt.y dd 0
  .lPrivate dd 0

align 8
F_InitializeWindow.WindowClass:
  .style dd 0, 0
  .lpfnWndProc dq 0
  .cbClsExtra dd 0
  .cbWndExtra dd 0
  .hInstance dq 0
  .hIcon dq 0
  .hCursor dq 0
  .hbrBackground dq 0
  .lpszMenuName dq 0
  .lpszClassName dq 0

align 8
F_InitializeWindow.BitmapInfoHeader:
  .biSize dd 40
  .biWidth dd K_WindowPixelsPerSide
  .biHeight dd K_WindowPixelsPerSide
  .biPlanes dw 1
  .biBitCount dw 32
  .biCompression dd 0
  .biSizeImage dd K_WindowPixelsPerSide * K_WindowPixelsPerSide
  .biXPelsPerMeter dd 0
  .biYPelsPerMeter dd 0
  .biClrUsed dd 0
  .biClrImportant dd 0

align 8
F_InitializeWindow.Rect:
  .left dd 0
  .top dd 0
  .right dd 0
  .bottom dd 0

ExitProcess dq 0
GetModuleHandle dq 0
QueryPerformanceFrequency dq 0
QueryPerformanceCounter dq 0

RegisterClass dq 0
CreateWindowEx dq 0
DefWindowProc dq 0
PeekMessage dq 0
DispatchMessage dq 0
LoadCursor dq 0
SetWindowText dq 0
AdjustWindowRect dq 0
PostQuitMessage dq 0
GetDC dq 0
wsprintf dq 0
SetProcessDPIAware dq 0
MessageBox dq 0

CreateDIBSection dq 0
CreateCompatibleDC dq 0
SelectObject dq 0
BitBlt dq 0

F_Start.Kernel32 db 'kernel32.dll', 0
F_Start.ExitProcess db 'ExitProcess', 0
F_Start.GetModuleHandle db 'GetModuleHandleA', 0
F_Start.QueryPerformanceFrequency db 'QueryPerformanceFrequency', 0
F_Start.QueryPerformanceCounter db 'QueryPerformanceCounter', 0

F_Start.User32 db 'user32.dll', 0
F_Start.RegisterClass db 'RegisterClassA', 0
F_Start.CreateWindowEx db 'CreateWindowExA', 0
F_Start.DefWindowProc db 'DefWindowProcA', 0
F_Start.PeekMessage db 'PeekMessageA', 0
F_Start.DispatchMessage db 'DispatchMessageA', 0
F_Start.LoadCursor db 'LoadCursorA', 0
F_Start.SetWindowText db 'SetWindowTextA', 0
F_Start.AdjustWindowRect db 'AdjustWindowRect', 0
F_Start.PostQuitMessage db 'PostQuitMessage', 0
F_Start.GetDC db 'GetDC', 0
F_Start.wsprintf db 'wsprintfA', 0
F_Start.SetProcessDPIAware db 'SetProcessDPIAware', 0
F_Start.MessageBox db 'MessageBoxA', 0

F_Start.Gdi32 db 'gdi32.dll', 0
F_Start.CreateDIBSection db 'CreateDIBSection', 0
F_Start.CreateCompatibleDC db 'CreateCompatibleDC', 0
F_Start.SelectObject db 'SelectObject', 0
F_Start.BitBlt db 'BitBlt', 0

section '.idata' import data readable writeable

dd 0, 0, 0, rva F_Start.Kernel32, rva F_Start.Kernel32Table
dd 0, 0, 0, 0, 0

align 8
F_Start.Kernel32Table:
  LoadLibrary dq rva F_Start.LoadLibrary
  GetProcAddress dq rva F_Start.GetProcAddress
  dq 0

F_Start.LoadLibrary dw 0
  db 'LoadLibraryA', 0
F_Start.GetProcAddress dw 0
  db 'GetProcAddress', 0
