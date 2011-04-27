set (SRC_FILES GridMetrics.cpp)

set (UTIL_FILENAMES argslib.cpp
                    argslib.h
                    CString.cpp
                    CString.h
                    DataFile.cpp
                    DataFile.h
                    DataIndex.cpp
                    DataIndex.h
                    DeleteFile.cpp
                    DeleteFile.h
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


# The directory with template for 2-dimensional arrays
set (ARRAY2D_DIR ${COMMON_DIR}/array2d)
include_directories (${ARRAY2D_DIR})
set (ARRAY2D_HDR ${ARRAY2D_DIR}/Array2DTemplate.h)
source_group("2D Array" FILES ${ARRAY2D_HDR})

#  The directory with TerrainSet code
set (TERRAIN_SET_DIR ${COMMON_DIR}/TerrainSet)
include_directories (${TERRAIN_SET_DIR})

foreach (name TerrainSet.cpp TerrainSet.h)
  list (APPEND TERRAIN_SET_SRCS ${TERRAIN_SET_DIR}/${name})
endforeach()
source_group("Terrain Set" FILES ${TERRAIN_SET_SRCS})


set (PROGRAM_NAME Gridmetrics)
add_executable (${PROGRAM_NAME} ${SRC_FILES} ${UTIL_SRCS} ${ARRAY2D_HDR} ${TERRAIN_SET_SRCS})
target_link_libraries (${PROGRAM_NAME}
                       ${Boost_FILESYSTEM_LIBRARY}
                       ${Boost_SYSTEM_LIBRARY}
                       ${SIGAR_LIBRARY})

if (MSVC)
  #  Link with setargv.obj so wildcards on command line are expanded
  set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS setargv.obj)
endif()

#-----------------------------------------------------------------------------
# Packaging

install (TARGETS ${PROGRAM_NAME} DESTINATION bin)
