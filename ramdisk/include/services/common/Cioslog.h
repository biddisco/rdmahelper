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

//! \file  Cioslog.h
//! \brief Declaration and inline methods for bgcios::jobctl::Job class.

#ifndef CIOSLOG_H
#define CIOSLOG_H
#include <ramdisk/include/services/common/logging.h>
#include <ramdisk/include/services/MessageHeader.h>
#include <signal.h>
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <ramdisk/include/services/common/Thread.h>

extern char Flight_log_table[][256];
enum
{
    FL_INVALID=0,
#define STANDARD(name) name,
#define FLIGHTPRINT(name, format)   name,
#define FLIGHTFUNCT(name, function) name,
#include <ramdisk/include/services/common/flightlog.h>
#undef STANDARD
#undef FLIGHTPRINT
#undef FLIGHTFUNCT
    FL_NUMENTRIES
};

typedef struct BG_FlightRecorderFormatter
{
    const char* id_str;
} BG_FlightRecorderFormatter_t;

#ifdef SIMPLIFIED_CIOSLOG
  #include "utility/include/Log.h"
  LOG_DECLARE_FILE("jb.");
  #define CIOSLOGRDMA_REQ(ID,region,frags,fd) LOG_TRACE_MSG("CIOSLOGRDMA_REQ " << ID << " " << region << " " << frags << " " << fd)
  #define CIOSLOGMSG_WC(ID,wc)                LOG_TRACE_MSG("CIOSLOGMSG_WC   " << ID << " " << ((struct ibv_wc *)wc)->wr_id)
  #define CIOSLOGPOSTSEND(ID,send_wr,err)     LOG_TRACE_MSG("CIOSLOGPOSTSEND " << ID << " " << send_wr.wr_id << " " << err)
  #define CIOSLOGEVT_CH(ID,event)             LOG_TRACE_MSG("CIOSLOGEVT_CH   " << ID << " " << event)
#else
  //#define CIOSLOGMSG(ID,msg) printMsg(ID,(bgcios::MessageHeader *)msg)
  #define CIOSLOGMSG(ID,msg) logMsg(ID,(bgcios::MessageHeader *)msg)
  #define CIOSLOGMSG_RECV_WC(ID,msg,completion) logMsgWC(ID,(bgcios::MessageHeader *)msg, (struct ibv_wc *)completion)
  #define CIOSLOGMSG_QP(ID,msg,qpNum) logMsgQpNum(ID,(bgcios::MessageHeader *)msg, qpNum)
  #define CIOSLOGMSG_SG(ID,siginfo_ptr) logMsgSig(ID,siginfo_ptr)
  #define CIOSLOGEVT_CH(ID,event) logChanEvent(ID,event)
  #define CIOSLOGSTAT_SIG(ID,sig_no,source) logIntString(ID,sig_no,source)
  #define CIOSLOGRDMA_REQ(ID,region,frags,fd) logCRdmaReg(ID, region->addr, region->length, region->lkey, frags, fd );
  #define CIOSLOGPLUGIN(ID,v0,v1,v2,v3) log4values(ID, (uint64_t)v0,(uint64_t)v1,(uint64_t)v2,(uint64_t)v3)
  #define CIOSLOGPOSTSEND(ID,send_wr,err) logPostSend( ID, send_wr,err)
  #define CIOSLOG4(ID,v0,v1,v2,v3) log4values(ID, (uint64_t)v0,(uint64_t)v1,(uint64_t)v2,(uint64_t)v3)
  #define CIOSLOGMSG_WC(ID,wc) logWorkCompletion(ID,(struct ibv_wc *)wc)
#endif

void printMsg(const char * ID, bgcios::MessageHeader *mh);

typedef union cios_connection {
  uint32_t BGV_recv[2];
  uint64_t other;
} cios_connection_t;


typedef struct BG_FlightRecorderLog
{
    uint64_t timeStamp;
    uint32_t entry_num;
    uint32_t id;
    cios_connection_t ci;
    uint64_t data[4];
} BG_FlightRecorderLog_t;


uint32_t logMsg(uint32_t ID,bgcios::MessageHeader *mh);
uint32_t logMsgWC(uint32_t ID,bgcios::MessageHeader *mh,struct ibv_wc *wc);
uint32_t logMsgQpNum(uint32_t ID,bgcios::MessageHeader *mh,uint32_t qp_num);
uint32_t logMsgSig(uint32_t ID,siginfo_t * siginfo_ptr);
uint32_t logIntString(uint32_t ID,int int_val, const char * info);
uint32_t logChanEvent(uint32_t ID,struct rdma_cm_event * event);
void printLogMsg(const char * pathname);
void setFlightLogSize(unsigned int size);
void printLogEntry(uint32_t entry);
void printprevLogEntries(uint32_t entry,uint32_t quantity);
void printlastLogEntries(uint32_t quantity);
size_t snprintfLogEntry(size_t bufsize, char* buffer,uint32_t entry );
uint32_t logCRdmaReg(uint32_t ID, void* address, uint64_t length, uint32_t lkey, int frags, int filedescriptor );
uint32_t logPostSend(uint32_t ID, struct ibv_send_wr& send_wr, int err=0);
uint32_t log4values(uint32_t ID, uint64_t val0, uint64_t val1, uint64_t val2, uint64_t val3);
uint32_t logWorkCompletion(uint32_t ID,struct ibv_wc *wc);

//Class to monitor for dumping flight log
class FlightLogDumpWaiter : public bgcios::Thread { 
  public:
  FlightLogDumpWaiter(const char * daemon_name)
  { 
    strncpy(_daemon_name,daemon_name,20);
  }
  void * run(void);
  private:
  char _daemon_name[21];
};


#endif

