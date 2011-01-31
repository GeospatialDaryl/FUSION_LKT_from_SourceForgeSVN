cmake_minimum_required (VERSION 2.6)

project (FUSION_UTIL_UNIT_TESTS CXX)

find_package (Boost 1.39 COMPONENTS unit_test_framework)

link_directories (${Boost_LIBRARY_DIRS})
include_directories (${Boost_INCLUDE_DIRS})

# Collect test source code
file (GLOB TEST_SOURCES *.cpp *.h)
list (SORT TEST_SOURCES)
source_group("Test Sources" FILES ${TEST_SOURCES})

# Get the root directory for the FUSION metrics project
get_filename_component(FUSION_METRICS_ROOT "${PROJECT_SOURCE_DIR}" PATH)
message("FUSION_METRICS_ROOT = ${FUSION_METRICS_ROOT}")

# Get the directory with FUSION utility source files.
set (FUSION_UTIL_SRC_DIR  ${FUSION_METRICS_ROOT}/src/common/FUSION_util)
include_directories (${FUSION_UTIL_SRC_DIR})

set (UTIL_SRC_NAMES CString.cpp
                    CString.h )
foreach (name ${UTIL_SRC_NAMES})
  list (APPEND FUSION_UTIL_SOURCES ${FUSION_UTIL_SRC_DIR}/${name})
endforeach()
source_group("FUSION Util Sources" FILES ${FUSION_UTIL_SOURCES})

if (MSVC)
  #  Disable warning C4996 about unsafe string functions
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set (PROGRAM_NAME fusion-util-unit-tests)
add_executable (${PROGRAM_NAME} ${TEST_SOURCES}
                                ${FUSION_UTIL_SOURCES})
target_link_libraries (${PROGRAM_NAME}
                       ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

# Create a command script for Windows to run the tests
if (WIN32)
  configure_file (
    ${PROJECT_SOURCE_DIR}/run-tests.cmd.in
    ${PROJECT_BINARY_DIR}/run-tests.cmd)
endif()
