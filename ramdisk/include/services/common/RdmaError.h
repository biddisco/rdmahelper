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

//! \file  RdmaError.h 
//! \brief Declaration and inline methods for bgcios::RdmaError class.

#ifndef COMMON_RDMAERROR_H
#define COMMON_RDMAERROR_H

// Includes
#include <stdexcept>
#include <string>

namespace bgcios
{

//! Exception for general RDMA errors.

class RdmaError : public std::runtime_error
{
public:

   //! \brief  Default constructor.
   //! \param  err Error code value.
   //! \param  what String describing error.

   RdmaError(int err=0, const std::string what="") : std::runtime_error(what), _errcode(err) { }

   int errcode(void) const { return _errcode; }

protected:

   //! Error code (typically errno from RDMA function).
   int _errcode;
};

} // namespace bgcios

#endif // COMMON_RDMAERROR_H

