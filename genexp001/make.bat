@echo off

set NAME=genexp001
::set CONFIG=/O2 /DNDEBUG /MT
set CONFIG=/Od /D_DEBUG /MTd

set CFLAGS=/Zi %CONFIG% /EHsc /GR- /Gy /Gw /W3 /nologo /I"external"
set LFLAGS=/incremental:no /opt:ref /machine:x64 /out:%NAME%.exe

if exist %NAME%.exe del %NAME%.exe
if not exist %NAME%_external.pch (cl %CFLAGS% /c /Yc%NAME%_external.h %NAME%_external.cpp)
cl %CFLAGS% /Yu%NAME%_external.h %NAME%_main.cpp /link %LFLAGS% %NAME%_external.obj kernel32.lib user32.lib gdi32.lib
if exist %NAME%_main.obj del %NAME%_main.obj
if "%1" == "run" if exist %NAME%.exe (.\%NAME%.exe)

:end
