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

//! \file  MessageUtility.h
//! \brief Functions for printing messages.

#ifndef MESSAGEUTILITY_H
#define MESSAGEUTILITY_H

// Includes
#include <ramdisk/include/services/MessageHeader.h>
#include <ramdisk/include/services/IosctlMessages.h>
#include <ramdisk/include/services/JobctlMessages.h>
#include <ramdisk/include/services/StdioMessages.h>
#include <ramdisk/include/services/SysioMessages.h>
#include <ramdisk/include/services/ToolctlMessages.h>

#include <sstream>
#include <string>

namespace bgcios
{

//! \brief  MessageHeader shift operator for output.
//! \param  m MessageHeader to print to output stream.

inline std::string printHeader(const MessageHeader& m)
{
   std::ostringstream os;
   os << "service=" << (int)m.service;
   os << " version=" << (int)m.version;
   os << " type=" << m.type;
   os << " rank=" << m.rank;
   os << " sequenceId=" << m.sequenceId;
   os << " returnCode=" << m.returnCode;
   os << " errorCode=" << m.errorCode;
   os << " length=" << m.length;
   os << " jobId=" << m.jobId;
   return os.str();
}

//! \brief  Convert a return code into a string.
//! \param  returnCode Return code value.
//! \return Return code string.

static const char* returnCodeToString(uint32_t returnCode)
{
   static const char* strings[] = {
      "Success (no error)",
      "Service value in message header is not valid",
      "Type value in message header is not supported",
      "Job id value is not valid",
      "Rank value is not valid",
      "Requested operation failed",
      "Sub-block job specifications are not valid",
      "Sending a message failed",
      "Receiving a message failed",
      "Protocol versions do not match",
      "Compute node is not ready for requested operation",
      "Setting secondary group id failed",
      "Setting primary group id failed",
      "Setting user id failed",
      "Changing to working directory failed",
      "Opening application executable failed",
      "No authority to application executable",
      "Reading data from application executable failed",
      "Application executable ELF header is wrong size",
      "Application executable ELF header contains invalid value",
      "Application executable contains no code sections",
      "Application executable code section is too big",
      "Application executable segment has wrong alignment",
      "Application executable has too many segments",
      "Generating static TLB map for application failed",
      "Initializing memory for process failed",
      "Argument list has too many items",
      "Starting tool process failed",
      "No authority to tool executable",
      "Tool id is not valid",
      "Timeout expired ending a tool",
      "Tool priority conflict",
      "Tool maximum number of tools exceeded",
      "Tool id conflict",
      "Creating /jobs directory failed",
      "Creating object in /jobs directory failed",
      "Tool priority level is not valid",
      "Tool attach request could not find requested target process",
      "Corner core number is not valid",
      "Number of cores allocated to a process is not valid",
      "Process is currently active on a hardware thread",
      "Number of processes on node is not valid",
      "Number of active ranks in job is not valid",
      "Class route data is not valid",
      "Tool number of commands is not valid",
      "Requested operation was partially successful",
      "Starting job prolog program process failed",
      "Job prolog program failed",
      "Starting job epilog program process failed",
      "Job epilog program failed",
      "Requested operation is currently in progress",
      "Control authority conflict with another tool",
      "No compute nodes matched job specifications",
      "Tool message contains an invalid combination of commands in the command list",
      "Tool control authority is required",
      "Tool release of control authority is required",
      "Map file is missing or contains invalid values",
      "Tool message not handled because target process is exiting",
   };

   if ((returnCode >= Success) && (returnCode < ReturnCodeListEnd)) {
      return strings[returnCode - Success];
   }
   return "INVALID RETURN CODE";
}

namespace iosctl
{

//! \brief  Convert a message type into a string.
//! \param  type Message type value.
//! \return Message type string.

static const char* toString(uint16_t type)
{
   static const char* strings[] = {
      "ErrorAck",
      "Ready",
      "ReadyAck",
      "Terminate",
      "TerminateAck",
      "StartNodeServices",
      "StartNodeServicesAck",
      "AllocateRegion",
      "AllocateRegionAck",
      "ReleaseRegion",
      "ReleaseRegionAck",
      "Interrupt",
      "InterruptAckInvalid",
   };

   if ((type >= ErrorAck) && (type <= InterruptAckInvalid)) {
      return strings[type - ErrorAck];
   }
   return "INVALID";
}

} // namespace iosctl

namespace jobctl
{

//! \brief  Convert a message type into a string.
//! \param  type Message type value.
//! \return Message type string.

static const char* toString(uint16_t type)
{
   static const char* strings[] = {
      "ErrorAck",
      "Authenticate",
      "AuthenticateAck",
      "LoadJob",
      "LoadJobAck",
      "StartJob",
      "StartJobAck",
      "StartTool",
      "StartToolAck",
      "EndTool",
      "EndToolAck",
      "SignalJob",
      "SignalJobAck",
      "CleanupJob",
      "CleanupJobAck",
      "ReconnectJobs",
      "ReconnectJobsAck",
      "ExitJob",
      "ExitJobAck",
      "DiscoverNode",
      "DiscoverNodeAck",
      "SetupJob",
      "SetupJobAck",
      "ExitProcess",
      "ExitProcessAck",
      "ChangeConfig",
      "ChangeConfigAck",
      "CheckToolStatus",
      "CheckToolStatusAck",
      "ExitTool",
      "ExitToolAck",
      "Heartbeat",
      "HeartbeatAck",
   };
   
   if (type >= ErrorAck && type <= HeartbeatAck) {
      return strings[type - ErrorAck];
   }
   if (type == Completed) {
      return "Completed";
   }
   return "INVALID";
}

} // namespace jobctl

namespace stdio
{

//! \brief  Convert a message type into a string.
//! \param  type Message type value.
//! \return Message type string.

static const char* toString(uint16_t type)
{
   static const char* strings[] = {
      "ErrorAck",
      "Authenticate",
      "AuthenticateAck",
      "StartJob",
      "StartJobAck",
      "ReadStdin",
      "ReadStdinAck",
      "WriteStdout",
      "WriteStdoutAck",
      "WriteStderr",
      "WriteStderrAck",
      "CloseStdio",
      "CloseStdioAck",
      "ReconnectJobs",
      "ReconnectJobsAck",
      "ChangeConfig",
      "ChangeConfigAck"
   };
   
   if ((type >= ErrorAck) && (type <= ChangeConfigAck)) {
      return strings[type - ErrorAck];
   }
   return "INVALID";
}

} // namespace stdio

namespace sysio
{

//! \brief  Convert a message type into a string.
//! \param  type Message type value.
//! \return Message type string.

static const char* toString(uint16_t type)
{
   static const char* strings[] = {
      "ErrorAck",
      "Access",
      "AccessAck",
      "Bind",
      "BindAck",
      "Chmod",
      "ChmodAck",
      "Chown",
      "ChownAck",
      "Close",
      "CloseAck",
      "Fchmod",
      "FchmodAck",
      "Fchown",
      "FchownAck",
      "Fstat64",
      "Fstat64Ack",
      "Fstatfs64",
      "Fstatfs64Ack",
      "Ftruncate64",
      "Ftruncate64Ack",
      "Fsync",
      "FsyncAck",
      "Getdents64",
      "Getdents64Ack",
      "Getpeername",
      "GetpeernameAck",
      "Getsockname",
      "GetsocknameAck",
      "Getsockopt",
      "GetsockoptAck",
      "Ioctl",
      "IoctlAck",
      "Link",
      "LinkAck",
      "Listen",
      "ListenAck",
      "Lseek64",
      "Lseek64Ack",
      "Mkdir",
      "MkdirAck",
      "Open",
      "OpenAck",
      "Readlink",
      "ReadlinkAck",
      "Rename",
      "RenameAck",
      "Resolvepath",
      "ResolvepathAck",
      "Setsockopt",
      "SetsockoptAck",
      "Shutdown",
      "ShutdownAck",
      "Socket",
      "SocketAck",
      "Stat64",
      "Stat64Ack",
      "Statfs64",
      "Statfs64Ack",
      "Symlink",
      "SymlinkAck",
      "Truncate64",
      "Truncate64Ack",
      "Unlink",
      "UnlinkAck",
      "Utime",
      "UtimeAck",
      "Accept",
      "AcceptAck",
      "Connect",
      "ConnectAck",
      "Fcntl",
      "FcntlAck",
      "Poll",
      "PollAck",
      "Select",
      "SelectAck",
      "Pread64",
      "Pread64Ack",
      "Pwrite64",
      "Pwrite64Ack",
      "Read",
      "ReadAck",
      "Recv",
      "RecvAck",
      "Recvfrom",
      "RecvfromAck",
      "Send",
      "SendAck",
      "Sendto",
      "SendtoAck",
      "Write",
      "WriteAck",
      "SetupJob",
      "SetupJobAck",
      "CleanupJob",
      "CleanupJobAck",
   };
   
   if ((type >= ErrorAck) && (type <= WriteAck)) {
      return strings[type - ErrorAck];
   } else if ((type >= SetupJob) && (type <= CleanupJobAck) ){ 
      // SetupJob et al have KINTERNALBIT set 
      return strings[type - KINTERNALBIT - ErrorAck];
   }
   return "INVALID";
}

} // namespace sysio

namespace toolctl
{

//! \brief  Convert a message type into a string.
//! \param  type Message type value.
//! \return Message type string.

static const char* toString(uint16_t type)
{
   static const char* strings[] = {
      "ErrorAck",
      "Attach",
      "AttachAck",
      "Detach",
      "DetachAck",
      "Query",
      "QueryAck",
      "Update",
      "UpdateAck",
      "SetupJob",
      "SetupJobAck",
      "Notify",
      "NotifyAck",
      "Control",
      "ControlAck",
   };

   if ((type >= ErrorAck) && (type <= ControlAck)) {
      return strings[type - ErrorAck];
   }
   return "INVALID";
}

} // namespace toolctl

//! \brief  Convert a message type into a string.
//! \param  msghdr Pointer to message header.
//! \return Message type string.

static const char* toString(MessageHeader *msghdr)
{
   switch (msghdr->service) {
      case bgcios::IosctlService:
         return bgcios::iosctl::toString(msghdr->type);

      case bgcios::JobctlService:
         return bgcios::jobctl::toString(msghdr->type);

      case bgcios::StdioService:
         return bgcios::stdio::toString(msghdr->type);

      case bgcios::SysioService:
         return bgcios::sysio::toString(msghdr->type);

      case bgcios::ToolctlService:
         return bgcios::toolctl::toString(msghdr->type);

      default:
         return "INVALID SERVICE";
   }
}

} // namespace bgcios

#endif // MESSAGEUTILITY_H

