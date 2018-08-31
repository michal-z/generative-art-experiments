@echo off
setlocal enableextensions
setlocal enabledelayedexpansion

set FINAL=/O2 /DNDEBUG /MT
set RELEASE=/Zi /O2 /DNDEBUG /MT
set DEBUG=/Zi /Od /D_DEBUG /MTd

set CONFIG=%FINAL%
set ERROR=0

for /d %%i in (.\*) do (
cd %%i
if exist *.pdb del *.pdb
if exist make.bat call make.bat clean & if ERRORLEVEL 1 (set ERROR=1)
if exist *.obj del *.obj
if exist *.pch del *.pch
cd..
if !ERROR! neq 0 (goto :end)
)

:end
exit /b %ERROR%