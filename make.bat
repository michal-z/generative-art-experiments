@echo off

set RELEASE_CONFIG=/O2 /DNDEBUG /MT
set DEBUG_CONFIG=/Od /D_DEBUG /MTd

::set CONFIG=%RELEASE_CONFIG%
::set CONFIG=%DEBUG_CONFIG%
::set CFLAGS=/Zi %CONFIG% /EHa- /GR- /Gy /Gw /W3 /nologo /I"..\external"
::set LFLAGS=/incremental:no /opt:ref /machine:x64

for /d %%i in (.\*) do (
cd %%i
if exist make.bat call make.bat clean & if errorlevel 1 cd.. & exit /b 1
cd..
)