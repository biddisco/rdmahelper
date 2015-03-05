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
find_package(Boost 1.50.0  REQUIRED 
   regex date_time serialization 
  program_options filesystem log log_setup system thread 
)
include_directories(${Boost_INCLUDE_DIR})

#------------------------------------------------------------------------------
# HPX 
#------------------------------------------------------------------------------
#find_package(HPX REQUIRED CONFIG)
#include_directories(${HPX_INCLUDE_DIRS})
#message("The HPX include dir is ${HPX_INCLUDE_DIRS}")

##------------------------------------------------------------------------------
# Logging
#------------------------------------------------------------------------------
option(RDMAHELPER_ENABLE_LOGGING "Enable logging" ON)
if (RDMAHELPER_ENABLE_LOGGING)
  add_config_define(RDMAHELPER_BOOST_LOGGING)
else()
  add_config_define(RDMAHELPER_DISABLE_LOGGING)
endif()

#------------------------------------------------------------------------------
# Detect Platform to setup special flags for specific machines
#------------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DetectMachine.cmake)

#------------------------------------------------------------------------------
# verbs support
#------------------------------------------------------------------------------
if(NOT BGQ)
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
else()
    add_config_define(RDMAHELPER_BGQ_CNVERBS)
    # this can be safely hardcoded
    set(BGSYS_includes "/bgsys/drivers/V1R2M2/ppc64/spi/include/kernel")
    include_directories(${BGSYS_includes})
endif()
    
#------------------------------------------------------------------------------
# Definitions 
#------------------------------------------------------------------------------
#add_definitions(${rdmahelper_DEFINITIONS})

################################################################################
# Configure the header that includes all compile definitions
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

#
# to #include "rdmahelper_defines.h"
#
include_directories(${CMAKE_CURRENT_BINARY_DIR})


#------------------------------------------------------------------------------
# RDMA Helper library 
#------------------------------------------------------------------------------
set(BGCIOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (NOT BGQ)
    set(BGCIOS_SRC
      ${BGCIOS_DIR}/RdmaMemoryRegion.cc
      ${BGCIOS_DIR}/RdmaDevice.cc
      ${BGCIOS_DIR}/RdmaCompletionChannel.cc
      ${BGCIOS_DIR}/RdmaCompletionQueue.cc
      ${BGCIOS_DIR}/RdmaProtectionDomain.cc
      ${BGCIOS_DIR}/RdmaConnection.cc
      ${BGCIOS_DIR}/RdmaClient.cc
      ${BGCIOS_DIR}/RdmaServer.cc
      ${BGCIOS_DIR}/RdmaSharedReceiveQueue.cc
      ${BGCIOS_DIR}/memory_pool.cpp 
    )
else()
    set(BGCIOS_SRC
      ${BGCIOS_DIR}/memory_pool.cpp 
      ${BGCIOS_DIR}/CNKMemoryRegion.cc
      ${BGCIOS_DIR}/CNKClient.cc
    )
endif()
 
set(BGCIOS_includes
  ${CMAKE_CURRENT_SOURCE_DIR}/include
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
