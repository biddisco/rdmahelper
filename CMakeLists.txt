project(rdmahelper C CXX)
cmake_minimum_required(VERSION 2.8)

#------------------------------------------------------------------------------
# CMake module path
#------------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/add_definition.cmake)

#------------------------------------------------------------------------------
# Boost 
#------------------------------------------------------------------------------
find_package(Boost 1.50.0  REQUIRED system regex date_time serialization program_options log thread filesystem)
include_directories(${Boost_INCLUDE_DIR})

#------------------------------------------------------------------------------
# Logging
#------------------------------------------------------------------------------
option(RDMAHELPER_ENABLE_LOGGING "Enable logging" ON)
if (RDMAHELPER_ENABLE_LOGGING)
  option(RDMAHELPER_USE_BOOST_LOG "Use boost log library (if off, use Log4CXX)" ON)
  if (RDMAHELPER_USE_BOOST_LOG)
    add_definitions(-DRDMAHELPER_BOOST_LOGGING)
    add_config_define(RDMAHELPER_BOOST_LOGGING)
  else()
    #------------------------------------------------------------------------------
    # Log4CXX 
    #------------------------------------------------------------------------------
    set(RDMAHELPER_USE_LOG4CXX 1)
    find_package(Log4Cxx)
    add_definitions(-DRDMAHELPER_LOG4CXX_LOGGING)
    add_config_define(RDMAHELPER_LOG4CXX_LOGGING)
    if(NOT Log4CXX_FOUND)
      add_definitions(-DRDMAHELPER_DISABLE_LOGGING)
      add_config_define(RDMAHELPER_DISABLE_LOGGING)
    endif()
  endif()
endif()

#------------------------------------------------------------------------------
# OFED verbs stack
#------------------------------------------------------------------------------
find_package(IB_VERBS REQUIRED)
include_directories(${IB_VERBS_INCLUDE_DIRS})

#------------------------------------------------------------------------------
# RDMA_CM
#------------------------------------------------------------------------------
find_package(RDMA_CM REQUIRED)
include_directories(${RDMA_CM_INCLUDE_DIRS})

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

################################################################################
# Configure the header to include all compile definitions
################################################################################
get_property(PROJECT_CONFIG_DEFINITIONS_VAR GLOBAL PROPERTY PROJECT_CONFIG_DEFINITIONS)

list(SORT PROJECT_CONFIG_DEFINITIONS_VAR)
list(REMOVE_DUPLICATES PROJECT_CONFIG_DEFINITIONS_VAR)

set(project_config_defines "\n")
foreach(def ${PROJECT_CONFIG_DEFINITIONS_VAR})
  set(project_config_defines "${project_config_defines}#define ${def} ${${def}_define}\n")#"
endforeach()

configure_file("${PROJECT_SOURCE_DIR}/cmake/config_defines.hpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/rdmahelper_defines.h"
               @ONLY)


#------------------------------------------------------------------------------
# RDMA Helper library 
#------------------------------------------------------------------------------
set(BGCIOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ramdisk/src/services/common)

set(BGCIOS_SRC
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
  ${MPI_C_INCLUDE_PATH}
)

if(RDMAHELPER_USE_LOG4CXX)
  set(BGCIOS_SRC
    ${BGCIOS_SRC}
    ${BGCIOS_DIR}/Cioslog.cc
  )
  set(BGCIOS_includes
    ${BGCIOS_includes}
    ${LOG4CXX_INCLUDE_DIR}
  )
endif()
   
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
  ${IB_VERBS_LIBRARIES}
  ${RDMA_CM_LIBRARIES}
  ${Boost_LIBRARIES}
  ${MPI_C_LIBRARIES}
)

if(RDMAHELPER_USE_LOG4CXX)
  target_link_libraries(rdmahelper
    ${LOG4CXX_LIBRARIES}
  )
endif()
   

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
