@echo off
setlocal

set "FXC=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\fxc.exe"
set "INCLUDE_DIR=."
set "OUTPUT_DIR=."

if not exist "%FXC%" (
    echo [ERROR] fxc.exe not found: "%FXC%"
    exit /b 1
)

echo ==============================
echo Compiling HLSL Shaders to CSO
echo ==============================

for %%f in (*_VS.hlsl) do (
    echo [VS] %%f
    "%FXC%" /nologo /Zpr /T vs_5_0 /E VS_Main /I "%INCLUDE_DIR%" /Fo "%OUTPUT_DIR%\%%~nf.cso" "%%f"
    if errorlevel 1 (
        echo [ERROR] Failed: %%f
        exit /b 1
    )
)

for %%f in (*_PS.hlsl) do (
    echo [PS] %%f
    "%FXC%" /nologo /Zpr /T ps_5_0 /E PS_Main /I "%INCLUDE_DIR%" /Fo "%OUTPUT_DIR%\%%~nf.cso" "%%f"
    if errorlevel 1 (
        echo [ERROR] Failed: %%f
        exit /b 1
    )
)

echo ==============================
echo Done.
exit /b 0