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

//! \file  RdmaClient.h
//! \brief Declaration and inline methods for bgcios::RdmaClient class.

#ifndef COMMON_RDMASHAREDRECEIVEQUEUE_H
#define COMMON_RDMASHAREDRECEIVEQUEUE_H

// Includes
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include "RdmaProtectionDomain.h"

namespace bgcios
{

//! Shared Receive Queue.

class RdmaSharedReceiveQueue
{
public:

   //! \brief  Default constructor.

   RdmaSharedReceiveQueue(struct rdma_cm_id *cmId, RdmaProtectionDomainPtr domain);

   //! \brief  Default destructor.

   ~RdmaSharedReceiveQueue();

   struct ibv_srq *get_SRQ() {
     if (_cmId->qp == NULL) {
       std::cout << "Trying to access SRQ before QP is ready! " << std::endl;
       return NULL;
     }
     return _cmId->qp->srq; }

private:

   //! Memory region for inbound messages.
   RdmaProtectionDomainPtr  _domain;
   struct ibv_srq_init_attr _srq_attr;
   struct rdma_cm_id       *_cmId;
};

//! Smart pointer for RdmaSharedReceiveQueue object.
typedef std::shared_ptr<RdmaSharedReceiveQueue> RdmaSharedReceiveQueuePtr;

} // namespace bgcios

#endif // COMMON_RDMACLIENT_H

