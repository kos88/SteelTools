set(HEADERS
    CorneaPushNode.h
    StickyLipsNode.h
)

set(SOURCES
    CorneaPushNode.cpp
    StickyLipsNode.cpp
)

# If no maya version is set, raise an error
if(NOT DEFINED MAYA_VERSION)
    message(FATAL_ERROR "MAYA_VERSION is not defined. Please set like: -DMAYA_VERSION=2022")
else ()
    message(STATUS "MAYA_VERSION: ${MAYA_VERSION}")
endif()

find_package(Maya REQUIRED)

set(MODULE_NAME SteelToolsMaya)  # This is the name that will be used for the target
add_library(${MODULE_NAME} SHARED ${SOURCES})
set_target_properties(${MODULE_NAME} PROPERTIES SUFFIX ".mll" LINKER_LANGUAGE CXX)
set_property(TARGET ${MODULE_NAME} PROPERTY CXX_STANDARD 23)

# Add the paths where the includes are
target_include_directories(${MODULE_NAME}
        PRIVATE
        ${MAYA_INCLUDE_DIR}
        ${PLUGIN_ROOT}/extern
)

# Set options like preprocessor definitions
target_compile_definitions(${MODULE_NAME} PRIVATE -DWIN32 -D_USE_MATH_DEFINES -DMAYA_EXPORTS -DNT_PLUGIN -DREQUIRE_IOSTREAM)

# Specify in which directories the linker can find the libs
target_link_directories(${MODULE_NAME} PUBLIC ${MAYA_LIBRARY_DIR})  # libs of the found maya to link
message(STATUS "Maya libraries are in: ${MAYA_LIBRARY_DIR}")
# .. other libs...

# Link maya libs, or other libs if needed
set(MAYA_LIBRARIES
        "OpenMayaUI.lib"
        "OpenMayaRender.lib"
        "OpenMaya.lib"
        "OpenMayaAnim.lib"
        "Foundation.lib")

target_link_libraries(${MODULE_NAME} PUBLIC ${MAYA_LIBRARIES} SteelTools)

# Install the maya file and pmp in the right directories
set(MAYA_MLL_OUTPUT ${CMAKE_INSTALL_PREFIX}/maya/plug-ins/${MAYA_VERSION})  # destination for the maya file
install(TARGETS ${MODULE_NAME}
        RUNTIME DESTINATION
        ${MAYA_MLL_OUTPUT})
message(STATUS "maya plugin will be installed to: ${MAYA_MLL_OUTPUT}")