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

//! \file  rdma_error.h
//! \brief Declaration and inline methods for rdma_error class.

#ifndef COMMON_RDMAERROR_H
#define COMMON_RDMAERROR_H

// Includes
#include <stdexcept>
#include <string.h>

namespace hpx {
namespace parcelset {
namespace policies {
namespace verbs {

//! Exception for general RDMA errors.

class rdma_error : public std::runtime_error
{
public:

   //! \brief  Default constructor.
   //! \param  err Error code value.
   //! \param  what String describing error.

   rdma_error(int err=0, const std::string what="") : std::runtime_error(what), _errcode(err) { }

   int errcode(void) const { return _errcode; }

   //! \brief  Return a string describing an errno value.
   //! \param  error Errno value.
   //! \return Pointer to string describing errno value.
   //! Warning : strerror_r is differnt under posix/gnu
   //! mac version returns int, others char*
   static inline char *error_string(int error)
   {
   #if !defined(_GNU_SOURCE) || defined(__APPLE__)
      char buf[256];
      if (strerror_r (error, buf, sizeof buf)==0) return buf;
      else return NULL;
   #else
      char errorBuffer[256];
      return strerror_r(error, errorBuffer, sizeof(errorBuffer));
   #endif
   }

protected:

   //! Error code (typically errno from RDMA function).
   int _errcode;
};

}}}} // namespace bgcios

#endif // COMMON_RDMAERROR_H

