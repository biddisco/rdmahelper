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

//! \file  RdmaCompletionQueue.h
//! \brief Declaration and inline methods for bgcios::RdmaCompletionQueue class.

#ifndef COMMON_RDMACOMPLETIONQUEUE_H
#define COMMON_RDMACOMPLETIONQUEUE_H

// Includes
#include <inttypes.h>
#include <infiniband/verbs.h>
#include <string>
#include <memory>
#include <mutex>

namespace bgcios
{

//! \brief Completion queue for RDMA operations.

class RdmaCompletionQueue
{
public:

   //! \brief  Default constructor.
   //! \param  context InfiniBand device context.
   //! \param  qSize Minimum number of entries in completion queue.
   //! \param  completionChannel Pointer to completion channel for event notification (can be NULL).
   //! \throws rdma_error.

   RdmaCompletionQueue(ibv_context *context, int qSize, ibv_comp_channel *completionChannel);

   //! \brief  Default destructor.

   ~RdmaCompletionQueue();

   //! \brief  Get the completion queue structure.
   //! \return Pointer to completion queue structure.

   struct ibv_cq *getQueue(void) const { return _completionQ; }

   //! \brief  Get the handle that identifies the completion queue.
   //! \return Handle value.

   uint32_t getHandle(void) const { return _completionQ != NULL ? _completionQ->handle : 0; }

   //! \brief  Request notification events on completion channel when completion queue entry is placed on queue.
   //! \return Nothing.
   //! \throws rdma_error.

   void requestEvent(void);

   //! \brief  Acknowledge all outstanding notification events on completion queue.
   //! \return Nothing.
   //! \throws rdma_error.

   void ackEvents(unsigned int numEvents);

   //! \brief  Remove the specified number of entries from the completion queue.
   //! \param  numEntries Maximum number of entries to remove.
   //! \return Number of completion queue entries removed from completion queue.
   //! \throws rdma_error.

   int removeCompletions(int numEntries = 1);

   //! \brief  Get the current completion queue entry.
   //!         Returns NULL when no completion queue entries are available.
   //! \return Pointer to completion queue entry.

   struct ibv_wc *popCompletion(void);

   //! \brief  Return a string naming a ibv_wc_opcode value.
   //! \param  opcode ibv_wc_opcode value.
   //! \return String representing value.

   static const std::string wc_opcode_str(enum ibv_wc_opcode opcode);

   //! \brief  Write info about the completion queue to the specified stream.
   //! \param  os Output stream to write to.
   //! \return Output stream.

   std::ostream& writeTo(std::ostream& os) const;

   //! Maximum number of entries in completion queue.
   static const int MaxQueueSize = 256;

   //! Number of entries in completion queue handling a single compute node.
   static const int SingleNodeQueueSize = 8;

   int poll_completion(struct ibv_wc *completion);

//private:

   //! Context for IB device.
   struct ibv_context *_context;

   //! Completion queue.
   struct ibv_cq *_completionQ;

   //! Array of work completions.
   struct ibv_wc _completions[MaxQueueSize];

   //! Number of valid work completions in _completions array.
   int _numCompletions;

   //! Index of next work completion to pop from _completions array.
   int _nextCompletion;

   //! Total number of work completions received by completion queue.
   int _totalCompletions;

   //! Total number of notification events acknowledged by completion queue.
   unsigned int _totalEvents;

   //! Tag to identify completion queue in trace points.
   std::string _tag;

   std::mutex completion_mutex;

};


//! Smart pointer for RdmaCompletionQueue object.
typedef std::shared_ptr<RdmaCompletionQueue> RdmaCompletionQueuePtr;

//! \brief  RdmaCompletionQueue shift operator for output.

inline std::ostream& operator<<(std::ostream& os, const RdmaCompletionQueue& cq)
{
   return cq.writeTo(os);
}

} // namespace bgcios

#endif // COMMON_RDMACOMPLETIONQUEUE_H

