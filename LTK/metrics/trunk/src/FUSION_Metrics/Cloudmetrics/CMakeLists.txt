cmake_minimum_required(VERSION 2.6)

project (CloudMetrics CXX)

set (SRC_FILES Cloudmetrics.cpp)
set (HDR_FILES StdAfx.h)

find_package (Boost 1.39 COMPONENTS filesystem
                                    system)
link_directories (${Boost_LIBRARY_DIRS})
include_directories (${Boost_INCLUDE_DIRS})


#  Get the root directory for the metrics tools
get_filename_component(METRICS_TOOLS_DIR "${PROJECT_SOURCE_DIR}" PATH)

#  Get the root directory for all the source code (metrics tools and
#  shared code)
get_filename_component(ALL_SOURCE_ROOT "${METRICS_TOOLS_DIR}" PATH)

#  The directory with the code common to all the metric tools
set (COMMON_DIR ${ALL_SOURCE_ROOT}/common)

#  The directory with the FUSION utility code
set (FUSION_UTIL ${COMMON_DIR}/FUSION_util)

set (UTIL_FILENAMES argslib.cpp
                    argslib.h
                    CString.cpp
                    CString.h
                    DataCatalogEntry.cpp
                    DataCatalogEntry.h 
                    DataFile.cpp
                    DataFile.h
                    DataIndex.cpp
                    DataIndex.h
                    filespec.cpp
                    filespec.h
                    getApplicationPath.cpp
                    getApplicationPath.h
                    LASFormatFile.cpp
                    LASFormatFile.h
                    LidarData.cpp
                    LidarData.h
                    plansdtm.cpp
                    plansdtm.h
                    vc6_to_std.h )
foreach (name ${UTIL_FILENAMES})
  list (APPEND UTIL_SRCS ${FUSION_UTIL}/${name})
endforeach()
source_group("FUSION util" FILES ${UTIL_SRCS})


#  The directory with the code shared by the LTK programs
set (LTK_DIR ${COMMON_DIR}/LTK)

include_directories ( ${PROJECT_SOURCE_DIR} ${FUSION_UTIL} ${LTK_DIR} )

if (MSVC)
  #  Disable warning C4996 about unsafe functions in stdio.h
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set (PROGRAM_NAME Cloudmetrics)
add_executable (${PROGRAM_NAME} ${SRC_FILES} ${HDR_FILES} ${UTIL_SRCS})
target_link_libraries (${PROGRAM_NAME}
                       ${Boost_FILESYSTEM_LIBRARY}
                       ${Boost_SYSTEM_LIBRARY})

if (MSVC)
  #  Link with setargv.obj so wildcards on command line are expanded
  set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS setargv.obj)
endif()
