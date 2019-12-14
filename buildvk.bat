@echo off

IF NOT EXIST ..\build\ mkdir ..\build
pushd ..\build

:: Is there a way to automate selective shader recompilation?

set vulkanInclude=%vulkan_sdk%\Include

set NAME=source
cl /I %vulkanInclude% -nologo -Zi "..\code\%NAME%.cpp" /link user32.lib gdi32.lib kernel32.lib shlwapi.lib winmm.lib

:: - compute
glslangValidator -V ..\code\raytrace.comp -o ..\code\raytrace.spv
glslangValidator -V ..\code\bvh.comp -o ..\code\bvh.spv
glslangValidator -V ..\code\tree_depth.comp -o ..\code\tree_depth.spv
glslangValidator -V ..\code\tree_shader.comp -o ..\code\tree_shader.spv
glslangValidator -V ..\code\step7_shader.comp -o ..\code\step7_shader.spv
glslangValidator -V ..\code\step6_shader.comp -o ..\code\step6_shader.spv
glslangValidator -V ..\code\step5_shader.comp -o ..\code\step5_shader.spv
glslangValidator -V ..\code\step4_shader.comp -o ..\code\step4_shader.spv
glslangValidator -V ..\code\step3_shader.comp -o ..\code\step3_shader.spv
glslangValidator -V ..\code\step2_shader.comp -o ..\code\step2_shader.spv
glslangValidator -V ..\code\step1_shader.comp -o ..\code\step1_shader.spv
glslangValidator -V ..\code\centroidmorton.comp -o ..\code\centroidmorton.spv
glslangValidator -V ..\code\minmax.comp -o ..\code\minmax_comp.spv

:: - graphics
glslangValidator -V ..\code\shader.frag -o ..\code\shader_frag.spv


popd

::IF %ERRORLEVEL% EQU 0 ( ..\build\%NAME%.exe )