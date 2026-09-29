@echo off

set "VSCMD_START_DIR=%CD%"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"

set tools_dir=tools
set bh=%tools_dir%\compile_threaded.exe
set bh_cpp=%tools_dir%\compile_threaded.cpp

if exist %bh% (
    del /Q %bh%
)

if not exist %bh% (
    cl.exe /EHsc /nologo /Fe%tools_dir%\ /Fo%tools_dir%\ %bh_cpp%
)

set glsl=glsl\\
set cl=%VULKAN_SDK%\Bin\glslangValidator.exe
::set cl=%tools_dir%\glslang\bin\glslangValidator.exe
set outf=+spirv\shader_data.c
set outfb=+spirv\shader_binding.c

"%bh%" "%glsl%" "%cl%" "%outf%" "%outfb%"

pause