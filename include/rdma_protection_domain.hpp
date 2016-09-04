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

//! \file  rdma_protection_domain.h
//! \brief Declaration and inline methods for bgcios::rdma_protection_domain class.

#ifndef COMMON_RDMAPROTECTIONDOMAIN_H
#define COMMON_RDMAPROTECTIONDOMAIN_H

// Includes
#include <infiniband/verbs.h>
//
#include <memory>
#include <cstdint>

namespace bgcios
{

//! \brief InfiniBand verbs protection domain.

class rdma_protection_domain
{
public:

   //! \brief  Default constructor.
   //! \param  context InfiniBand device context.

   rdma_protection_domain(struct ibv_context *context);

   //! \brief  Default destructor.

   ~rdma_protection_domain();

   //! \brief  Get the pointer to the protection domain for use with verbs.
   //! \return Pointer to protection domain.

   struct ibv_pd *getDomain(void) const { return _protectionDomain; }

   //! \brief  Get the handle that identifies the protection domain.
   //! \return Handle.

   uint32_t getHandle(void) const { return _protectionDomain != NULL ? _protectionDomain->handle : 0; }

   //! \brief  Write info about the protection domain to the specified stream.
   //! \param  os Output stream to write to.
   //! \return Output stream.

   std::ostream& writeTo(std::ostream& os) const;

private:

   //! Protection domain.
   struct ibv_pd *_protectionDomain;

};

//! Smart pointer for rdma_protection_domain object.
typedef std::shared_ptr<rdma_protection_domain> rdma_protection_domainPtr;

//! \brief  rdma_protection_domain shift operator for output.

inline std::ostream& operator<<(std::ostream& os, const rdma_protection_domain& pd)
{
   return pd.writeTo(os);
}

} // namespace bgcios

#endif // COMMON_RDMAPROTECTIONDOMAIN_H

