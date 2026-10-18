@echo off
echo ===========================================
echo Building Omniroute Assistant (MSVC 2022)
echo ===========================================

REM Setup MSVC Environment Variables
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

echo.
echo Preparing build directory...
if not exist "build" mkdir build
cd build

echo.
echo Cleaning CMake cache (if any MinGW residue exists)...
del /F /S /Q CMakeCache.txt
rmdir /S /Q CMakeFiles

echo.
echo Configuring CMake...
REM Adjust CMAKE_PREFIX_PATH if Qt is located elsewhere
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.4.0/msvc2019_64" ..

echo.
echo Building...
cmake --build . --config Release

echo.
echo Done!
pause
