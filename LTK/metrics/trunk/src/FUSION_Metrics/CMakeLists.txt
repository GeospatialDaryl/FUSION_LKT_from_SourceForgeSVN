cmake_minimum_required(VERSION 2.6)

project (FUSION_Metrics CXX)

if (WIN32)
  #  The FindBoost module looks for dynamic libraries by default.  But Boost's
  #  auto-linking on Windows uses static libraries by default.  These two
  #  defaults will cause linker errors about multiple definitions of symbols.
  #  So tell FindBoost to look for static libraries.
  set (Boost_USE_STATIC_LIBS ON)
endif()
find_package (Boost 1.39 COMPONENTS filesystem
                                    system)
link_directories (${Boost_LIBRARY_DIRS})
include_directories (${Boost_INCLUDE_DIRS})


#  Set the root directory for the metrics tools
set (METRICS_TOOLS_DIR "${PROJECT_SOURCE_DIR}")

#  Get the root directory for all the source code (metrics tools and
#  shared code)
get_filename_component(ALL_SOURCE_ROOT "${METRICS_TOOLS_DIR}" PATH)


if (CMAKE_SIZEOF_VOID_P EQUAL 4)
  set (SYSTEM_BIT_SIZE 32)
else()
  set (SYSTEM_BIT_SIZE 64)
endif()
message ("${SYSTEM_BIT_SIZE}-bit system")

# find the SIGAR library
if (APPLE)
  if (SYSTEM_BIT_SIZE EQUAL 32)
    set (SIGAR_NAME sigar-universal-macosx)
  else()
    set (SIGAR_NAME sigar-universal64-macosx)
  endif()
elseif (WIN32)
  if (SYSTEM_BIT_SIZE EQUAL 32)
    set (SIGAR_NAME sigar-x86-winnt)
  else()
    set (SIGAR_NAME sigar-amd64-winnt)
  endif()
else()
  message (FATAL ERROR "Only have SIGAR libraries for OS X and Windows.")
endif()

set (SIGAR_DIR ${ALL_SOURCE_ROOT}/third-party/SIGAR)
find_library (SIGAR_LIBRARY ${SIGAR_NAME} ${SIGAR_DIR}/lib)
include_directories (${SIGAR_DIR}/include)


#  The directory with the code common to all the metric tools
set (COMMON_DIR ${ALL_SOURCE_ROOT}/common)

#  The directory with the FUSION utility code
set (FUSION_UTIL ${COMMON_DIR}/FUSION_util)

#  The directory with the code shared by the LTK programs
set (LTK_DIR ${COMMON_DIR}/LTK)

include_directories ( ${FUSION_UTIL} ${LTK_DIR} )

if (MSVC)
  #  Disable warning C4996 about unsafe functions in stdio.h
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory (Cloudmetrics)
add_subdirectory (GridMetrics)

#-----------------------------------------------------------------------------
# Packaging

if (MSVC)
  install (PROGRAMS package/win/set_path.bat DESTINATION bin)
 
  include (InstallRequiredSystemLibraries)

  set (DOC_FILE "Documentation.txt")
  set (PROGRAM_DOC "${PROJECT_BINARY_DIR}/${DOC_FILE}")
  configure_file("${PROJECT_SOURCE_DIR}/package/win/${DOC_FILE}" "${PROGRAM_DOC}")
  install (FILES "${PROGRAM_DOC}" DESTINATION doc)
endif()

set (LICENSE_FILE "${PROJECT_SOURCE_DIR}/package/LICENSE.txt")
if (MSVC)
  #  Make sure the license has Windows line endings for Notepad.
  set (LICENSE_FILE_WINDOWS "${PROJECT_BINARY_DIR}/LICENSE.txt")
  configure_file("${LICENSE_FILE}" "${LICENSE_FILE_WINDOWS}")
  install (FILES "${LICENSE_FILE_WINDOWS}" DESTINATION .)
endif()

set (VERSION_MAJOR 0)
set (VERSION_MINOR 82)
set (VERSION_PATCH 0)
set (VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set (VERSION_RELEASE_ABBR "${VERSION}")

set (APP_NAME "FUSION Metrics")
string (REGEX REPLACE " " "-" APP_NAME_NO_SPACES "${APP_NAME}")
set (CPACK_PACKAGE_VERSION              ${VERSION})
set (CPACK_PACKAGE_VERSION_MAJOR        ${VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR        ${VERSION_MINOR})
set (CPACK_PACKAGE_FILE_NAME            "${APP_NAME_NO_SPACES}-${VERSION_RELEASE_ABBR}-setup")
set (CPACK_PACKAGE_INSTALL_DIRECTORY    "${APP_NAME} ${VERSION_RELEASE_ABBR}")
set (CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${APP_NAME} ${VERSION_RELEASE_ABBR}")
set (CPACK_RESOURCE_FILE_LICENSE        "${LICENSE_FILE}")

set (CPACK_OPTIONS_FILE "${PROJECT_BINARY_DIR}/CPackOptions.cmake")
configure_file ("${PROJECT_SOURCE_DIR}/package/CPackOptions.cmake.in"
                "${CPACK_OPTIONS_FILE}"
                @ONLY)
set (CPACK_PROJECT_CONFIG_FILE "${CPACK_OPTIONS_FILE}")

include(CPack)
