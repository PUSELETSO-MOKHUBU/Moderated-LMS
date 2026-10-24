@echo off
setlocal
cd /d "%~dp0"
cmake -S . -B build -DBUILD_TESTING=ON
if errorlevel 1 exit /b 1
cmake --build build --config Release
if errorlevel 1 exit /b 1
ctest --test-dir build -C Release --output-on-failure
if errorlevel 1 exit /b 1
call build\ModeratedLMS.exe
