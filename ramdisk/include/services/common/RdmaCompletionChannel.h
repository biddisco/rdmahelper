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

//! \file  RdmaCompletionChannel.h
//! \brief Declaration and inline methods for bgcios::RdmaCompletionChannel class.

#ifndef COMMON_RDMACOMPLETIONCHANNEL_H
#define COMMON_RDMACOMPLETIONCHANNEL_H

// Includes
#include "RdmaCompletionQueue.h"
#include "PointerMap.h"
#include <inttypes.h>
#include <infiniband/verbs.h>
#include <tr1/memory>

namespace bgcios
{

//! \brief Completion channel for RDMA operations.

class RdmaCompletionChannel
{
public:

   //! \brief  Default constructor.
   //! \param  context InfiniBand device context.
   //! \param  nonblockMode True to put completion channel in non-blocking mode.
   //! \param  ackLimit Number of notification events to accumulate before acknowledging events.
   //! \throws RdmaError.

   RdmaCompletionChannel(ibv_context *context, bool nonblockMode, unsigned int ackLimit = 1);

   //! \brief  Default destructor.

   ~RdmaCompletionChannel();

   //! \brief  Get the descriptor representing the completion channel.
   //! \return Completion channel descriptor.

   int getChannelFd(void) const { return _completionChannel->fd; }

   //! \brief  Get the InfiniBand completion channel.
   //! \return Pointer to completion channel structure.

   ibv_comp_channel *getChannel(void) const { return _completionChannel; }

   //! \brief  Set non-blocking mode for completion channel.
   //! \param  mode New value for non-blocking mode.
   //! \return Nothing.
   //! \throws RdmaError.

   void setNonBlockMode(bool mode);

   //! \brief  Check if completion channel is in non-blocking mode..
   //! \return True if completion channel is in non-blocking mode, otherwise false.

   bool isNonBlockMode(void);

   //! \brief  Add a completion queue to the list of queues using the completion channel.
   //! \param  cq Pointer to completion queue.
   //! \return Nothing.

   void addCompletionQ(RdmaCompletionQueuePtr cq);

   //! \brief  Remove a completion queue from the list of queues using the completion channel.
   //! \param  cq Pointer to completion queue.
   //! \return Nothing.

   void removeCompletionQ(RdmaCompletionQueuePtr cq);

   //! \brief  Wait for a notification event on the completion channel.
   //! \return Nothing.
   //! \throws RdmaError.

   void waitForEvent(void);

   //! \brief  Get a notification event from the completion channel.
   //! \return Pointer to completion queue that has a completion available.
   //! \throws RdmaError.

   RdmaCompletionQueuePtr getEvent(void);

   //! \brief  Write info about the completion channel to the specified stream.
   //! \param  os Output stream to write to.
   //! \return Output stream.

   std::ostream& writeTo(std::ostream& os) const;

private:

   //! Context for InfiniBand device.
   struct ibv_context *_context;

   //! Completion channel.
   struct ibv_comp_channel *_completionChannel;

   //! Number of notification events that have not been acknowledged yet.
   unsigned int _numEvents;

   //! Acknowledge notification events when the limit is reached.
   unsigned int _ackEventLimit;

   //! Map of completion queues using the completion channel indexed by completion queue handle.
   bgcios::PointerMap<uint32_t, RdmaCompletionQueuePtr> _queues;

};

//! Smart pointer for RdmaCompletionChannel object.
typedef std::tr1::shared_ptr<RdmaCompletionChannel> RdmaCompletionChannelPtr;

//! \brief  RdmaCompletionChannel shift operator for output.

inline std::ostream& operator<<(std::ostream& os, const RdmaCompletionChannel& cc)
{
   return cc.writeTo(os);
}

} // namespace bgcios

#endif // COMMON_RDMACOMPLETIONCHANNEL_H


