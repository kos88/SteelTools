@echo off
:: Enable delayed expansion to be able to use variables inside loops (is this for real? lol)
setlocal EnableDelayedExpansion

:: Check if cmake is installed
cmake --version >nul 2>&1

:: If not, raise an error
if %ERRORLEVEL% neq 0 (
    echo Error: CMake is not installed or not in the system PATH.
    exit /b 1
)


:: Define the path to CMake and Visual Studio versions
set GENERATOR="Visual Studio 17 2022"
set "BUILD_DIR_BASE=..\..\release_builds\cmake-build-release-"

:: Define the path to Inno Setup Compiler and the .iss file for building the installer after the binaries
set ISS_FILE="..\maya\installer\windows\steelToolsInstaller.iss"
set VERSION_FILE="..\maya\version.txt"
set ISCC_PATH="C:\Program Files (x86)\Inno Setup 6\ISCC.exe"

:: Define the Maya versions using a variable
set "MAYA_VERSIONS=2023 2024 2025"

:: Get the current directory
set "CURRENT_DIR=%CD%"

set "ROOT_CMAKE_DIR=%CURRENT_DIR%\.."

:: Iterate over each Maya version stored in the variable
for %%M in (%MAYA_VERSIONS%) do (
    echo Building for Maya version %%M

    :: Create the build directory for the specific Maya version
    set "BUILD_DIR=%BUILD_DIR_BASE%%%M"
    set "FULL_BUILD_PATH=%CURRENT_DIR%\!BUILD_DIR!"
    echo Creating build directory for %%M at !FULL_BUILD_PATH!

    :: Create the build directory if it doesn't exist, otherwise print that it exists
    if not exist "!BUILD_DIR!" (
        mkdir "!BUILD_DIR!"
        echo Created directory: !FULL_BUILD_PATH!
    ) else (
         echo Directory already exists: !FULL_BUILD_PATH!
    )

    :: Run CMake to configure the project
    echo Running CMake configuration for Maya %%M...
    cmake -G %GENERATOR% -DMAYA_VERSION=%%M -DBUILD_MAYA=TRUE -DCMAKE_BUILD_TYPE="Release" -S "%ROOT_CMAKE_DIR%" -B "!BUILD_DIR!"
    if !ERRORLEVEL! neq 0 (
        echo CMake configuration failed for Maya %%M
        exit /b !ERRORLEVEL!
    )

    :: Build the project
    echo Building project for Maya %%M...
    cmake --build "!BUILD_DIR!" --config Release --target install
    if !ERRORLEVEL! neq 0 (
        echo Build failed for Maya %%M
        exit /b !ERRORLEVEL!
    )

)



:: ------------------------------------------------------------------------------------------------------------------ ::
:: Read the txt file to get the version number
set /p VERSION=<!VERSION_FILE!


echo Building installer using Inno Setup with version !VERSION!
:: Compile the .iss file using Inno Setup Compiler
if exist %ISCC_PATH% (
    echo Compiling Inno Setup script...
    %ISCC_PATH% %ISS_FILE% /DMyVersion=%VERSION%
    if %errorlevel% neq 0 (
        echo Error: Failed to compile the Inno Setup script.
        exit /b 1
    ) else (
        echo Inno Setup script compiled successfully.
    )
) else (
    echo Error: Inno Setup Compiler not found at %ISCC_PATH%.
    exit /b 1
)

:: Disable delayed expansion
endlocal

