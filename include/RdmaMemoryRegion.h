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

//! \file  RdmaMemoryRegion.h 
//! \brief Declaration and inline methods for bgcios::RdmaMemoryRegion class.

#ifndef COMMON_RDMAMEMORYREGION_H
#define COMMON_RDMAMEMORYREGION_H

// Includes
#include <RdmaProtectionDomain.h>
#include <infiniband/verbs.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <memory>

namespace bgcios
{

//! Special memory region structure returned by the bgvrnic device.

struct bgvrnic_mr
{
   struct ibv_mr region;     //! Standard verbs memory region.
   uint32_t num_frags;       //! Number of fragments in the physical pages used by the memory region.
};

// Help for lame declaration for ibv_access_flags
const int _IBV_ACCESS_LOCAL_WRITE = IBV_ACCESS_LOCAL_WRITE;
const int _IBV_ACCESS_REMOTE_WRITE = IBV_ACCESS_REMOTE_WRITE;
const int _IBV_ACCESS_REMOTE_READ = IBV_ACCESS_REMOTE_READ;
const int _IBV_ACCESS_REMOTE_ATOMIC = IBV_ACCESS_REMOTE_ATOMIC;
const int _IBV_ACCESS_MW_BIND = IBV_ACCESS_MW_BIND;

//! \brief Memory region for RDMA operations.

class RdmaMemoryRegion
{
public:

   //! \brief  Default constructor.

   RdmaMemoryRegion()
   {
      _region        = NULL;
      _messageLength = 0;
      _flags         = 0;
      _frags         = 0;
      _fd            =-1;
   }

    RdmaMemoryRegion(struct ibv_mr *region, uint32_t messageLength)
    {
       _region        = region;
       _messageLength = messageLength;
       _flags         = 0;
       _frags         = 0;
       _fd            =-1;
    }

    // create a memory region object by registering an existing address buffer
    RdmaMemoryRegion(RdmaProtectionDomainPtr pd, const void *buffer, const uint64_t length);

   ~RdmaMemoryRegion()
   {
     release();
   }

   // overload operator->
   inline unsigned char* operator->() const
   {
     return _region != NULL ? (unsigned char*)(_region->addr) : NULL;
   }

   // overload operator*
   inline unsigned char* operator*() const
   {
     return _region != NULL ? (unsigned char*)(_region->addr) : NULL;
   }

   //! \brief  Check if memory region has been allocated.
   //! \return True if memory region is allocated, otherwise false.

   inline bool isAllocated(void) const { return _region != NULL ? true : false; }

   //! \brief  Allocate and register a memory region.
   //! \param  pd Protection domain for memory region.
   //! \param  length Length of the memory region.
   //! \return 0 when successful, errno when unsuccessful.

   int allocate(RdmaProtectionDomainPtr pd, size_t length);

   //! \brief  Allocate and register a memory region from the bgvrnic device
   //! \param  pd Protection domain for memory region.
   //! \param  length Length of the memory region.
   //! \return 0 when successful, errno when unsuccessful.

   int allocateFromBgvrnicDevice(RdmaProtectionDomainPtr pd, size_t length);

   //! \brief  Allocate and register a 64KB memory region.
   //! \param  pd Protection domain for memory region.
   //! \return 0 when successful, errno when unsuccessful.
   int allocate64kB(RdmaProtectionDomainPtr pd);

   //! \brief  Deregister and free the memory region.
   //! \return 0 when successful, errno when unsuccessful.

   int release(void);

   //! \brief  Get the address of the memory region.
   //! \return Address value.

   inline void *getAddress(void) const { return _region->addr; }

   //! \brief  Get the length of the memory region.
   //! \return Length value.

   inline uint32_t getLength(void) const { return (uint32_t)_region->length; }

   //! \brief  Get the local key of the memory region.
   //! \return Local key value.

   inline uint32_t getLocalKey(void) const { return _region->lkey; }

   //! \brief  Get the remote key of the memory region.
   //! \return Remote key value.

   inline uint32_t getRemoteKey(void) const { return _region->rkey; }

   //! \brief  Set the length of a message in the memory region.
   //! \param  length New message length value.
   //! \return Nothing.

   inline void setMessageLength(uint32_t length) { _messageLength = length; }

   //! \brief  Get the length of a message in the memory region.
   //! \return Message length value.

   inline uint32_t getMessageLength(void) const { return _messageLength; }

   //! \brief  Return indicator if a message is ready.
   //! \return True if a message is ready, otherwise false.

   inline bool isMessageReady(void) const { return _messageLength > 0 ? true : false; }

   enum {
       BLOCK_USER = 1,
       BLOCK_TEMP = 2,
   };

   //! Check is this region was constructed from user allocated memory
   inline bool isUserRegion() { return (_flags & BLOCK_USER) == BLOCK_USER; }
   inline bool isTempRegion() { return (_flags & BLOCK_TEMP) == BLOCK_TEMP; }
   inline void setTempRegion() { _flags |= BLOCK_TEMP; }

   //! \brief  Write info about the memory region to the specified stream.
   //! \param  os Output stream to write to.
   //! \return Output stream.

   std::ostream& writeTo(std::ostream& os) const;

private:

   //! Memory region.
   struct ibv_mr *_region;

   //! Number of memory fragments (0 if unknown)
   int _frags;

   //! File descriptor if using with mmap
   int _fd;

   //! flags to control lifetime of blocks
   int _flags;

   //! Length of a message in the memory region.
   uint32_t _messageLength;

   //! Maximum number of times to try allocating a memory region to reduce physical page fragmentation.
   static const uint32_t MaxAllocateAttempts = 16;
};

//! Smart pointer for RdmaMemoryRegion object.
typedef std::shared_ptr<RdmaMemoryRegion> RdmaMemoryRegionPtr;

//! \brief  RdmaMemoryRegion shift operator for output.

inline std::ostream& operator<<(std::ostream& os, const RdmaMemoryRegion& mr)
{
   return mr.writeTo(os);
}

} // namespace bgcios

#endif // COMMON_RDMAMEMORYREGION_H
