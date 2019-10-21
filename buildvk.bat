@echo off

IF NOT EXIST ..\build\ mkdir ..\build
pushd ..\build

::glslangValidator -V ..\code\shader.comp -o ..\code\shader_comp.spv
::glslangValidator -H ..\code\minmax.comp -o ..\code\minmax_comp.spv > ..\code\asm.txt
glslangValidator -V ..\code\minmax.comp -o ..\code\minmax_comp.spv
glslangValidator -V ..\code\radix.comp -o ..\code\radix_comp.spv
glslangValidator -V ..\code\centroidmorton.comp -o ..\code\centroidmorton.spv
glslangValidator -V ..\code\shader.vert -o ..\code\shader_vert.spv
glslangValidator -V ..\code\shader.frag -o ..\code\shader_frag.spv

set vulkanInclude=%vulkan_sdk%\Include

set NAME=source
cl /I %vulkanInclude% -nologo -Zi "..\code\%NAME%.cpp" /link user32.lib gdi32.lib kernel32.lib shlwapi.lib winmm.lib

popd

::IF %ERRORLEVEL% EQU 0 ( ..\build\%NAME%.exe )