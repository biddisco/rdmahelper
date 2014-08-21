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
/* (C) Copyright IBM Corp.  2012, 2012                              */
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

//! \file  UserMessages.h
//! \brief Declarations for bgcios::stdio message classes.

#ifndef USERMESSAGES_H
#define USERMESSAGES_H

// Includes
#include <ramdisk/include/services/MessageHeader.h>
#include <inttypes.h>

#ifdef __cplusplus
namespace bgcios
{
#endif

#define UserMessageDataSize  480
struct UserMessage
{
   struct MessageHeader header;                    //!< Message header 
   char MessageData[UserMessageDataSize];          //!< Message data, length in header = sizeof(struct MessageHeader) + amount of data
};

struct UserMessageRDMAxferBytes
{
   struct MessageHeader header;                    //!< Message header 
   uint64_t bytesXfer;                             //!< Bytes transferred in RDMA operation
};

struct UserRDMA {
  uint64_t cnk_address;                           //!< CNK memory address of a memory region within user CNK process space
  uint32_t cnk_bytes;                             //!< Number of bytes to CNK memory region in CNK user process space to apply operations
  uint32_t cnk_memkey;                            //!< CNK kernel key for RDMA
};

#define MostRdmaRegions  8
#define UserMessageFdRDMADataSize  340
struct UserMessageFdRDMA
{
   struct MessageHeader header;                    //!< Message header 
   int ionode_fd[2];                               //!< IO node file descriptor for function-ship applied to local CNK file descriptor 
   int numberOfRdmaRegions;                        //!< Number of CNK memory regions
   struct UserRDMA uRDMA[MostRdmaRegions];         //!< list of RDMA regions
   char MessageData[UserMessageFdRDMADataSize];    //!< Message data, remainder of the 512 bytes
};

#ifdef __cplusplus
} // namespace bgcios
#endif

#endif // USERMESSAGES_H


