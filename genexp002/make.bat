@echo off
setlocal
setlocal enableextensions

set NAME=genexp002
if "%1" == "clean" if exist %NAME%-external.pch del %NAME%-external.pch

set FINAL=/O2 /DNDEBUG /MT
set RELEASE=/Zi /O2 /DNDEBUG /MT
set DEBUG=/Zi /Od /D_DEBUG /MTd

if not defined CONFIG set CONFIG=%DEBUG%
set CFLAGS=%CONFIG% /EHa- /GR- /Gy /Gw /W3 /nologo /I"..\external"
set LFLAGS=/incremental:no /opt:ref /machine:x64
set FXC=fxc.exe /Ges /O3 /WX /nologo /Qstrip_reflect /Qstrip_debug /Qstrip_priv

set ERROR=0

if exist data\shaders\*.cso del data\shaders\*.cso
if exist %NAME%.exe del %NAME%.exe

%FXC% /D VS_IMGUI /E FVertexShader /Fo data\shaders\imgui-vs.cso /T vs_5_1 %NAME%.hlsl & if ERRORLEVEL 1 (set ERROR=1 & goto :end)
%FXC% /D PS_IMGUI /E FPixelShader /Fo data\shaders\imgui-ps.cso /T ps_5_1 %NAME%.hlsl & if ERRORLEVEL 1 (set ERROR=1 & goto :end)
%FXC% /D VS_DISPLAY_CANVAS /E FVertexShader /Fo data\shaders\display-canvas-vs.cso /T vs_5_1 %NAME%.hlsl & if ERRORLEVEL 1 (set ERROR=1 & goto :end)
%FXC% /D PS_DISPLAY_CANVAS /E FPixelShader /Fo data\shaders\display-canvas-ps.cso /T ps_5_1 %NAME%.hlsl & if ERRORLEVEL 1 (set ERROR=1 & goto :end)
%FXC% /D VS_SAND /E FVertexShader /Fo data\shaders\sand-vs.cso /T vs_5_1 %NAME%.hlsl & if ERRORLEVEL 1 (set ERROR=1 & goto :end)
%FXC% /D PS_SAND /E FPixelShader /Fo data\shaders\sand-ps.cso /T ps_5_1 %NAME%.hlsl & if ERRORLEVEL 1 (set ERROR=1 & goto :end)

if not exist %NAME%-external.pch (cl %CFLAGS% /c /Yc%NAME%-external.h %NAME%-external.cpp)
cl %CFLAGS% /Yu%NAME%-external.h %NAME%-main.cpp /link %LFLAGS% %NAME%-external.obj kernel32.lib user32.lib gdi32.lib /out:%NAME%.exe
if ERRORLEVEL 1 (set ERROR=1)
if exist %NAME%-main.obj del %NAME%-main.obj
if "%1" == "run" if exist %NAME%.exe %NAME%.exe

:end
exit /b %ERROR%
