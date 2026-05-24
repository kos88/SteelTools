set(HEADERS
        SCorneaPush.h
        SStickyLips.h
)

set(SOURCES
        SCorneaPush.cpp
        SStickyLips.cpp
)

set(MODULE_NAME SteelTools)
add_library(${MODULE_NAME} SHARED ${SOURCES})
option(BUILD_PLAYGROUND OFF)
message(STATUS "Build playground executable ${BUILD_PLAYGROUND}")

if(BUILD_PLAYGROUND)
add_executable(${MODULE_NAME}
    devToolsPlayground.cpp
    )
endif()
