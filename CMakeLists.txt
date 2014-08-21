project(rdma_lib C CXX)
cmake_minimum_required(VERSION 2.8)

if(BGQ OR BGAS)
  message("Setting BGQ/BGAS flags")
  add_definitions(-DBGQ_BGAS)
endif()

#------------------------------------------------------------------------------
# Simple RDMA library 
#------------------------------------------------------------------------------

#  find_package(Boost 1.50.0  REQUIRED system regex date_time serialization program_options thread filesystem)

  LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
  
  find_package(Log4Cxx)

  set(BGCIOS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ramdisk/src/services/common)

  set(BGCIOS_UTILITY_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/src/Log.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/src/Properties.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/src/LoggingProgramOptions.cc
  )
	
  set(BGCIOS_SRC
	  ${BGCIOS_DIR}/Cioslog.cc
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

  set(BGC_includes
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utility/include

    ${_ofed_INCLUDE_DIR}/rdma
    ${LOG4CXX_INCLUDE_DIR}
    ${MPI_C_INCLUDE_PATH}
  )
   
  set_global_variable(BGC_INCLUDE_DIR
    "${BGC_includes}" 
  )
  
  include_directories(
    ${BGC_includes}
  )
      
  link_directories(
    ${SLURM_LIBRARY_PATH}
    ${MPICH_ROOT}/lib
  )
  
  add_library(rdma_lib
    ${BGCIOS_UTILITY_SRC}
    ${BGCIOS_SRC} 
  )
  
  target_link_libraries(rdma_lib
    ${LOG4CXX_LIBRARIES}
    ${_rdma_LIBRARY}
    ${_ibverbs_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG}
    ${Boost_SYSTEM_LIBRARY_DEBUG}
    ${Boost_THREAD_LIBRARY_DEBUG}
    ${Boost_FILESYSTEM_LIBRARY_DEBUG}
    ${MPI_C_LIBRARIES}
  )

  #  set_target_properties(ofedtest 
  #                        PROPERTIES COMPILE_DEFINITIONS
  #                        "-std=c++0x" )

set(BGCIOS_EXPORTED_TARGETS "bgq-targets") 

#---------------------------------------------------------------------------
# Add Target(s) to CMake Install
#---------------------------------------------------------------------------
install(
  TARGETS
    rdma_lib
  EXPORT
    ${BGCIOS_EXPORTED_TARGETS}
  LIBRARY DESTINATION ${BGQ_INSTALL_LIB_DIR}
  ARCHIVE DESTINATION ${BGQ_INSTALL_LIB_DIR}
  RUNTIME DESTINATION ${BGQ_INSTALL_BIN_DIR}
)

