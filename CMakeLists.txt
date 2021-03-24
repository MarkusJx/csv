cmake_minimum_required(VERSION 3.15)
enable_testing()
project(csv)

set(CMAKE_CXX_STANDARD 17)

# Get the ExternalProject module
include(ExternalProject)

add_executable(csv test.cpp csv.hpp)

# Get and build gtest
ExternalProject_Add(gtest
        URL https://github.com/google/googletest/archive/release-1.10.0.tar.gz
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
        CMAKE_ARGS -Dgtest_force_shared_crt=ON -DBUILD_GMOCK=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        )

ExternalProject_Get_Property(gtest install_dir)
set(gtest_dir ${install_dir})

# Include gtest
include_directories(${gtest_dir}/include)

# Set link directories to gtest_dir/lib
target_link_directories(${PROJECT_NAME} PRIVATE ${gtest_dir}/lib)

# Add dependencies
add_dependencies(${PROJECT_NAME} gtest)

# Link test against gtest
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE gtest_maind gtestd)
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE gtest_main gtest pthread)
endif ()

add_test(NAME csvTest
        COMMAND csv --config $<CONFIG>
        --exe $<TARGET_FILE:csv>)