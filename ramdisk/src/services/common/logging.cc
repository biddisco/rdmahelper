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

//! \file  logging.cc
//! \brief Log helper functions.

"rdmahelper_logging.h"
#include <log4cxx/logger.h>
#include <execinfo.h>
#include <stdlib.h>
#include <iomanip>

LOG_DECLARE_FILE("cios.common");

namespace bgcios
{
    
void
setLoggingLevel( 
        const std::string& logger,
        char level
        )
{
    switch ( level ) {
        case 'O': log4cxx::Logger::getLogger( logger )->setLevel( log4cxx::Level::getOff() ); break;
        case 'F': log4cxx::Logger::getLogger( logger )->setLevel( log4cxx::Level::getFatal() ); break;
        case 'E': log4cxx::Logger::getLogger( logger )->setLevel( log4cxx::Level::getError() ); break;
        case 'W': log4cxx::Logger::getLogger( logger )->setLevel( log4cxx::Level::getWarn() ); break;
        case 'I': log4cxx::Logger::getLogger( logger )->setLevel( log4cxx::Level::getInfo() ); break;
        case 'D': log4cxx::Logger::getLogger( logger )->setLevel( log4cxx::Level::getDebug() ); break;
        case 'T': log4cxx::Logger::getLogger( logger )->setLevel( log4cxx::Level::getTrace() ); break;
        case 'A': log4cxx::Logger::getLogger( logger )->setLevel( log4cxx::Level::getAll() ); break;
        default: break;
    }
    return;
}

void
logStackBackTrace(int backTraceSize)
{
   // Log a back trace of the stack.
   LOG_ERROR_MSG("Back trace of stack:");
   char buffer[(unsigned int)backTraceSize*sizeof(void *)];
   void **addressBuffer = (void **)&buffer;
   int numAddrs = backtrace(addressBuffer, backTraceSize);
   if (numAddrs == backTraceSize) {
      LOG_ERROR_MSG("There is a possible stack recursion with " << backTraceSize << " entries on the stack");
   }
   char **stringBuffer = backtrace_symbols(addressBuffer, numAddrs);
   if (stringBuffer != NULL) {
      for (int index = 0; index < numAddrs; ++index) {
         LOG_ERROR_MSG(std::setw(2) << index << ") " << stringBuffer[index]);
      }
   }
   else {
      LOG_ERROR_MSG("Stack addresses could not be converted to strings");
      for (int index = 0; index < numAddrs; ++index) {
         LOG_ERROR_MSG(std::setw(2) << index << ") 0x" << addressBuffer[index]);
      }
   }

   free(stringBuffer);

   return;
}

} // namespace bgcios
