@echo off
echo DirectX 12 3D Platformer - Build Script
echo ========================================

REM Check if CMake is available
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found. Please install CMake and add it to PATH.
    pause
    exit /b 1
)

REM Create build directory
if not exist build mkdir build
cd build

REM Generate Visual Studio project files
echo Generating Visual Studio project files...
cmake .. -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake generation failed.
    pause
    exit /b 1
)

echo.
echo Build files generated successfully!
echo.
echo Next steps:
echo 1. Open build/DirectX12_3DPlatformer.sln in Visual Studio
echo 2. Build the solution (F7)
echo 3. Run the game (F5)
echo.
echo Or run: cmake --build . --config Release
echo.

pause
