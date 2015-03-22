// Copyright (c) 2014-2015 John Biddiscombe
// Copyright (c) 2011,2012 IBM Corp.
//
// ================================================================
// Portions of this code taken from IBM BlueGene-Q source
// 
// This software is available to you under the
// Eclipse Public License (EPL).
//
// Please refer to the file "eclipse-1.0.txt" 
// ================================================================
//
#ifndef RDMAHELPER_LOGGING_H_
#define RDMAHELPER_LOGGING_H_

#include "rdmahelper/rdmahelper_defines.h"
#include <ostream>
#include <iomanip>
#include <sstream>
#include <memory>
#include <string>

//
// useful macros for formatting messages
//
#define hexpointer(p) "0x" << std::setfill('0') << std::setw(12) << std::hex << (uintptr_t)(p) << " "
#define hexlength(p)  "0x" << std::setfill('0') << std::setw( 6) << std::hex << (uintptr_t)(p) << " "
#define hexnumber(p)  "0x" << std::setfill('0') << std::setw( 4) << std::hex << p << " "
#define decnumber(p)  "" << std::dec << p << " "
#define ipaddress(p)  "" << std::dec << (int) ((uint8_t*) &p)[0] << "." << (int) ((uint8_t*) &p)[1] << \
    "." << (int) ((uint8_t*) &p)[2] << "." << (int) ((uint8_t*) &p)[3] << " "

//
// Logging disabled, #define all macros to be empty
//
#ifdef RDMAHELPER_DISABLE_LOGGING
#  include <sstream>

#  define LOG_DECLARE_FILE(x)

#  define LOG_DEBUG_MSG(x)
#  define LOG_TRACE_MSG(x)
#  define LOG_INFO_MSG(x)
#  define LOG_WARN_MSG(x)
#  define LOG_ERROR_MSG(x)

#  define FUNC_START_DEBUG_MSG
#  define FUNC_END_DEBUG_MSG

#  define LOG_CIOS_DEBUG_MSG(x)
#  define LOG_CIOS_INFO_MSG(x)
#  define LOG_CIOS_TRACE_MSG(x)
#  define CIOSLOGEVT_CH(x,y)

#  define CIOSLOGMSG_WC(x,y)
#  define CIOSLOGRDMA_REQ(x,y,z,a)

#else
//
// Logging enabled
//
#  ifdef RDMAHELPER_HPX_COMPATIBILITY
#   include <hpx/hpx_fwd.hpp>
#  endif

//#include <boost/format.hpp>
#  include <boost/log/trivial.hpp>
#  include <boost/log/expressions/formatter.hpp>
#  include <boost/log/expressions/formatters.hpp>
#  include <boost/log/expressions/formatters/stream.hpp>
#  include <boost/log/expressions.hpp>
#  include <boost/log/sources/severity_logger.hpp>
#  include <boost/log/sources/record_ostream.hpp>
#  include <boost/log/utility/formatting_ostream.hpp>
#  include <boost/log/utility/manipulators/to_log.hpp>
#  include <boost/log/utility/setup/console.hpp>
#  include <boost/log/utility/setup/common_attributes.hpp>

  namespace logging = boost::log;
  namespace src = boost::log::sources;
  namespace expr = boost::log::expressions;
  namespace sinks = boost::log::sinks;
  namespace attrs = boost::log::attributes;
  namespace keywords = boost::log::keywords;

  void initLogging();

// need to put this one back ...
#  define LOG_DECLARE_FILE(f)

#  define LOG_TRACE_MSG(x) BOOST_LOG_TRIVIAL(trace)   << x;
#  define LOG_DEBUG_MSG(x) BOOST_LOG_TRIVIAL(debug)   << x;
#  define LOG_INFO_MSG(x)  BOOST_LOG_TRIVIAL(info)    << x;
#  define LOG_WARN_MSG(x)  BOOST_LOG_TRIVIAL(warning) << x;
#  define LOG_ERROR_MSG(x) BOOST_LOG_TRIVIAL(error)   << x;
#  define LOG_FATAL_MSG(x) BOOST_LOG_TRIVIAL(fatal)   << x;

#  define FUNC_START_DEBUG_MSG LOG_DEBUG_MSG("**************** Enter " << __func__ << " ****************");
#  define FUNC_END_DEBUG_MSG   LOG_DEBUG_MSG("################ Exit  " << __func__ << " ################");


#  define LOG_CIOS_DEBUG_MSG(x) BOOST_LOG_TRIVIAL(debug)   << x;
#  define LOG_CIOS_INFO_MSG(x)  BOOST_LOG_TRIVIAL(info)    << x;
#  define LOG_CIOS_TRACE_MSG(x) BOOST_LOG_TRIVIAL(trace)   << x;

#  define BGV_RDMADROP 1
#  define BGV_RDMA_REG 2
#  define BGV_RDMA_RMV 3
#  define BGV_WORK_CMP 4
#  define BGV_RECV_EVT 5

#  define CIOSLOGRDMA_REQ(ID,region,frags,fd) BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGRDMA_REQ " << ID << " " << region << " " << frags << " " << fd;
#  define CIOSLOGMSG_WC(ID,wc)                BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGMSG_WC   " << ID << " " << ((struct ibv_wc *)wc)->wr_id;
#  define CIOSLOGPOSTSEND(ID,send_wr,err)     BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGPOSTSEND " << ID << " " << send_wr.wr_id << " " << err;
#  define CIOSLOGEVT_CH(ID,event)             BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGEVT_CH   " << ID << " " << event;

#endif // RDMAHELPER_DISABLE_LOGGING

#endif
