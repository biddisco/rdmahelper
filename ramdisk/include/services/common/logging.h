/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/* ================================================================ */
/*                                                                  */
/* Licensed Materials - Property of IBM                             */
/*                                                                  */
/* Blue Gene/Q                                                      */
/*                                                                  */
/* (C) Copyright IBM Corp.  2011, 2012                              */
/*                                                                  */
/* US Government Users Restricted Rights -                          */
/* Use, duplication or disclosure restricted                        */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/* This software is available to you under the                      */
/* Eclipse Public License (EPL).                                    */
/*                                                                  */
/* ================================================================ */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */

//! \file  logging.h
//! \brief Declarations for log helper functions.
#ifndef COMMON_LOGGING_H
#define COMMON_LOGGING_H

#include <string>
#include <utility/include/Log.h>
#include <unistd.h>

// The following #defines will disable compiling calls to log4cxx into the CIOS code (for performance reasons)
// To compile calls to particular levels of logging, comment out the appropriate defines from the following list 
//#define LOG_CIOS_DEBUG_DISABLE
//#define LOG_CIOS_TRACE_DISABLE
//#define LOG_CIOS_INFO_DISABLE
//#define LOG_CIOS_WARN_DISABLE
//#undef LOG_CIOS_TRACE_DISABLE

#define LOGGING_DECLARE_PID_MDC(value) \
    log4cxx::MDC _pid_mdc( "pid", boost::lexical_cast<std::string>(value) + ":" );

#define LOGGING_DECLARE_SERVICE_MDC(value) \
    log4cxx::MDC _service_mdc( "service", boost::lexical_cast<std::string>(value) + ":" );


#ifdef LOG_CIOS_DEBUG_DISABLE
    #define LOG_CIOS_DEBUG_MSG( message_expr )
#else
    #define LOG_CIOS_DEBUG_MSG( message_expr ) LOG_DEBUG_MSG( message_expr )
#endif
#ifdef LOG_CIOS_TRACE_DISABLE
    #define LOG_CIOS_TRACE_MSG( message_expr )
#else
    #define LOG_CIOS_TRACE_MSG( message_expr ) LOG_TRACE_MSG( message_expr )
#endif
#ifdef LOG_CIOS_DEBUG_DISABLE
    #define LOG_CIOS_INFO_MSG( message_expr )
#else
    #define LOG_CIOS_INFO_MSG( message_expr ) LOG_INFO_MSG( message_expr )
#endif
#ifdef LOG_CIOS_WARN_DISABLE
    #define LOG_CIOS_WARN_MSG( message_expr )
#else
    #define LOG_CIOS_WARN_MSG( message_expr ) LOG_WARN_MSG( message_expr )
#endif
#define LOG_CIOS_DEBUG_MSG_FORCED( message_expr )  LOG_DEBUG_MSG_FORCED( message_expr ) 
#define LOG_CIOS_TRACE_MSG_FORCED( message_expr )  LOG_TRACE_MSG_FORCED( message_expr ) 
#define LOG_CIOS_INFO_MSG_FORCED( message_expr )   LOG_INFO_MSG_FORCED( message_expr )  
#define LOG_CIOS_WARN_MSG_FORCED( message_expr )   LOG_WARN_MSG_FORCED( message_expr )  
#define LOG_CIOS_ERROR_MSG_FORCED( message_expr )  LOG_ERROR_MSG_FORCED( message_expr ) 
#define LOG_CIOS_FATAL_MSG_FORCED( message_expr )  LOG_FATAL_MSG_FORCED( message_expr ) 

namespace bgcios
{
    //! \brief Set the logger level.
    //! \param logger the logger name to set
    //! \param level the level character

    void setLoggingLevel(const std::string& logger, char level);

    //! \brief  Log a stack back trace.
    //! \param  backTraceSize Maximum number of stack frames to include in back trace.
    //! \return Nothing.

    void logStackBackTrace(int backTraceSize);

} // namespace bgcios

#endif // COMMON_LOGGING_H
