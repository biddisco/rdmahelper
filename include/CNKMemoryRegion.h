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

//! \file  RdmaMemoryRegion.h
//! \brief Declaration and inline methods for bgcios::RdmaMemoryRegion class.

#ifndef COMMON_RdmaMemoryRegion_H
#define COMMON_RdmaMemoryRegion_H

// Includes
#include <SystemLock.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <memory>
#include "rdma.h"

//! Access flag values for cnv_reg_mr().
enum cnv_access_flags {
   CNV_ACCESS_LOCAL_WRITE  = 1,        //!< Enable local write access.
   CNV_ACCESS_REMOTE_WRITE = (1<<1),   //!< Enable remote write access.
   CNV_ACCESS_REMOTE_READ  = (1<<2)    //!< Enable remote read access.
};

// Help for lame declaration for cnv_access_flags
const int _CNV_ACCESS_LOCAL_WRITE = CNV_ACCESS_LOCAL_WRITE;
const int _CNV_ACCESS_REMOTE_WRITE = CNV_ACCESS_REMOTE_WRITE;
const int _CNV_ACCESS_REMOTE_READ = CNV_ACCESS_REMOTE_READ;
//const int _CNV_ACCESS_REMOTE_ATOMIC = CNV_ACCESS_REMOTE_ATOMIC;
//const int _CNV_ACCESS_MW_BIND = CNV_ACCESS_MW_BIND;

//! \brief Memory region for RDMA operations.

class RdmaMemoryRegion
{
public:

   //! \brief  Default constructor.

  RdmaMemoryRegion()
  {
     _region.address = NULL;
     _messageLength  = 0;
     _frags          = 0;
     _fd             = -1;
  }

    RdmaMemoryRegion(int Rdma_fd, const void *buffer, const uint64_t length);

   ~RdmaMemoryRegion()
   {
     release();
   }

   //! \brief  Check if memory region has been allocated.
   //! \return True if memory region is allocated, otherwise false.

   bool isAllocated(void) const { return _region.address != NULL ? true : false; }

   //! \brief  Allocate and register a memory region.
   //! \param  pd Protection domain for memory region.
   //! \param  length Length of the memory region.
   //! \return 0 when successful, errno when unsuccessful.

   int allocate(int Rdma_fd, size_t length);

   //! \brief  Deregister and free the memory region.
   //! \return 0 when successful, errno when unsuccessful.

   int release(void);

   //! \brief  Get the address of the memory region.
   //! \return Address value.

   void *getAddress(void) const { return _region.address; }

   //! \brief  Get the length of the memory region.
   //! \return Length value.

   uint32_t getLength(void) const { return (uint32_t)_region.length; }

   //! \brief  Get the local key of the memory region.
   //! \return Local key value.

   uint32_t getLocalKey(void) const { return _region.lkey; }

   //! \brief  Get the remote key of the memory region.
   //! \return Remote key value.

//   uint32_t getRemoteKey(void) const { return _region->lkey; }

   //! \brief  Set the length of a message in the memory region.
   //! \param  length New message length value.
   //! \return Nothing.

   void setMessageLength(uint32_t length) { _messageLength = length; }

   //! \brief  Get the length of a message in the memory region.
   //! \return Message length value.

   uint32_t getMessageLength(void) const { return _messageLength; }

   //! \brief  Return indicator if a message is ready.
   //! \return True if a message is ready, otherwise false.

   bool isMessageReady(void) const { return _messageLength > 0 ? true : false; }

   //! \brief  Write info about the memory region to the specified stream.
   //! \param  os Output stream to write to.
   //! \return Output stream.

   std::ostream& writeTo(std::ostream& os) const;

private:

   //! Memory region.
   Kernel_RDMARegion_t _region;

   //! Number of memory fragments (0 if unknown)
   int _frags;

   //! File descriptor if using with mmap
   int _fd;

   //! Length of a message in the memory region.
   uint32_t _messageLength;

   //! MaximTum number of times to try allocating a memory region to reduce physical page fragmentation.
   static const uint32_t MaxAllocateAttempts = 16;
};

//! Smart pointer for RdmaMemoryRegion object.
typedef std::shared_ptr<RdmaMemoryRegion> RdmaMemoryRegionPtr;

//! \brief  RdmaMemoryRegion shift operator for output.

inline std::ostream& operator<<(std::ostream& os, const RdmaMemoryRegion& mr)
{
   return mr.writeTo(os);
}

#endif // COMMON_RdmaMemoryRegion_H
