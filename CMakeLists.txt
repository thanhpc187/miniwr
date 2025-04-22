cmake_minimum_required(VERSION 3.16)
project(miniwr VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Find required packages
find_package(ZLIB REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Optional: Find LibArchive
find_package(LibArchive)

# Define source files
set(CORE_SOURCES
    src/core/Compressor.cpp
    src/core/DeflateCompressor.cpp
    src/core/TarWrapper.cpp
    src/core/ArchiveWriter.cpp
    src/core/ArchiveReader.cpp
)

set(UTIL_SOURCES
    src/util/FileSystem.cpp
    src/util/Buffer.cpp
    src/util/ProgressBar.cpp
)

set(CLI_SOURCES
    src/cli/ArgParser.cpp
    src/cli/CommandAdd.cpp
    src/cli/CommandExtract.cpp
)

# Main executable
add_executable(miniwr 
    src/main.cpp
    ${CORE_SOURCES}
    ${UTIL_SOURCES}
    ${CLI_SOURCES}
)

target_include_directories(miniwr PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ZLIB_INCLUDE_DIRS}
)

target_link_libraries(miniwr PRIVATE 
    ZLIB::ZLIB
    Threads::Threads
)

if(LibArchive_FOUND)
    target_compile_definitions(miniwr PRIVATE HAVE_LIBARCHIVE)
    target_link_libraries(miniwr PRIVATE LibArchive::LibArchive)
endif()

# Unit tests
enable_testing()
add_executable(unit_tests
    tests/test_main.cpp
    tests/test_compression.cpp
    tests/test_archive.cpp
    ${CORE_SOURCES}
    ${UTIL_SOURCES}
)

target_include_directories(unit_tests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${ZLIB_INCLUDE_DIRS}
)

target_link_libraries(unit_tests PRIVATE
    GTest::GTest
    GTest::Main
    ZLIB::ZLIB
    Threads::Threads
)

if(LibArchive_FOUND)
    target_compile_definitions(unit_tests PRIVATE HAVE_LIBARCHIVE)
    target_link_libraries(unit_tests PRIVATE LibArchive::LibArchive)
endif()

add_test(NAME unit_tests COMMAND unit_tests)

# Install rules
install(TARGETS miniwr
    RUNTIME DESTINATION bin) 