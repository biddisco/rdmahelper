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
#include <iostream>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <memory>
#include <string>

//
// useful macros for formatting messages
//
#define hexpointer(p) "0x" << std::setfill('0') << std::setw(12) << std::hex << (uintptr_t)(p) << " "
#define hexuint32(p)  "0x" << std::setfill('0') << std::setw( 8) << std::hex << (uint32_t)(p) << " "
#define hexlength(p)  "0x" << std::setfill('0') << std::setw( 6) << std::hex << (uintptr_t)(p) << " "
#define hexnumber(p)  "0x" << std::setfill('0') << std::setw( 4) << std::hex << p << " "
#define decnumber(p)  "" << std::dec << p << " "
#define ipaddress(p)  "" << std::dec << (int) ((uint8_t*) &p)[0] << "." << (int) ((uint8_t*) &p)[1] << \
    "." << (int) ((uint8_t*) &p)[2] << "." << (int) ((uint8_t*) &p)[3] << " "

//
// HPX support
//
#  ifdef RDMAHELPER_HAVE_HPX
#    include <hpx/config.hpp>
#    include <hpx/hpx_fwd.hpp>
#  endif

//
// Logging disabled, #define all macros to be empty
//
#ifndef RDMAHELPER_HAVE_LOGGING
#  include <sstream>

#  define LOG_DECLARE_FILE(x)

#  define LOG_DEBUG_MSG(x)
#  define LOG_TRACE_MSG(x)
#  define LOG_INFO_MSG(x)
#  define LOG_WARN_MSG(x)
#  define LOG_ERROR_MSG(x) std::cout << x << " " << __FILE__ << " " << __LINE__ << std::endl;
#  define LOG_ARRAY_MSG(m, p, n, t)

#  define FUNC_START_DEBUG_MSG
#  define FUNC_END_DEBUG_MSG

#  define LOG_CIOS_DEBUG_MSG(x)
#  define LOG_CIOS_INFO_MSG(x)
#  define LOG_CIOS_TRACE_MSG(x)
#  define CIOSLOGEVT_CH(x,y)

#  define CIOSLOGMSG_WC(x,y)
#  define CIOSLOGRDMA_REQ(x,y,z,a)
#  define initRdmaHelperLogging()

#else
//
// Logging enabled
//
#  ifdef RDMAHELPER_HAVE_HPX
#    include <hpx/runtime/threads/thread.hpp>
#    define THREAD_ID (hpx::this_thread::get_id())
#  else 
#    define THREAD_ID ""
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
#  include <boost/preprocessor.hpp>

  namespace logging = boost::log;
  namespace src = boost::log::sources;
  namespace expr = boost::log::expressions;
  namespace sinks = boost::log::sinks;
  namespace attrs = boost::log::attributes;
  namespace keywords = boost::log::keywords;

  void initRdmaHelperLogging();

// need to put this one back ...
#  define LOG_DECLARE_FILE(f)

#  define LOG_TRACE_MSG(x) BOOST_LOG_TRIVIAL(trace)   << THREAD_ID << " " << x;
#  define LOG_DEBUG_MSG(x) BOOST_LOG_TRIVIAL(debug)   << THREAD_ID << " " << x;
#  define LOG_INFO_MSG(x)  BOOST_LOG_TRIVIAL(info)    << THREAD_ID << " " << x;
#  define LOG_WARN_MSG(x)  BOOST_LOG_TRIVIAL(warning) << THREAD_ID << " " << x;
#  define LOG_ERROR_MSG(x) BOOST_LOG_TRIVIAL(error)   << THREAD_ID << " " << x;
#  define LOG_FATAL_MSG(x) BOOST_LOG_TRIVIAL(fatal)   << THREAD_ID << " " << x;

#  define LOG_MEMORY_MSG(m, p, n) \
        { \
          const char *src = reinterpret_cast<const char*>(p); \
          std::stringstream tmp; \
          tmp << m << ' ' << hexpointer(p) << "= ("; \
          for (uint32_t i=0; i<(n/sizeof(uint32_t)); i++) { \
            uint32_t res; \
            std::memcpy(&res, &src[i*sizeof(res)], sizeof(res)); \
            tmp << hexuint32(res) << (((int)(i)<n-1) ? ',' : ')'); \
          } \
          LOG_DEBUG_MSG(tmp.str()) \
        }

#  define FUNC_START_DEBUG_MSG LOG_DEBUG_MSG("**************** Enter " << __func__ << " ****************");
#  define FUNC_END_DEBUG_MSG   LOG_DEBUG_MSG("################ Exit  " << __func__ << " ################");


#  define LOG_CIOS_DEBUG_MSG(x) LOG_DEBUG_MSG(x)
#  define LOG_CIOS_INFO_MSG(x)  LOG_INFO_MSG(x)
#  define LOG_CIOS_TRACE_MSG(x) LOG_TRACE_MSG(x)

#  define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem)    \
    case elem : return BOOST_PP_STRINGIZE(elem);

#  define DEFINE_ENUM_WITH_STRING_CONVERSIONS(name, enumerators)              \
    enum name {                                                               \
        BOOST_PP_SEQ_ENUM(enumerators)                                        \
    };                                                                        \
                                                                              \
    inline const char* ToString(name v)                                       \
    {                                                                         \
        switch (v)                                                            \
        {                                                                     \
            BOOST_PP_SEQ_FOR_EACH(                                            \
                X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,          \
                name,                                                         \
                enumerators                                                   \
            )                                                                 \
            default: return "[Unknown " BOOST_PP_STRINGIZE(name) "]";         \
        }                                                                     \
    }

   DEFINE_ENUM_WITH_STRING_CONVERSIONS(BGCIOS_type, (BGV_RDMADROP)(BGV_RDMA_REG)(BGV_RDMA_RMV)(BGV_WORK_CMP)(BGV_RECV_EVT))

#  define CIOSLOGRDMA_REQ(ID,region,frags,fd) BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGRDMA_REQ " << ToString((BGCIOS_type)(ID)) << " " << region << " " << frags << " " << fd;
#  define CIOSLOGMSG_WC(ID,wc)                BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGMSG_WC   " << ToString((BGCIOS_type)(ID)) << " " << hexpointer(((struct ibv_wc *)wc)->wr_id);
#  define CIOSLOGPOSTSEND(ID,send_wr,err)     BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGPOSTSEND " << ToString((BGCIOS_type)(ID)) << " " << hexpointer(send_wr.wr_id) << " " << err;
#  define CIOSLOGEVT_CH(ID,event)             BOOST_LOG_TRIVIAL(trace)   << "CIOSLOGEVT_CH   " << ToString((BGCIOS_type)(ID)) << " " << event;
#  ifdef RDMAHELPER_HAVE_HPX
#   include <hpx/config.hpp>
#   include <hpx/hpx_fwd.hpp>
#  endif

#endif // RDMAHELPER_DISABLE_LOGGING

#endif
