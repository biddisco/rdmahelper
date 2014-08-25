project(rdmahelper C CXX)
cmake_minimum_required(VERSION 2.8)

#------------------------------------------------------------------------------
# CMake module path
#------------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#------------------------------------------------------------------------------
# Log4CXX 
#------------------------------------------------------------------------------
find_package(Log4Cxx)
if(NOT Log4CXX_FOUND)
  add_definitions(-DDISABLE_LOGGING)
endif()

# We don't want to build the entire BGQ stack, so we override a bunch of
# logging stuff with this #define
set(rdmahelper_DEFINITIONS ${rdmahelper_DEFINITIONS} "-DSIMPLIFIED_CIOSLOG")

#------------------------------------------------------------------------------
# Boost 
#------------------------------------------------------------------------------
find_package(Boost 1.50.0  REQUIRED system regex date_time serialization program_options thread filesystem)
include_directories(${Boost_INCLUDE_DIR})

#------------------------------------------------------------------------------
# OFED verbs stack
#------------------------------------------------------------------------------
find_package(OFED REQUIRED)
include_directories(${OFED_INCLUDE_DIRS})

#------------------------------------------------------------------------------
# Detect Platform to setup special flags for specific machines
#------------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DetectMachine.cmake)

if(BGQ OR BGAS)
  message("Setting BGQ/BGAS flags")
  add_definitions(-DBGQ_BGAS)
endif()

#------------------------------------------------------------------------------
# Definitions 
#------------------------------------------------------------------------------
add_definitions(${rdmahelper_DEFINITIONS})

#------------------------------------------------------------------------------
# RDMA Helper library 
#------------------------------------------------------------------------------
set(BGCIOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ramdisk/src/services/common)

set(BGCIOS_SRC
#  ${BGCIOS_DIR}/Cioslog.cc
  ${BGCIOS_DIR}/logging.cc
  ${BGCIOS_DIR}/RdmaMemoryRegion.cc
  ${BGCIOS_DIR}/RdmaDevice.cc
  ${BGCIOS_DIR}/RdmaCompletionChannel.cc
  ${BGCIOS_DIR}/RdmaCompletionQueue.cc
  ${BGCIOS_DIR}/RdmaProtectionDomain.cc
  ${BGCIOS_DIR}/RdmaConnection.cc
  ${BGCIOS_DIR}/RdmaClient.cc
  ${BGCIOS_DIR}/RdmaServer.cc
)

set(BGCIOS_includes
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/utility/include

  ${_ofed_INCLUDE_DIR}/rdma
  ${LOG4CXX_INCLUDE_DIR}
  ${MPI_C_INCLUDE_PATH}
)
   
include_directories(
  ${BGCIOS_includes}
)

link_directories(
  ${SLURM_LIBRARY_PATH}
  ${MPICH_ROOT}/lib
)

add_library(rdmahelper
  ${BGCIOS_UTILITY_SRC}
  ${BGCIOS_SRC} 
)

target_link_libraries(rdmahelper
  ${LOG4CXX_LIBRARIES}
  ${_rdmahelperRARY}
  ${_ibverbs_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG}
  ${Boost_SYSTEM_LIBRARY_DEBUG}
  ${Boost_THREAD_LIBRARY_DEBUG}
  ${Boost_FILESYSTEM_LIBRARY_DEBUG}
  ${MPI_C_LIBRARIES}
)

if (NOT rdmahelper_EXPORTED_TARGETS)
  set(rdmahelper_EXPORTED_TARGETS "rdmahelper-targets") 
endif()

#---------------------------------------------------------------------------
# Add Target(s) to CMake Install
#---------------------------------------------------------------------------
install(
  TARGETS
    rdmahelper
  EXPORT
    ${rdmahelper_EXPORTED_TARGETS}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib/static
)

#---------------------------------------------------------------------------
# Create a config file for the build dir 
#---------------------------------------------------------------------------
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.build.in 
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
)

#---------------------------------------------------------------------------
# Generate a targets file for the build dir
#---------------------------------------------------------------------------
export(
  TARGETS rdmahelper
  FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake
)
