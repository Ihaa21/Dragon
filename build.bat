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

call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\build_tile_data_comp.spv -H %CodeDir%\build_tile_data.comp > build_tile_data_comp.spv.txt
call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\depth_pre_pass_vert.spv -H %CodeDir%\depth_pre_pass.vert > depth_pre_pass_vert.spv.txt
call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\light_culling_comp.spv -H %CodeDir%\light_culling.comp > light_culling_comp.spv.txt
call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\tiled_forward_vert.spv -H %CodeDir%\tiled_forward.vert > tiled_forward_vert.spv.txt
call %VulcanSdkDir%\glslangValidator.exe -V -o ..\data\tiled_forward_frag.spv -H %CodeDir%\tiled_forward.frag > tiled_forward_frag.spv.txt

popd
