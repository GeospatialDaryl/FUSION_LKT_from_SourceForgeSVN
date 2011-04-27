set (SRC_FILES Cloudmetrics.cpp)
set (HDR_FILES StdAfx.h)

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


set (PROGRAM_NAME Cloudmetrics)
add_executable (${PROGRAM_NAME} ${SRC_FILES} ${HDR_FILES} ${UTIL_SRCS})
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
