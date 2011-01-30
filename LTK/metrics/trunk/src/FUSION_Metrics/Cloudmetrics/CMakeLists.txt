cmake_minimum_required(VERSION 2.6)

project (CLOUDMETRICS CXX)

set (SRC_FILES Cloudmetrics.cpp)
set (HDR_FILES StdAfx.h)


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
                    DataCatalogEntry.cpp
                    DataCatalogEntry.h 
                    DataFile.cpp
                    DataFile.h
                    DataIndex.cpp
                    DataIndex.h
                    filespec.cpp
                    filespec.h
                    LASFormatFile.cpp
                    LASFormatFile.h
                    LidarData.cpp
                    LidarData.h
                    plansdtm.cpp
                    plansdtm.h )
foreach (name ${UTIL_FILENAMES})
  list (APPEND UTIL_SRCS ${FUSION_UTIL}/${name})
endforeach()
source_group("FUSION util" FILES ${UTIL_SRCS})


include_directories ( ${PROJECT_SOURCE_DIR} ${FUSION_UTIL} )


set (PROGRAM_NAME Cloudmetrics)
add_executable (${PROGRAM_NAME} ${SRC_FILES} ${HDR_FILES} ${UTIL_SRCS})
