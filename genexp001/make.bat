@echo off

set NAME=genexp001
if "%1" == "clean" if exist %NAME%-external.pch del %NAME%-external.pch

set FINAL=/O2 /DNDEBUG /MT
set RELEASE=/Zi /O2 /DNDEBUG /MT
set DEBUG=/Zi /Od /D_DEBUG /MTd

if not defined CONFIG set CONFIG=%DEBUG%
CFLAGS set CFLAGS=%CONFIG% /EHa- /GR- /Gy /Gw /W3 /nologo /I"..\external"
LFLAGS set LFLAGS=/incremental:no /opt:ref /machine:x64

set ERROR=0

if exist %NAME%.exe del %NAME%.exe
if not exist %NAME%-external.pch cl %CFLAGS% /c /Yc%NAME%-external.h %NAME%-external.cpp
cl %CFLAGS% /Yu%NAME%-external.h %NAME%-main.cpp /link %LFLAGS% %NAME%-external.obj kernel32.lib user32.lib gdi32.lib /out:%NAME%.exe
if ERRORLEVEL 1 set ERROR=1
if exist %NAME%-main.obj del %NAME%-main.obj
if "%1" == "run" if exist %NAME%.exe %NAME%.exe

exit /b %ERROR%