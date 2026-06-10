# Determine if this is debug build
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    set(IS_DEV TRUE)
    set(PYTHON_PATH "${CMAKE_SOURCE_DIR}/maya/scripts")
    message(STATUS "Debug mode - Python path: ${PYTHON_PATH}")
else()
    # Release or RelWithDebInfo - copy scripts
    set(IS_DEV FALSE)
    set(PYTHON_PATH "${CMAKE_INSTALL_PREFIX}/maya/scripts")
    
    set(SCRIPT_SOURCE "${CMAKE_SOURCE_DIR}/maya/scripts")

    message(STATUS "Copying scripts from: ${SCRIPT_SOURCE}")
    message(STATUS "Copying scripts to: ${PYTHON_PATH}")

    file(MAKE_DIRECTORY ${PYTHON_PATH})
    file(COPY ${SCRIPT_SOURCE}/ DESTINATION ${PYTHON_PATH})

    message(STATUS "Release mode - Python path: ${PYTHON_PATH}")
endif()

# Generate the .mod file content
set(MOD_CONTENT "
+ MAYAVERSION:${MAYA_VERSION} PLATFORM:${PLATFORM} ${MODULE_NAME} any ${MAYA_ROOT_PATH}
plug-ins: plug-ins/${MAYA_VERSION}
PYTHONPATH += ${PYTHON_PATH}
")

# Write to file
set(MOD_FILE "${CMAKE_INSTALL_PREFIX}/${MODULE_NAME}.mod")
file(WRITE ${MOD_FILE} ${MOD_CONTENT})

message(STATUS "Generated MOD file: ${MOD_FILE}")
message(STATUS "Maya version: ${MAYA_VERSION}")
message(STATUS "Python path (appended): ${PYTHON_PATH}")