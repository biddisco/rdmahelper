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

//! \file  RdmaProtectionDomain.cc
//! \brief Methods for bgcios::RdmaProtectionDomain class.

// Includes
#include <ramdisk/include/services/common/RdmaProtectionDomain.h>
#include <ramdisk/include/services/common/RdmaError.h>
#include <ramdisk/include/services/ServicesConstants.h>
#include <ramdisk/include/services/common/logging.h>
#include <errno.h>

using namespace bgcios;

LOG_DECLARE_FILE("cios.common");


RdmaProtectionDomain::RdmaProtectionDomain(struct ibv_context *context)
{
   // Validate context pointer (since ibv_ functions won't check it).
   if (context == NULL) {
      RdmaError e(EFAULT, "device context pointer is null");
      LOG_ERROR_MSG("error with context pointer " << context << " when constructing protection domain");
      throw e;
   }

   // Allocate a protection domain.
   _protectionDomain = ibv_alloc_pd(context);
   if (_protectionDomain == NULL) {
      RdmaError e(ENOMEM, "ibv_alloc_pd() failed");
      LOG_ERROR_MSG("error allocating protection domain");
      throw e;
   }
   LOG_CIOS_DEBUG_MSG("allocated protection domain " << _protectionDomain->handle);
}

RdmaProtectionDomain::~RdmaProtectionDomain()
{
   if (_protectionDomain != NULL) {
      uint32_t handle = _protectionDomain->handle;
      int err = ibv_dealloc_pd(_protectionDomain);
      if (err == 0) {
         _protectionDomain = NULL;
         LOG_CIOS_DEBUG_MSG("deallocated protection domain " << handle);
      }
      else {
         LOG_ERROR_MSG("error deallocating protection domain " << handle << ": " <<  bgcios::errorString(err));
      }
   }
}

std::ostream&
RdmaProtectionDomain::writeTo(std::ostream& os) const
{
   os << "context=" << _protectionDomain->context << " handle=" << _protectionDomain->handle;
   return os;
}

