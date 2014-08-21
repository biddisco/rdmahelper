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

//! \file  StdioMessages.h
//! \brief Declarations for bgcios::stdio message classes.

#ifndef STDIOMESSAGES_H
#define STDIOMESSAGES_H

// Includes
#include <ramdisk/include/services/MessageHeader.h>
#include <inttypes.h>

namespace bgcios
{

namespace stdio
{

const uint16_t ErrorAck           = 3000;
const uint16_t Authenticate       = 3001;
const uint16_t AuthenticateAck    = 3002;
const uint16_t StartJob           = 3003;
const uint16_t StartJobAck        = 3004;
const uint16_t ReadStdin          = 3005;
const uint16_t ReadStdinAck       = 3006;
const uint16_t WriteStdout        = 3007;
const uint16_t WriteStdoutAck     = 3008;
const uint16_t WriteStderr        = 3009;
const uint16_t WriteStderrAck     = 3010;
const uint16_t CloseStdio         = 3011;
const uint16_t CloseStdioAck      = 3012;
const uint16_t Reconnect          = 3013;
const uint16_t ReconnectAck       = 3014;
const uint16_t ChangeConfig       = 3015;
const uint16_t ChangeConfigAck    = 3016;

//! Base port number for RDMA connections.
const uint16_t BaseRdmaPort = 7101;

//! Current version of protocol.
const uint8_t ProtocolVersion = 4;

//! Size of unencrypted (plain text) data in Authenticate message.
const size_t PlainDataSize = 32;

//! Size of encrypted data in Authenticate message.
const size_t EncryptedDataSize = 64;

//! Message to acknowledge a message that is in error.

struct ErrorAckMessage
{
   struct MessageHeader header;        //!< Message header.
};

//! Message to authenticate connection.

struct AuthenticateMessage
{
   struct MessageHeader header;                    //!< Message header.
   unsigned char plainData[PlainDataSize];         //!< Unencrypted (plain text) data.
   unsigned char encryptedData[EncryptedDataSize]; //!< Encrypted data.
};

//! Message to acknowledge authenticating connection.

struct AuthenticateAckMessage
{
   struct MessageHeader header;        //!< Message header.
};

//! Message to start a job.

struct StartJobMessage
{
   struct MessageHeader header;        //!< Message header.
   uint32_t numRanksForIONode;         //!< Number of ranks using services on IO node.
};

//! Message to acknowledge starting a job.

struct StartJobAckMessage
{
   struct MessageHeader header;        //!< Message header.
};

//! Message to read data from standard input.

struct ReadStdinMessage
{
   struct MessageHeader header;        //!< Message header.
   size_t length;                      //!< Amount of data to read from standard input.
};

//! Message to acknowledge reading data from standard input.

struct ReadStdinAckMessage
{
   struct MessageHeader header;        //!< Message header.
   char data[bgcios::SmallMessageDataSize]; //!< Data read from standard input.
};

//! Message to write data to standard output or error.

struct WriteStdioMessage
{
   struct MessageHeader header;        //!< Message header.
   char data[bgcios::SmallMessageDataSize]; //!< Data to write to standard output or error.
};

//! Message to acknowledge writing data to standard output or error.

struct WriteStdioAckMessage
{
   struct MessageHeader header;        //!< Message header.
};

//! Message to indicate standard I/O is closed for job.

struct CloseStdioMessage
{
   struct MessageHeader header;        //!< Message header.
};

//! Message to acknowledge closing standard I/O.

struct CloseStdioAckMessage
{
   struct MessageHeader header;        //!< Message header.
};

//! Message to reconnect to a running I/O node.

struct ReconnectMessage
{
   struct MessageHeader header;        //!< Message header.
   uint32_t numJobs;                   //!< Number of jobs loaded or started on I/O node.
};

//! Message to acknowledge reconnecting to a running I/O node.

struct ReconnectAckMessage
{
   struct MessageHeader header;        //!< Message header.
};

//! Message to change the configuration of the daemon.

struct ChangeConfigMessage
{
   struct MessageHeader header;        //!< Message header.
   char stdiodTraceLevel;              //!< New ibm.cios.stdiod trace level.
   char commonTraceLevel;              //!< New ibm.cios.common trace level.
};

//! Message to acknowledge changing the configuration of the daemon.

struct ChangeConfigAckMessage
{
   struct MessageHeader header;        //!< Message header.
};

} // namespace stdio

} // namespace bgcios

#endif // STDIOMESSAGES_H


