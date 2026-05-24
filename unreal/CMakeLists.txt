# Unreal Engine integration for SteelFaceTools
# Unreal typically uses its own build system (UnrealBuildTool), 
# but we can provide a CMake wrapper or helper to link our core library.

set(MODULE_NAME SteelToolsUnreal)

# This is a placeholder. Unreal integration usually involves 
# a .Build.cs file and specific directory structure.
# Here we just define a target that links to the core for reference.

add_library(${MODULE_NAME} INTERFACE)
target_link_libraries(${MODULE_NAME} INTERFACE SteelTools)

message(STATUS "Unreal integration target defined. Note: Unreal usually uses UBT.")
