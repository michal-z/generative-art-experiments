@echo off

set NAME=genexp002
if "%1" == "clean" if exist %NAME%-external.pch del %NAME%-external.pch

if not defined CONFIG set CONFIG=%DEBUG_CONFIG%
if not defined CFLAGS set CFLAGS=/Zi %CONFIG% /EHa- /GR- /Gy /Gw /W3 /nologo /I"..\external"
if not defined LFLAGS set LFLAGS=/incremental:no /opt:ref /machine:x64

set ERROR=0

if exist %NAME%.exe del %NAME%.exe
if not exist %NAME%-external.pch cl %CFLAGS% /c /Yc%NAME%-external.h %NAME%-external.cpp
cl %CFLAGS% /Yu%NAME%-external.h %NAME%-main.cpp /link %LFLAGS% %NAME%-external.obj kernel32.lib user32.lib gdi32.lib /out:%NAME%.exe
if ERRORLEVEL 1 set ERROR=1
if exist %NAME%-main.obj del %NAME%-main.obj
if "%1" == "run" if exist %NAME%.exe %NAME%.exe

exit /b %ERROR%