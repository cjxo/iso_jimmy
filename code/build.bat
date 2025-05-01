@echo off
setlocal enabledelayedexpansion

if not exist ..\build mkdir ..\build
pushd ..\build
cl /nologo /Zi /Od /W4 /wd4201 /DISO_DEBUG ..\code\main.c /link /incremental:no user32.lib d3d11.lib dxgi.lib dxguid.lib gdi32.lib d3dcompiler.lib winmm.lib
popd

if %errorlevel% neq 0 exit /b %errorlevel%

endlocal
