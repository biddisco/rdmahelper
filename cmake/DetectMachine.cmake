#------------------------------------------------------------------------------
# Machine check, see if we can tell if we are we running on BGQ or BGAS
#------------------------------------------------------------------------------
SITE_NAME(hostname)
if(hostname MATCHES bbpbg1)
  set(BGQ 1)
  message("Running on BGQ")
elseif(hostname MATCHES bbpbg2)
  set(BGAS 1)
  message("Running on BGAS")
elseif(hostname MATCHES bbpviz1)
  set(BGVIZ 1)
  message("Running on VIZ")
endif()
message("CMAKE_SYSTEM is ${CMAKE_SYSTEM} with hostname ${hostname} and processor ${CMAKE_SYSTEM_PROCESSOR}" )
