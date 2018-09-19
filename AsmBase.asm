format PE64 GUI 4.0
entry F_Start

INFINITE = 0xffffffff
IDI_APPLICATION = 32512
IDC_ARROW = 32512
WS_VISIBLE = 010000000h
WS_OVERLAPPED = 000000000h
WS_CAPTION = 000C00000h
WS_SYSMENU = 000080000h
WS_VISIBLE = 010000000h
WS_MINIMIZEBOX = 000020000h
CW_USEDEFAULT = 80000000h
PM_REMOVE = 0001h
WM_QUIT = 0012h
WM_KEYDOWN = 0100h
WM_DESTROY = 0002h
VK_ESCAPE = 01Bh
SRCCOPY = 0x00CC0020

K_WindowStyle equ WS_OVERLAPPED+WS_SYSMENU+WS_CAPTION+WS_MINIMIZEBOX
K_WindowPixelsPerSide equ 1024

K_StackSize equ 1024+24
virtual at rsp
  rept ((K_StackSize-24)/32) N:0 { yword#N: rb 32*N }
end virtual

macro falign { align 16 }
macro icall Addr* { call [Addr] }
macro inline Name*, [Params] { common Name Params }

macro M_GetProcAddress Lib*, Proc* {
            mov         rcx, [G_#Lib]
            lea         rdx, [G_#Proc]
            icall       GetProcAddress
            mov         [Proc], rax }

section '.text' code readable executable

falign
F_ProcessWindowMessage:
            sub         rsp, K_StackSize
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
.Destroy:   icall       PostQuitMessage
            xor         eax, eax
            jmp         .Return
.Return:    add         rsp, K_StackSize
            ret

falign
F_InitializeWindow:
            sub         rsp, K_StackSize
            ; create window class
            lea         rax, [F_ProcessWindowMessage]
            lea         rcx, [G_ApplicationName]
            mov         [G_WindowClass.lpfnWndProc], rax
            mov         [G_WindowClass.lpszClassName], rcx
            xor         ecx, ecx
            icall       GetModuleHandle
            mov         [G_WindowClass.hInstance], rax
            xor         ecx, ecx
            mov         edx, IDC_ARROW
            icall       LoadCursor
            mov         [G_WindowClass.hCursor], rax
            lea         rcx, [G_WindowClass]
            icall       RegisterClass
            test        eax, eax
            jz          .Error
            ; compute window size
            mov         eax, K_WindowPixelsPerSide
            mov         [G_Rect.right], eax
            mov         [G_Rect.bottom], eax
            lea         rcx, [G_Rect]
            mov         edx, K_WindowStyle
            xor         r8d, r8d
            icall       AdjustWindowRect
            mov         r10d, [G_Rect.right]
            mov         r11d, [G_Rect.bottom]
            sub         r10d, [G_Rect.left]
            sub         r11d, [G_Rect.top]
            ; create window
            mov         eax, 1
            add         rsp, K_StackSize
            ret
.Error:     xor         eax, eax
            add         rsp, K_StackSize
            ret                        

falign
F_Initialize:
            sub         rsp, K_StackSize
            vmovaps     [yword0], ymm0
            call        F_InitializeWindow
            add         rsp, K_StackSize
            ret

falign
F_Start:    sub         rsp, 32+24              ; shadow space + alignment to 32 bytes
            vmovaps     [yword0], ymm0
            lea         rcx, [G_Kernel32Str]
            icall       LoadLibrary
            mov         [G_Kernel32], rax
            lea         rcx, [G_User32Str]
            icall       LoadLibrary
            mov         [G_User32], rax
            lea         rcx, [G_Gdi32Str]
            icall       LoadLibrary
            mov         [G_Gdi32], rax
            inline      M_GetProcAddress, Kernel32, ExitProcess
            inline      M_GetProcAddress, Kernel32, GetModuleHandle
            inline      M_GetProcAddress, Kernel32, QueryPerformanceFrequency
            inline      M_GetProcAddress, Kernel32, QueryPerformanceCounter
            inline      M_GetProcAddress, User32, RegisterClass
            inline      M_GetProcAddress, User32, CreateWindowEx
            inline      M_GetProcAddress, User32, DefWindowProc
            inline      M_GetProcAddress, User32, PeekMessage
            inline      M_GetProcAddress, User32, DispatchMessage
            inline      M_GetProcAddress, User32, LoadCursor
            inline      M_GetProcAddress, User32, SetWindowText
            inline      M_GetProcAddress, User32, AdjustWindowRect
            inline      M_GetProcAddress, User32, PostQuitMessage
            call        F_Initialize
            mov         ecx, eax
            icall       ExitProcess
            ret

section '.data' data readable writeable

align 8
G_Message:
  .hwnd dq 0
  .message dd 0, 0
  .wParam dq 0
  .lParam dq 0
  .time dd 0
  .pt.x dd 0
  .pt.y dd 0, 0

align 8
G_WindowClass:
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
G_BitmapInfoHeader:
  .biSize dd 0
  .biWidth dd 0
  .biHeight dd 0
  .biPlanes dw 0
  .biBitCount dw 0
  .biCompression dd 0
  .biSizeImage dd 0
  .biXPelsPerMeter dd 0
  .biYPelsPerMeter dd 0
  .biClrUsed dd 0
  .biClrImportant dd 0

align 8
G_Rect:
  .left dd 0
  .top dd 0
  .right dd 0
  .bottom dd 0

align 8
G_Kernel32 dq 0
G_User32 dq 0
G_Gdi32 dq 0

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

G_ApplicationName db 'AsmBase', 0

G_Kernel32Str db 'kernel32.dll', 0
G_ExitProcess db 'ExitProcess', 0
G_GetModuleHandle db 'GetModuleHandleA', 0
G_QueryPerformanceFrequency db 'QueryPerformanceFrequency', 0
G_QueryPerformanceCounter db 'QueryPerformanceCounter', 0

G_User32Str db 'user32.dll', 0
G_RegisterClass db 'RegisterClassA', 0
G_CreateWindowEx db 'CreateWindowExA', 0
G_DefWindowProc db 'DefWindowProcA', 0
G_PeekMessage db 'PeekMessageA', 0
G_DispatchMessage db 'DispatchMessageA', 0
G_LoadCursor db 'LoadCursorA', 0
G_SetWindowText db 'SetWindowTextA', 0
G_AdjustWindowRect db 'AdjustWindowRect', 0
G_PostQuitMessage db 'PostQuitMessage', 0

G_Gdi32Str db 'gdi32.dll', 0

section '.idata' import data readable writeable

dd 0, 0, 0, rva G_Kernel32Str, rva G_Kernel32Table
dd 0, 0, 0, 0, 0

align 8
G_Kernel32Table:
  LoadLibrary dq rva G_LoadLibrary
  GetProcAddress dq rva G_GetProcAddress
  dq 0

G_LoadLibrary dw 0
  db 'LoadLibraryA', 0
G_GetProcAddress dw 0
  db 'GetProcAddress', 0