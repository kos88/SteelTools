# generate_mod.cmake
# This runs during install, with variables passed from main CMakeLists.txt

# Input variables (passed via -D)
# - MAYA_VERSION
# - PLATFORM
# - PLUGIN_NAME
# - MAYA_ROOT_PATH
# - CMAKE_INSTALL_PREFIX

# Generate the .mod file content
set(MOD_CONTENT "
+ MAYAVERSION:${MAYA_VERSION} PLATFORM:${PLATFORM} ${MODULE_NAME} any ${MAYA_ROOT_PATH}
plug-ins: plug-ins/${MAYA_VERSION}
")

# Write to file
set(MOD_FILE "${CMAKE_INSTALL_PREFIX}/${MODULE_NAME}.mod")
file(WRITE ${MOD_FILE} ${MOD_CONTENT})

message(STATUS "✅ Generated MOD file: ${MOD_FILE}")
message(STATUS "   Maya version: ${MAYA_VERSION}")