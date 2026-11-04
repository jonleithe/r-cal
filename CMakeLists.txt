cmake_minimum_required(VERSION 3.16)
project(r_cal VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)

add_library(rcal_header src/RCalHeaderFramework.cpp)
target_include_directories(rcal_header PUBLIC include PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(rcal_header PUBLIC ${CURSES_LIBRARIES})
target_compile_options(rcal_header PRIVATE -Wall -Wextra -Wpedantic)

add_executable(rcal_header_demo examples/rcal_header_demo.cpp)
target_link_libraries(rcal_header_demo PRIVATE rcal_header)

include(CTest)
if(BUILD_TESTING)
    add_executable(rcal_header_test tests/RCalHeaderFrameworkTest.cpp)
    target_link_libraries(rcal_header_test PRIVATE rcal_header)
    target_compile_definitions(rcal_header_test PRIVATE RCAL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME rcal_header_test COMMAND rcal_header_test)
endif()
