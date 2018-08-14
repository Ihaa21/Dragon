@echo off

set CodeDir=..\code
set OutputDir=..\build_win32
set VulcanSdkDir="C:\VulkanSDK\1.1.73.0\Bin"

set CommonCompilerFlags=-Od -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4127 -wd4201 -wd4100 -wd4189 -wd4505 -Z7 -FC
set CommonCompilerFlags=-I "C:\VulkanSDK\1.1.73.0\Include\vulkan" %CommonCompilerFlags%
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib Winmm.lib opengl32.lib DbgHelp.lib

IF NOT EXIST %OutputDir% mkdir %OutputDir%

pushd %OutputDir%

del *.pdb > NUL 2> NUL

REM Asset File Builder
cl %CommonCompilerFlags% -D_CRT_SECURE_NO_WARNINGS %CodeDir%\dragon_asset_builder.cpp /link %CommonLinkerFlags%

REM 64-bit build
call cl %CommonCompilerFlags% %CodeDir%\win32_dragon.cpp -Fmwin32_dragon.map /link %CommonLinkerFlags%

call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\deferred_gbuffer_vert.spv -H %CodeDir%\deferred_gbuffer.vert > deferred_gbuffer_vert.spv.txt
call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\deferred_gbuffer_frag.spv -H %CodeDir%\deferred_gbuffer.frag > deferred_gbuffer_frag.spv.txt

call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\deferred_point_light_vert.spv -H %CodeDir%\deferred_point_light.vert > deferred_point_light_vert.spv.txt
call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\deferred_point_light_frag.spv -H %CodeDir%\deferred_point_light.frag > deferred_point_light_frag.spv.txt

call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\fullscreen_vert.spv -H %CodeDir%\fullscreen.vert > fullscreen_vert.spv.txt

call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\deferred_dir_light_frag.spv -H %CodeDir%\deferred_dir_light.frag > deferred_dir_light_frag.spv.txt

call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\deferred_ssao_frag.spv -H %CodeDir%\ssao.frag > ssao_frag.spv.txt

popd
