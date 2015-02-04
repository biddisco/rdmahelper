#ifndef RDMAHELPER_LOGGING_H_
#define RDMAHELPER_LOGGING_H_

#include "rdmahelper_defines.h"

#ifndef RDMAHELPER_DISABLE_LOGGING

  #ifdef RDMAHELPER_BOOST_LOGGING
    #include <ostream>
    #include <sstream>
    #include <memory>
    #include <string>
    #include <boost/log/trivial.hpp>

    #define hexpointer(p) "0x" << std::setfill('0') << std::setw(12) << std::hex << (uintptr_t)(p) << " "
    #define hexlength(p)  "0x" << std::setfill('0') << std::setw( 6) << std::hex << (uintptr_t)(p) << " "

    #define LOG_DECLARE_FILE(f)

    #define LOG_TRACE_MSG(x) BOOST_LOG_TRIVIAL(trace)   << x;
    #define LOG_DEBUG_MSG(x) BOOST_LOG_TRIVIAL(debug)   << x;
    #define LOG_INFO_MSG(x)  BOOST_LOG_TRIVIAL(info)    << x;
    #define LOG_WARN_MSG(x)  BOOST_LOG_TRIVIAL(warning) << x;
    #define LOG_ERROR_MSG(x) BOOST_LOG_TRIVIAL(error)   << x;
    #define LOG_FATAL_MSG(x) BOOST_LOG_TRIVIAL(fatal)   << x;


    #define LOG_CIOS_DEBUG_MSG(x) BOOST_LOG_TRIVIAL(debug)   << x;
    #define LOG_CIOS_INFO_MSG(x)  BOOST_LOG_TRIVIAL(info)    << x;
    #define LOG_CIOS_TRACE_MSG(x) BOOST_LOG_TRIVIAL(trace)    << x;

    #define BGV_RDMADROP 1
    #define BGV_RDMA_REG 2
    #define BGV_RDMA_RMV 3
    #define BGV_WORK_CMP 4
    #define BGV_RECV_EVT 5
    #define CIOSLOGRDMA_REQ(ID,region,frags,fd) BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGRDMA_REQ " << ID << " " << region << " " << frags << " " << fd;
    #define CIOSLOGMSG_WC(ID,wc)                BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGMSG_WC   " << ID << " " << ((struct ibv_wc *)wc)->wr_id;
    #define CIOSLOGPOSTSEND(ID,send_wr,err)     BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGPOSTSEND " << ID << " " << send_wr.wr_id << " " << err;
    #define CIOSLOGEVT_CH(ID,event)             BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGEVT_CH   " << ID << " " << event;


  #elif RDMAHELPER_LOG4CXX_LOGGING
    #include <ramdisk/include/services/logging.h>
  #else
    #pragma error("Either boost or log4cxx should be defined")
  #endif

#else
  #include <sstream>

  #define LOG_DECLARE_FILE(x)

  #define LOG_DEBUG_MSG(x)
  #define LOG_TRACE_MSG(x)
  #define LOG_INFO_MSG(x)
  #define LOG_WARN_MSG(x)
  #define LOG_ERROR_MSG(x)

  #define LOG_CIOS_DEBUG_MSG(x)
  #define LOG_CIOS_INFO_MSG(x)
  #define LOG_CIOS_TRACE_MSG(x)
  #define CIOSLOGEVT_CH(x,y)

  #define CIOSLOGMSG_WC(x,y)
  #define CIOSLOGRDMA_REQ(x,y,z,a)

#endif

#endif
