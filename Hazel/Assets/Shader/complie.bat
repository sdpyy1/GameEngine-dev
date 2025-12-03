@echo off
setlocal EnableDelayedExpansion

set "GLSLC=D:\context\VulkanSDK\1.4.309.0\Bin\glslc.exe"

echo ========== Shader Compile Start ==========

for /r %%f in (*.glsl) do (

    REM ==== 跳过 common 文件夹 ====
    echo %%f | findstr /i "\\common\\" >nul
    if not errorlevel 1 (
        echo Skip common: %%f
    ) else (

        set "FILE=%%f"
        set "DIR=%%~dpf"
        set "NAME=%%~nf"

        REM 创建 spv 目录
        if not exist "%%~dpfspv" (
            mkdir "%%~dpfspv"
        )
        REM ---- Compute Shader  ----
        findstr /i /m "COMPUTE_SHADER" "%%f" >nul
        if not errorlevel 1 (
        echo Compiling compute shader: %%f ...
        "%GLSLC%" -fshader-stage=comp "%%f" -DCOMPUTE_SHADER -o "%%~dpfspv\%%~nfComp.spv"
        )
        REM ---- Vertex Shader ----
        findstr /i /m "VERTEX_SHADER" "%%f" >nul
        if not errorlevel 1 (
            echo Compiling vertex shader: %%f ...
            "%GLSLC%" -fshader-stage=vert "%%f" -DVERTEX_SHADER -o "%%~dpfspv\%%~nfVert.spv"
        )

        REM ---- Fragment Shader ----
        findstr /i /m "FRAGMENT_SHADER" "%%f" >nul
        if not errorlevel 1 (
            echo Compiling fragment shader: %%f ...
            "%GLSLC%" -fshader-stage=frag "%%f" -DFRAGMENT_SHADER -o "%%~dpfspv\%%~nfFrag.spv"
        )

        REM ---- Geometry Shader ----
        findstr /i /m " GEOMETRY_SHADER" "%%f" >nul
        if not errorlevel 1 (
            echo Compiling geometry shader: %%f ...
            "%GLSLC%" -fshader-stage=geom "%%f" -DGEOMETRY_SHADER -o "%%~dpfspv\%%~nfGeom.spv"
        )

        REM ---- RayGen Shader ----
        findstr /i /m "RAYGEN_SHADER" "%%f" >nul
        if not errorlevel 1 (
            echo Compiling raygen shader: %%f ...
            "%GLSLC%" -fshader-stage=rgen "%%f" -DRAYGEN_SHADER --target-spv=spv1.4 -o "%%~dpfspv\%%~nfRgen.spv"
        )
        )
    )

)

echo ========== Shader Compile Finished ==========
pause
