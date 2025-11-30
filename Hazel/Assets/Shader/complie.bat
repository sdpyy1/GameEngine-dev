@echo off
:: ==============================================
:: 自动编译当前目录下的所有 .glsl 着色器文件
:: 生成的 spv 文件放到 spv 文件夹中
:: 仅当文件中定义了 GEOMETRY_SHADER 时才编译几何着色器
:: 需要安装 VulkanSDK 并配置正确路径
:: ==============================================
setlocal enabledelayedexpansion

set GLSLC="D:\context\VulkanSDK\1.3.243.0\Bin\glslc.exe"

if not exist spv (
    mkdir spv
)

echo ==============================================
echo [Shader Compilation Started]
echo ==============================================

for %%f in (*.glsl) do (

    echo %%f | findstr /i "\.comp\." >nul
    if !errorlevel! equ 0 (
        echo Compiling compute shader: %%f ...
        %GLSLC% -fshader-stage=comp %%f -DCOMPUTE_SHADER -o spv/%%~nf.spv
    ) else (
        echo Compiling vertex/fragment shader: %%f ...
        %GLSLC% -fshader-stage=vert %%f -DVERTEX_SHADER -o spv/%%~nfVert.spv
        %GLSLC% -fshader-stage=frag %%f -DFRAGMENT_SHADER -o spv/%%~nfFrag.spv
        
        findstr /i /m "#define GEOMETRY_SHADER" "%%f" >nul
        if !errorlevel! equ 0 (
            echo Compiling geometry shader: %%f ...
            %GLSLC% -fshader-stage=geom %%f -DGEOMETRY_SHADER -o spv/%%~nfGeom.spv
        )
    )
)
endlocal

echo ==============================================
echo [All shaders compiled successfully!]
echo Output folder: spv\
echo ==============================================

pause