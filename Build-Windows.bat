@echo off
setlocal
where cmake >nul 2>nul
if errorlevel 1 (
 echo CMake is required. Install CMake and Visual Studio 2022 with Desktop development with C++.
 pause
 exit /b 1
)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto failed
cmake --build build --config Release --target VOGPolySynthPro_VST3 VOGPolySynthPro_Standalone
if errorlevel 1 goto failed
echo.
echo Build complete. Find the .vst3 and .exe under build\VOGPolySynthPro_artefacts\Release\
echo Copy the .vst3 bundle to C:\Program Files\Common Files\VST3\
pause
exit /b 0
:failed
echo Build failed. Read the error above and install missing build dependencies.
pause
exit /b 1
