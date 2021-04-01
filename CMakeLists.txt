cmake_minimum_required(VERSION 3.15)
project(csv)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

# Get the ExternalProject module
include(ExternalProject)

if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # lots of warnings
    add_compile_options(-Wall -Wextra -pedantic)
endif()

include_directories(include)

SET(SRC include/definitions.hpp include/exceptions.hpp include/util.hpp include/escape_sequence_generator.hpp
        include/csv_cell.hpp include/const_csv_row.hpp include/csv_row.hpp include/basic_csv_file_def.hpp
        include/basic_csv.hpp include/basic_csv_file.hpp include/csv.hpp include/index_iterator.hpp)
add_executable(${PROJECT_NAME} test.cpp ${SRC})

# Get and build gtest
ExternalProject_Add(gtest_project
        URL https://github.com/google/googletest/archive/release-1.10.0.tar.gz
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
        CMAKE_ARGS -Dgtest_force_shared_crt=ON -DBUILD_GMOCK=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        )

ExternalProject_Get_Property(gtest_project install_dir)
set(gtest_dir ${install_dir})

# Include gtest
include_directories(${gtest_dir}/include)

# Set link directories to gtest_dir/lib
target_link_directories(${PROJECT_NAME} PRIVATE ${gtest_dir}/lib)

# Add dependencies
add_dependencies(${PROJECT_NAME} gtest_project)

# Link test against gtest
if (WIN32 AND ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME} PRIVATE gtest_maind gtestd)
else ()
    if (WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE gtest_main gtest)
    else ()
        target_link_libraries(${PROJECT_NAME} PRIVATE gtest_main gtest pthread)
    endif ()
endif ()

add_test(NAME csvTest
        COMMAND csv --config $<CONFIG>
        --exe $<TARGET_FILE:csv>)