# Project implementing core logic for Steel Face Tools
# The library is independent from any DCC and is meant to be the
# core processing logic for various face deformation rigging tools
set(HEADERS
        SCorneaPush.h
        SStickyLips.h
)

set(SOURCES
        SCorneaPush.cpp
        SStickyLips.cpp
)

add_library(SteelTools STATIC ${SOURCES} ${HEADERS})
target_include_directories(SteelTools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(BUILD_PLAYGROUND "Build playground executable" OFF)
if(BUILD_PLAYGROUND)
    add_executable(SteelToolsPlayground devToolsPlayground.cpp)
    target_link_libraries(SteelToolsPlayground PRIVATE SteelTools)
endif()
