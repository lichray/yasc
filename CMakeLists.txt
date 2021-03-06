cmake_minimum_required(VERSION 3.1.3)
project(yasd CXX)
enable_testing()

include(CTest)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY tests)
set(CMAKE_CXX_STANDARD 14)

if(MINGW)
	add_definitions(-DMINGW_HAS_SECURE_API)
endif()

if(NOT MSVC)
	add_definitions(-Wall -Wconversion -Wsign-conversion -Wsign-compare)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif()

include_directories(3rdparty/include)
include_directories(include)

file(GLOB yasd_srcs src/*.cc src/*.cpp)
file(GLOB tests_srcs tests/*.cc)

add_library(yasd ${yasd_srcs})

if(BUILD_TESTING)
	configure_file(tests/ctest_paths.h.in tests/ctest_paths.h)
	add_executable(run ${tests_srcs})
	target_include_directories(run PRIVATE ${CMAKE_BINARY_DIR}/tests)
	target_link_libraries(run yasd)
	add_test(testall tests/run)
endif()

file(GLOB demo_srcs demo/*.cc)

foreach(demo_src ${demo_srcs})
	get_filename_component(demo ${demo_src} NAME_WE)
	add_executable(${demo} ${demo_src})
	target_link_libraries(${demo} yasd)
	set_target_properties(${demo} PROPERTIES RUNTIME_OUTPUT_DIRECTORY demo)
endforeach()
