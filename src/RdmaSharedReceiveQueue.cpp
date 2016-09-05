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
//! \file  RdmaClient.h
//! \brief Declaration and inline methods for class.

// Includes

#include <iostream>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_logging.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/RdmaSharedReceiveQueue.h>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_error.hpp>
#include <cstring>
#include <rdma/rdma_verbs.h>

/*
  struct ibv_srq_init_attr {
          void                   *srq_context;    // Associated context of the SRQ
          struct ibv_srq_attr     attr;           // SRQ attributes
  };

  struct ibv_srq_attr {
          uint32_t                max_wr;         // Requested max number of outstanding work requests (WRs) in the SRQ
          uint32_t                max_sge;        // Requested max number of scatter elements per WR
          uint32_t                srq_limit;      // The limit value of the SRQ (irrelevant for ibv_create_srq)
  };
*/

using namespace hpx::parcelset::policies::verbs;
namespace bgcios
{

//! Shared Receive Queue.

#define VERBS_EP_RX_CNT         (4096)  // default SRQ size
#define VERBS_EP_TX_CNT         (4096)  // default send count

RdmaSharedReceiveQueue::RdmaSharedReceiveQueue(struct rdma_cm_id *cmId, rdma_protection_domainPtr domain)
{
  _domain = domain;
  _cmId   = cmId;
  memset(&_srq_attr, 0, sizeof(ibv_srq_init_attr));
  _srq_attr.attr.max_wr = VERBS_EP_RX_CNT;
  _srq_attr.attr.max_sge = 6; // @todo : need to query max before setting this

//  std::cout << "Here with cmId " << _cmId << std::endl;

  int err = rdma_create_srq(_cmId, _domain->getDomain(), &_srq_attr);
//  std::cout << "Here 2 with cmId " << _cmId << std::endl;

  if (err != 0) {
     rdma_error e(errno, "rdma_create_srq() failed");
     LOG_ERROR_MSG("error creating shared receive queue : " << rdma_error::error_string(e.errcode()));
     throw e;
   }
  std::cout << "Here 2 with cmId " << _cmId << std::endl;

  LOG_DEBUG_MSG("created SRQ shared receive queue " /*<< _cmId->qp->srq*/ << " context " << _srq_attr.srq_context << " max wr " << _srq_attr.attr.max_wr << " max sge " << _srq_attr.attr.max_sge);
//  std::cout << "Here 2 with cmId " << _cmId << std::endl;
   return;
}

RdmaSharedReceiveQueue::~RdmaSharedReceiveQueue()
{
//  if (rdma_destroy_srq(_cmId)) {
  rdma_destroy_srq(_cmId);

//    rdma_error e(errno, "rdma_destroy_srq() failed");
//    LOG_ERROR_MSG("error deleting shared receive queue : " << rdma_error::error_string(e.errcode()));
//    throw e;
//  }
}

} // namespace
