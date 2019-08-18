@echo off

IF NOT EXIST ..\build\ mkdir ..\build
pushd ..\build

glslangValidator -V ..\code\shader.comp -o ..\code\shader_comp.spv

set vulkanInclude=%vulkan_sdk%\Include

set NAME=source
cl /I %vulkanInclude% /I %meta_tools% -nologo -Zi "..\code\%NAME%.cpp" /link user32.lib gdi32.lib kernel32.lib shlwapi.lib winmm.lib

popd

IF %ERRORLEVEL% EQU 0 ( ..\build\%NAME%.exe )