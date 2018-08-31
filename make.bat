@echo off

set FINAL=/O2 /DNDEBUG /MT
set RELEASE=/Zi /O2 /DNDEBUG /MT
set DEBUG=/Zi /Od /D_DEBUG /MTd

set CONFIG=%FINAL%

for /d %%i in (.\*) do (
cd %%i
if exist make.bat call make.bat clean & if ERRORLEVEL 1 (cd.. & exit /b 1)
cd..
)
exit /b 0