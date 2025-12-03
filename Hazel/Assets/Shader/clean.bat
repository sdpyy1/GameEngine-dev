@echo off
setlocal enabledelayedexpansion

echo ==============================================
echo [SPV Folder Cleanup Tool]
echo ==============================================
echo Warning: This will DELETE ALL "spv" folders and their contents
echo Skipped folders: All "common" directories
echo ==============================================
echo.
set /p "CONFIRM=Type '1' to continue (other inputs will exit): "

if /i not "!CONFIRM!"=="1" (
    echo.
    echo Cleanup cancelled by user.
    pause
    exit /b 0
)

echo.
echo Starting cleanup...
echo ==============================================

set "DELETE_ERROR=0"

:: 递归遍历所有目录，查找spv文件夹
for /r %%d in (spv) do (
    :: 检查当前spv文件夹的上级目录是否是common（或路径包含common）
    set "PARENT_DIR=%%~dpd"
    echo !PARENT_DIR! | findstr /i "\common\" >nul
    if not errorlevel 1 (
        echo Skipping spv in common folder: "%%d"
        continue
    )

    :: 检查是否是实际存在的文件夹
    if exist "%%d\" (
        echo Deleting: "%%d"
        :: 删除文件夹及所有内容（/s递归，/q静默）
        rmdir /s /q "%%d" 2>nul
        if errorlevel 1 (
            echo Error: Failed to delete "%%d"
            set "DELETE_ERROR=1"
        ) else (
            echo Success: Deleted "%%d"
        )
    )
)

echo.
echo ==============================================
if !DELETE_ERROR! equ 0 (
    echo [Cleanup completed successfully!]
) else (
    echo [Cleanup completed with errors!]
)
echo Note: All non-common spv folders have been removed
echo ==============================================