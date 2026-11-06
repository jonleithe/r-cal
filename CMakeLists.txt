cmake_minimum_required(VERSION 3.16)
project(r_cal VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NCURSESW REQUIRED IMPORTED_TARGET ncursesw)
find_package(SQLite3 REQUIRED)
if(NOT TARGET SQLite3::SQLite3)
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif()

add_library(rcal_header src/RCalHeaderFramework.cpp src/ColorSelector.cpp
    src/CreateItemSelector.cpp src/NoteEditor.cpp src/SQLiteCalendarStore.cpp)
target_include_directories(rcal_header PUBLIC include)
target_link_libraries(rcal_header PUBLIC PkgConfig::NCURSESW SQLite3::SQLite3)
target_compile_definitions(rcal_header PUBLIC _XOPEN_SOURCE_EXTENDED)
target_compile_options(rcal_header PRIVATE -Wall -Wextra -Wpedantic)

add_executable(r-cal main.cpp)
target_link_libraries(r-cal PRIVATE rcal_header)

include(CTest)
if(BUILD_TESTING)
    add_executable(rcal_header_test tests/RCalHeaderFrameworkTest.cpp)
    target_link_libraries(rcal_header_test PRIVATE rcal_header)
    target_compile_definitions(rcal_header_test PRIVATE RCAL_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME rcal_header_test COMMAND rcal_header_test)

    add_executable(sqlite_calendar_store_test tests/SQLiteCalendarStoreTest.cpp)
    target_link_libraries(sqlite_calendar_store_test PRIVATE rcal_header)
    add_test(NAME sqlite_calendar_store_test COMMAND sqlite_calendar_store_test)
endif()
