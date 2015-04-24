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

//! \file  RdmaCompletionQueue.cc
//! \brief Methods for bgcios::RdmaCompletionQueue class.

#include <RdmaCompletionQueue.h>
#include <RdmaError.h>
#include "RdmaLogging.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
//#include <Cioslog.h>

using namespace bgcios;
const int RdmaCompletionQueue::MaxQueueSize;

//LOG_DECLARE_FILE("cios.common");

RdmaCompletionQueue::RdmaCompletionQueue(ibv_context *context, int qSize,  ibv_comp_channel *completionChannel)
{
   // Initialize private data.
   _context = context;
   _completionQ = NULL;
   _numCompletions = 0;
   _nextCompletion = 0;
   _totalCompletions = 0;
   _totalEvents = 0;
   _tag = "[CQ ?] ";

   // Validate context pointer (since ibv_ functions won't check it).
   if (context == NULL) {
      RdmaError e(EFAULT, "device context pointer is null");
      LOG_ERROR_MSG("error with context pointer " << context << " when constructing completion queue");
      throw e;
   }

   LOG_DEBUG_MSG("Completion queue create size " << qSize);
   // Create a completion queue.
   _completionQ = ibv_create_cq(context, qSize, NULL, completionChannel, 0);
   if (_completionQ == NULL) {
      RdmaError e(errno, "ibv_create_cq() failed");
      LOG_ERROR_MSG(_tag << "error creating completion queue: " << RdmaError::errorString(e.errcode()));
      throw e;
   }

   std::ostringstream tag;
   tag << "[CQ " << _completionQ->handle << "] ";
   _tag = tag.str();
   LOG_CIOS_DEBUG_MSG(_tag << "created completion queue ");

   // Request notification of events on the completion queue.
   try {
      requestEvent();
   }
   catch (const RdmaError& e) {
      LOG_ERROR_MSG(_tag << "error requesting first completion queue notification: " << RdmaError::errorString(e.errcode()));
      throw e;
   }
   LOG_CIOS_TRACE_MSG(_tag << "requested first notification for completion queue");
}

RdmaCompletionQueue::~RdmaCompletionQueue()
{
   if (_completionQ != NULL) {
      int err = ibv_destroy_cq(_completionQ);
      if (err == 0) {
         _completionQ = NULL;
         LOG_CIOS_DEBUG_MSG(_tag << "destroyed completion queue");
      }
      else {
         LOG_ERROR_MSG(_tag << "error destroying completion queue: " << RdmaError::errorString(err));
      }
   }
}

void
RdmaCompletionQueue::requestEvent(void)
{
   int err = ibv_req_notify_cq(_completionQ, 0);
   if (err != 0) {
      RdmaError e(err, "ibv_req_notify_cq() failed");
      LOG_ERROR_MSG(_tag << "error requesting notification for completion queue: " << RdmaError::errorString(e.errcode()));
      throw e;
   }

   LOG_CIOS_TRACE_MSG(_tag << "requested notification for completion queue");
   return;
}

void
RdmaCompletionQueue::ackEvents(unsigned int numEvents)
{
   // Acknowledge the outstanding notification events.
   ibv_ack_cq_events(_completionQ, numEvents);
   LOG_CIOS_TRACE_MSG(_tag << "acked " << numEvents << " notification event for completion queue");
   _totalEvents += numEvents;

   return;
}

// Remove the work completions from the completion queue.
int RdmaCompletionQueue::removeCompletions(int numEntries)
{
    std::lock_guard<std::mutex> lock(completion_mutex);
    //
    int nc = ibv_poll_cq(_completionQ, numEntries, &(_completions[_numCompletions]));
    if (nc < 0) {
        RdmaError e(EINVAL, "ibv_poll_cq() failed"); // Documentation does not indicate how errno is returned
        LOG_ERROR_MSG(_tag << "error polling completion queue: " << RdmaError::errorString(e.errcode()));
        throw e;
    }
    for (int i=0; i<nc; i++){
        CIOSLOGMSG_WC(BGV_WORK_CMP, _completions+i);
    }
    _numCompletions += nc;
    if (nc>0) {
        LOG_CIOS_TRACE_MSG(_tag << _numCompletions-_nextCompletion << " pending : removing " << nc << " work completions from completion queue, ");
    }
    _totalCompletions += _numCompletions;

    return nc;
}

struct ibv_wc *
RdmaCompletionQueue::popCompletion(void)
{
    std::lock_guard<std::mutex> lock(completion_mutex);
    //
    if (_numCompletions == 0) {
        LOG_CIOS_TRACE_MSG(_tag << "no work completions are available");
        return NULL;
    }

    // Get the next work completion from the list.
    struct ibv_wc *completion = &(_completions[_nextCompletion]);
    if (completion->status != IBV_WC_SUCCESS) {
        LOG_ERROR_MSG(_tag << "work completion status '" << ibv_wc_status_str(completion->status)
                << "' for operation " << wc_opcode_str(completion->opcode) <<  " (" << completion->opcode << ")");
    }
    else {
        LOG_CIOS_TRACE_MSG(_tag << "work completion status '" << ibv_wc_status_str(completion->status)
                << "' for operation " << wc_opcode_str(completion->opcode) <<  " (" << completion->opcode << ")");
    }

    // Increment next work completion index.
    _nextCompletion++;

    // All of the work completions have been popped.
    if (_nextCompletion == _numCompletions) {
        LOG_CIOS_TRACE_MSG(_tag << "done, all " << _numCompletions << " work completions are popped");
        _numCompletions = 0;
        _nextCompletion = 0;
    }
    LOG_CIOS_TRACE_MSG(_tag << "after pop, next completion is " << _nextCompletion << ", num completions is " << _numCompletions);

    return completion;
}

std::string const RdmaCompletionQueue::wc_opcode_str(ibv_wc_opcode opcode)
{
   std::string str;
   switch (opcode) {
      case IBV_WC_SEND: str = "IBV_WC_SEND"; break;
      case IBV_WC_RDMA_WRITE: str = "IBV_WC_RDMA_WRITE"; break;
      case IBV_WC_RDMA_READ: str = "IBV_WC_RDMA_READ"; break;
      case IBV_WC_COMP_SWAP: str = "IBV_WC_COMP_SWAP"; break;
      case IBV_WC_FETCH_ADD: str = "IBV_WC_FETCH_ADD"; break;
      case IBV_WC_BIND_MW: str = "IBV_WC_BIND_MW"; break;
      case IBV_WC_RECV: str = "IBV_WC_RECV"; break;
      case IBV_WC_RECV_RDMA_WITH_IMM: str = "IBV_WC_RECV_RDMA_WITH_IMM"; break;
   }

   return str;
}

std::ostream&
RdmaCompletionQueue::writeTo(std::ostream& os) const
{
   os << _tag;
   os << " numCompletions=" << _numCompletions << " nextCompletion=" << _nextCompletion << " totalCompletions=" << _totalCompletions;
   os << " totalEvents=" << _totalEvents;
   return os; 
}

