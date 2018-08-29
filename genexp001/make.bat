@echo off

set NAME=genexp001
::set CONFIG=/O2 /DNDEBUG /MT
set CONFIG=/Od /D_DEBUG /MTd

set CFLAGS=/Zi %CONFIG% /EHsc /GR- /Gy /Gw /W3 /nologo /I"../external"
set LFLAGS=/incremental:no /opt:ref /machine:x64 /out:%NAME%.exe

if exist %NAME%.exe del %NAME%.exe
if not exist %NAME%-external.pch (cl %CFLAGS% /c /Yc%NAME%-external.h %NAME%-external.cpp)
cl %CFLAGS% /Yu%NAME%-external.h %NAME%-main.cpp /link %LFLAGS% %NAME%-external.obj kernel32.lib user32.lib gdi32.lib
if exist %NAME%-main.obj del %NAME%-main.obj
if "%1" == "run" if exist %NAME%.exe (.\%NAME%.exe)

:end
