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

//! \file  RdmaCompletionChannel.cc
//! \brief Methods for bgcios::RdmaCompletionChannel class.

// Includes
#include "RdmaLogging.h"
#include <RdmaCompletionChannel.h>
#include <RdmaError.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

using namespace bgcios;

LOG_DECLARE_FILE("cios.common");

RdmaCompletionChannel::RdmaCompletionChannel(ibv_context *context, bool nonblockMode, unsigned int ackLimit)
{
   // Validate context pointer (since ibv_ functions won't check it).
   if (context == NULL) {
      RdmaError e(EFAULT, "device context pointer is null");
      LOG_ERROR_MSG("error with context pointer " << context << " when constructing completion channel");
      throw e;
   }

   // Create a completion channel.
   _completionChannel = ibv_create_comp_channel(context);
   if (_completionChannel == NULL) {
      RdmaError e(ENOMEM, "ibv_create_comp_channel() failed");
      LOG_ERROR_MSG("error creating completion channel");
      throw e;
   }
   LOG_CIOS_DEBUG_MSG("created completion channel with fd " << hexnumber(_completionChannel->fd));

   // If requested, put the completion channel descriptor in non-blocking mode.
   if (nonblockMode) {
      setNonBlockMode(nonblockMode);
   }

   // Initialize private data.
   _numEvents = 0;
   _ackEventLimit = ackLimit;
}

RdmaCompletionChannel::~RdmaCompletionChannel()
{
  _queues.clear();
   // Destroy the completion channel.
   if (_completionChannel != NULL) {
      int fd = _completionChannel->fd;
      int err = ibv_destroy_comp_channel(_completionChannel);
      if (err == 0) {
         _completionChannel = NULL;
         LOG_CIOS_DEBUG_MSG("destroyed completion channel using fd " << hexnumber(fd));
      }
      else {
         LOG_ERROR_MSG("error destroying completion channel using fd " << hexnumber(fd) << ": " << RdmaError::errorString(err));
      }
   }
}

void
RdmaCompletionChannel::setNonBlockMode(bool mode)
{
   int flags = fcntl(_completionChannel->fd, F_GETFL);
   if (mode) {
      flags |= O_NONBLOCK;
      LOG_CIOS_TRACE_MSG("turning on non-blocking mode for completion channel with fd " << hexnumber(_completionChannel->fd));
   }
   else {
      flags &= ~(O_NONBLOCK);
      LOG_CIOS_TRACE_MSG("turning off non-blocking mode for completion channel with fd " << hexnumber(_completionChannel->fd));
   }
   int rc = fcntl(_completionChannel->fd, F_SETFL, flags);
   if (rc != 0) {
      RdmaError e(errno, "fcntl() failed");
      LOG_ERROR_MSG("error changing completion channel with fd " << hexnumber(_completionChannel->fd) << " non-blocking mode using flags " <<
                    std::hex << flags << ": " << RdmaError::errorString(e.errcode()));
      throw e;
   }

   return;
}

bool
RdmaCompletionChannel::isNonBlockMode(void)
{
   int flags = fcntl(_completionChannel->fd, F_GETFL);
   return (flags & O_NONBLOCK ? true : false);
}

void
RdmaCompletionChannel::addCompletionQ(RdmaCompletionQueuePtr cq)
{
   _queues.add(cq->getHandle(), cq);
   LOG_CIOS_TRACE_MSG("added completion queue " << cq->getHandle() << " to completion channel using fd " << hexnumber(_completionChannel->fd));
   return;
}

void
RdmaCompletionChannel::removeCompletionQ(RdmaCompletionQueuePtr cq)
{
   _queues.remove(cq->getHandle());
   LOG_CIOS_TRACE_MSG("removed completion queue " << cq->getHandle() << " from completion channel using fd " << hexnumber(_completionChannel->fd));
   return;
}

void
RdmaCompletionChannel::waitForEvent(void)
{
   pollfd pollInfo;
   pollInfo.fd = _completionChannel->fd;
   pollInfo.events = POLLIN;
   pollInfo.revents = 0;

   int rc = poll(&pollInfo, 1, -1); // Wait forever
   if (rc != 1) {
      RdmaError e(errno, "poll() failed");
      LOG_ERROR_MSG("error polling completion channel using fd " << hexnumber(_completionChannel->fd) << ": " << RdmaError::errorString(e.errcode()));
      throw e;
   }

   LOG_CIOS_TRACE_MSG("notification event is available on completion channel using fd " << hexnumber(_completionChannel->fd));
   return;
}

RdmaCompletionQueuePtr
RdmaCompletionChannel::getEvent(void)
{
   // Get the notification event from the completion channel.
   LOG_CIOS_TRACE_MSG("getting notification event on completion channel using fd " << hexnumber(_completionChannel->fd) << " ...");
   struct ibv_cq *eventQ;
   void *context;
   if (ibv_get_cq_event(_completionChannel, &eventQ, &context) != 0) {
      int err = errno;
      if (err == EAGAIN) {
         LOG_CIOS_TRACE_MSG("no notification events available from completion channel using fd " << hexnumber(_completionChannel->fd));
         RdmaCompletionQueuePtr nullQ;
         return nullQ;
      }
      RdmaError e(err, "ibv_get_cq_event() failed");
      LOG_ERROR_MSG("error getting notification event from completion channel using fd " << hexnumber(_completionChannel->fd) << ": " << RdmaError::errorString(e.errcode()));
      throw e;
   }

   // Find the completion queue that has completions available.
   RdmaCompletionQueuePtr completionQ = _queues.get(eventQ->handle);
   if (completionQ == NULL) {
//    ibv_ack_cq_event(eventQ, 1);
      RdmaError e(ESRCH, "completion queue not found");
      LOG_ERROR_MSG("could not find completion queue " << eventQ->handle << " in list after successful ibv_get_cq_event");
      throw e;
   }

   // Acknowledge the notification event and "re-arm" the completion queue for the next event.
   ++_numEvents;
   if (_numEvents == _ackEventLimit) {
      completionQ->ackEvents(_numEvents);
      _numEvents = 0;
   }
   completionQ->requestEvent();
   LOG_CIOS_TRACE_MSG("got notification event for completion queue " << eventQ->handle);

   return completionQ;
}

std::ostream&
RdmaCompletionChannel::writeTo(std::ostream& os) const
{
   os << "fd=" << _completionChannel->fd << " refcnt=" << _completionChannel->refcnt << " num_cq=" << _queues.size();
   return os;
}
