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

//! \file  Cioslog.cc
//! \brief Declaration 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#include <ramdisk/include/services/ServicesConstants.h>
#include <ramdisk/include/services/MessageHeader.h>
//#include <ramdisk/include/services/SysioMessages.h>
//#include <ramdisk/include/services/JobctlMessages.h>
#include <ramdisk/include/services/StdioMessages.h>
//#include <ramdisk/include/services/IosctlMessages.h>
#include <Log.h>

#include <ramdisk/include/services/common/Cioslog.h>
#include <stdio.h>

#ifdef __PPC64__
#include <hwi/include/bqc/A2_inlines.h>
#endif

#include <infiniband/verbs.h>

LOG_DECLARE_FILE( "cios.logging" );

#include <ramdisk/include/services/common/SignalHandler.h>

using namespace bgcios;

static unsigned int FlightLogSize=16;

static BG_FlightRecorderLog_t dummy[16];

static BG_FlightRecorderLog_t * FlightLog = dummy;

static BG_FlightRecorderLog_t * BGV_RECV_MSG_entry = dummy;

void setFlightLogSize(unsigned int size){
  if (size==0) size=16;
  FlightLogSize=size;
  FlightLog = new BG_FlightRecorderLog_t[size];
}

const char * CIOS_FLIGHTLOG_FMT[] =
{
    "FL_INVALD", 
//using short form for function pointer assignment
#define STANDARD(name)  #name ,
#include <ramdisk/include/services/common/flightlog.h>
#undef STANDARD
  "FL_INLAST"
};
static volatile uint32_t total_entries = 0;
static uint32_t fl_index=0;
static uint32_t wrapped=0;
static int log_fd= 2;  //stdout default

#ifdef __PPC64__
#define LLUS long long unsigned int
void printMsg(uint32_t ID, bgcios::MessageHeader *mh){
   printf("%s: type=%u rank=%u jobid=%llu sequenceId=%d length=%d timestamp=%llu\n", CIOS_FLIGHTLOG_FMT[ID],(int)mh->type,(int)mh->rank,(LLUS)mh->jobId, (int)mh->sequenceId,(int)mh->length,(LLUS)GetTimeBase());
}
#undef LLUS
#endif

#ifdef __x86_64__
#define LLUS long long unsigned int
static uint64_t GetTimeBase() { return 0; }
void printMsg(uint32_t ID, bgcios::MessageHeader *mh){
// Don't do anything for x86.
  printf("%s: type=%u rank=%u jobid=%llu sequenceId=%d length=%d timestamp=%llu\n", CIOS_FLIGHTLOG_FMT[ID],(int)mh->type,(int)mh->rank,(LLUS)mh->jobId, (int)mh->sequenceId,(int)mh->length,(LLUS)GetTimeBase());
}

#undef LLUS
#endif


uint32_t logPostSend(uint32_t ID, struct ibv_send_wr& send_wr,int err){
   uint32_t index = fl_index++;
   BG_FlightRecorderLog_t * entry = &FlightLog[index];
   entry->entry_num = total_entries++;
   entry->id = ID;
   entry->timeStamp = GetTimeBase();
   uint64_t * sge = (uint64_t *)send_wr.sg_list;
   entry->data[0] = sge[0];
   entry->data[1] = sge[1];
   uint64_t * save = (uint64_t *)&send_wr.send_flags;
   entry->data[2] = save[0];
   entry->data[3] = save[1];

   //grab some other helpful data
   entry->ci.BGV_recv[0] = send_wr.opcode;
   entry->ci.BGV_recv[1] = send_wr.wr.rdma.rkey;
   if (err){
      entry->id = BGV_POST_ERR;
      entry->ci.BGV_recv[1]=(uint32_t)err;
      if (err==ENOMEM) {
        LOG_ERROR_MSG("Post Send Error " << err << " ENOMEM")
      }
      else {
        LOG_ERROR_MSG("Post Send Error " << err << " " <<  bgcios::errorString(err))
      }
      throw bgcios::errorString(err);
   }

   if (fl_index >= FlightLogSize){
     fl_index=0;
     wrapped=1;
   }
   return index;
}

uint32_t log4values(uint32_t ID, uint64_t val0, uint64_t val1, uint64_t val2, uint64_t val3){
   uint32_t index = fl_index++;
   BG_FlightRecorderLog_t * entry = &FlightLog[index];
   entry->entry_num = total_entries++;
   entry->id = ID;
   entry->timeStamp = GetTimeBase();
   entry->data[0] = val0;
   entry->data[1] = val1;
   entry->data[2] = val2;
   entry->data[3] = val3;

   //grab some other helpful data
   entry->ci.other = (uint64_t)getpid();

   if (fl_index >= FlightLogSize){
     fl_index=0;
     wrapped=1;
   }
   return index;
}


uint32_t logMsg(uint32_t ID,bgcios::MessageHeader *mh){
   uint32_t index = fl_index++;
   BG_FlightRecorderLog_t * entry = &FlightLog[index];
   entry->entry_num = total_entries++;
   entry->id = ID;
   entry->timeStamp = GetTimeBase();
   uint64_t * msg = (uint64_t *)mh;
   entry->data[0] = msg[0];
   entry->data[1] = msg[1];
   entry->data[2] = msg[2];
   entry->data[3] = msg[3];
   entry->ci.other = 0;  //not using other...
   if (fl_index >= FlightLogSize){
     fl_index=0;
     wrapped=1;
   }
   return index;
}

uint32_t logMsgWC(uint32_t ID,bgcios::MessageHeader *mh,struct ibv_wc *wc){
   uint32_t index = fl_index++;
   BG_FlightRecorderLog_t * entry = &FlightLog[index];
   entry->entry_num = total_entries++;
   entry->id = ID;
   entry->timeStamp = GetTimeBase();
   uint64_t * msg = (uint64_t *)mh;
   entry->data[0] = msg[0];
   entry->data[1] = msg[1];
   entry->data[2] = msg[2];
   entry->data[3] = msg[3];
   entry->ci.BGV_recv[0] = //remote QP number unknown...
   entry->ci.BGV_recv[1] = wc->qp_num; //local QP number
   if (fl_index >= FlightLogSize){
     fl_index=0;
     wrapped=1;
   }
   return index;
}

uint32_t logMsgQpNum(uint32_t ID,bgcios::MessageHeader *mh,uint32_t qp_num){
   uint32_t index = fl_index++;
   BG_FlightRecorderLog_t * entry = &FlightLog[index];
   entry->entry_num = total_entries++;
   entry->id = ID;
   entry->timeStamp = GetTimeBase();
   uint64_t * msg = (uint64_t *)mh;
   entry->data[0] = msg[0];
   entry->data[1] = msg[1];
   entry->data[2] = msg[2];
   entry->data[3] = msg[3];
   entry->ci.BGV_recv[0] = 0; //remote QP number unknown...
   entry->ci.BGV_recv[1] = qp_num; //local QP number
   if (fl_index >= FlightLogSize){
     fl_index=0;
     wrapped=1;
   }
   return index;
}

uint32_t logWorkCompletion(uint32_t ID,struct ibv_wc *wc){
   uint32_t index = fl_index++;
   BG_FlightRecorderLog_t * entry = &FlightLog[index];
   entry->entry_num = total_entries++;
   entry->id = ID;
   entry->timeStamp = GetTimeBase();
   uint64_t * msg = (uint64_t *)wc;
   entry->data[0] = msg[0];
   entry->data[1] = msg[1];
   entry->data[2] = msg[2];
   entry->data[3] = msg[3];
   entry->ci.BGV_recv[0] = wc->src_qp; //remote QP number unknown...
   entry->ci.BGV_recv[1] = wc->qp_num; //local QP number
   if (fl_index >= FlightLogSize){
     fl_index=0;
     wrapped=1;
   }
   return index;
}

uint32_t logMsgSig(uint32_t ID,siginfo_t * siginfo_ptr){
   uint32_t index = fl_index++;
   BG_FlightRecorderLog_t * entry = &FlightLog[index];
   entry->entry_num = total_entries++;
   entry->id = ID;
   entry->timeStamp = GetTimeBase();
   uint64_t * msg = (uint64_t *)siginfo_ptr;
   entry->data[0] = msg[0];
   entry->data[1] = msg[1];
   entry->data[2] = msg[2];
   entry->data[3] = msg[3];
   entry->ci.other = (uint64_t)getpid();
   if (fl_index >= FlightLogSize){
     fl_index=0;
     wrapped=1;
   }
   return index;
}

uint32_t logIntString(uint32_t ID,int int_val, const char * info){
   uint32_t index = fl_index++;
   BG_FlightRecorderLog_t * entry = &FlightLog[index];
   entry->entry_num = total_entries++;
   entry->id = ID;
   entry->timeStamp = GetTimeBase();
   entry->ci.other = (uint64_t)int_val;
   memcpy(entry->data,info,32);
   return index;
}

struct chanEventLog{
	enum rdma_cm_event_type	 event;
	int			 status;
        uint32_t                 qp;
        short int                lcl_port;
        short int                rmt_port; 
        int                     lcl_addr;
        int                      rmt_addr;                     
};

uint32_t logChanEvent(uint32_t ID,struct rdma_cm_event * event){
   uint32_t index = fl_index++;
   BG_FlightRecorderLog_t * entry = &FlightLog[index];
   entry->entry_num = total_entries++;
   entry->id = ID;
   entry->timeStamp = GetTimeBase();

   entry->ci.other = (uint64_t)event->id;

   struct chanEventLog * cel = (struct chanEventLog *)entry->data;
   if ( (event->id!=NULL)&& (event->id->qp!=NULL) ){
       cel->qp=event->id->qp->qp_num;
   }
   
   cel->event = event->event;
   cel->status = event->status;
   struct sockaddr_in * sin = (struct sockaddr_in *)&event->id->route.addr.src_addr;
   struct sockaddr_in * sout = (struct sockaddr_in *)&event->id->route.addr.dst_addr;
   switch(cel->event){
   case RDMA_CM_EVENT_CONNECT_REQUEST:
     if (sin){ 
       cel->lcl_port = (short int)sin->sin_port;
       memcpy(&cel->lcl_addr,&sin->sin_addr,sizeof(struct in_addr));
     }
     if (sout){
      cel->rmt_port= (short int)sout->sin_port;
      memcpy(&cel->rmt_addr,&sout->sin_addr,sizeof(struct in_addr));
     }
     break;
     default: break;
   }
   if (fl_index >= FlightLogSize){
     fl_index=0;
     wrapped=1;
   }
   return index;
}

uint32_t logCRdmaReg(uint32_t ID, void* address, uint64_t length, uint32_t lkey, int frags, int filedescriptor ){
   uint32_t index = fl_index++;
   BG_FlightRecorderLog_t * entry = &FlightLog[index];
   entry->entry_num = total_entries++;
   entry->id = ID;
   entry->timeStamp = GetTimeBase();

   entry->ci.other = (uint64_t)filedescriptor;
   entry->data[0] = (uint64_t)address;
   entry->data[1] = length;
   entry->data[2] = uint64_t(lkey);
   entry->data[3] = uint64_t(frags);

   if (fl_index >= FlightLogSize){
     fl_index=0;
     wrapped=1;
   }
   return index;
}

size_t rdmaReg_decoder(size_t bufsize, char* buffer, const BG_FlightRecorderLog_t* log)
{

#define LLUS long long unsigned int
   size_t length = (size_t)snprintf(buffer,bufsize,"address=%p length=%llu lkey=%llu frags=%llu fd=%lld\n",(void *)log->data[0],(LLUS)log->data[1],(LLUS)log->data[2],(LLUS)log->data[3], (long long signed int)log->ci.other); 
#undef LLUS
   buffer += length;
   bufsize -= length;
   return length;
}
size_t Flight_CIOS_EvtDecoder(size_t bufsize, char* buffer, const BG_FlightRecorderLog_t* log)
{
   struct chanEventLog * cel = (struct chanEventLog *)log->data;
   size_t length = (size_t)snprintf(buffer,bufsize,"cm_id=%p",(void * )log->ci.other); 
   buffer += length;
   bufsize -= length;

   switch (cel->event){
   case RDMA_CM_EVENT_CONNECT_REQUEST:
     length = (size_t)snprintf(buffer,bufsize," RDMA_CM_EVENT_CONNECT_REQUEST(%d)",cel->event); break;
   case RDMA_CM_EVENT_DISCONNECTED:
     length = (size_t)snprintf(buffer,bufsize," RDMA_CM_EVENT_DISCONNECTED(%d)",cel->event); break;
   case RDMA_CM_EVENT_ESTABLISHED:
     length = (size_t)snprintf(buffer,bufsize," RDMA_CM_EVENT_ESTABLISHED(%d)",cel->event); break;
   case RDMA_CM_EVENT_REJECTED:
     length = (size_t)snprintf(buffer,bufsize," RDMA_CM_EVENT_REJECTED(%d)",cel->event); break;
   default:
     length = (size_t)snprintf(buffer,bufsize," event(%d)",cel->event); 
   }
   buffer += length;
   bufsize -= length;

   if (cel->status){
      length = (size_t)snprintf(buffer,bufsize," status(%d)",cel->status);
      buffer += length;
      bufsize -= length;
   }
 
   char * l = (char *)&cel->lcl_addr;
   char * r = (char *)&cel->rmt_addr;
   length = (size_t)snprintf(buffer, bufsize, "lcl=%hhu.%hhu.%hhu.%hhu:%d rmt=%hhu.%hhu.%hhu.%hhu:%d",l[0],l[1],l[2],l[3], cel->lcl_port,r[0],r[1],r[2],r[3],cel->rmt_port );
   buffer += length;
   bufsize -= length;  

   if (cel->qp){
     length = (size_t)snprintf(buffer, bufsize, " lQP=%d ",cel->qp );
     buffer += length;
     bufsize -= length;  
   }

   length = (size_t)snprintf(buffer,bufsize," \n");
   buffer += length;
   bufsize -= length;

   return length;
}

size_t postSendDecoder(size_t bufsize, char* buffer, const BG_FlightRecorderLog_t* log)
{
   struct ibv_sge * sge = (struct ibv_sge *)log->data;
   int send_flags = (int)(log->data[2]>>32);
   void * imm_data = (void *)(log->data[2] & 0xFFFFFFFF);
   void * remote_addr = (void *)log->data[3];
   int opcode = (int)log->ci.BGV_recv[0];
   uint32_t rkey_or_error = log->ci.BGV_recv[1];

   size_t length = (size_t)snprintf(buffer,bufsize,"immData=%p laddr=%p len=%u lkey=%u raddr=%p",imm_data,(void *)sge->addr,(unsigned int)sge->length,(unsigned int)sge->lkey,remote_addr); 
   buffer += length;
   bufsize -= length;

   if (log->id==BGV_POST_ERR){
      length = (size_t)snprintf(buffer,bufsize," error=%d",rkey_or_error); 
      buffer += length;
      bufsize -= length;
   }
   else {
      length = (size_t)snprintf(buffer,bufsize," rkey=%d",rkey_or_error); 
      buffer += length;
      bufsize -= length;
   }

   //opcode
   switch(opcode){
   case IBV_WR_RDMA_WRITE:
      length = (size_t)snprintf(buffer,bufsize," RDMA_WRITE(%d)",opcode); 
      buffer += length;
      bufsize -= length;
      break;
   case IBV_WR_RDMA_READ:
      length = (size_t)snprintf(buffer,bufsize," RDMA_READ(%d)",opcode); 
      buffer += length;
      bufsize -= length;
      break;
   default:
      length = (size_t)snprintf(buffer,bufsize," opcode=%d",opcode); 
      buffer += length;
      bufsize -= length;
      break;
   }

   length = (size_t)snprintf(buffer,bufsize," sendflags(%d)",send_flags); 
   buffer += length;
   bufsize -= length;

   if(send_flags&IBV_SEND_FENCE){
      length = (size_t)snprintf(buffer,bufsize,":FENCE"); 
      buffer += length;
      bufsize -= length;
   }
   if(send_flags&IBV_SEND_SIGNALED){
      length = (size_t)snprintf(buffer,bufsize,":SIGNALED"); 
      buffer += length;
      bufsize -= length;
   }
   if(send_flags&IBV_SEND_SOLICITED){
      length = (size_t)snprintf(buffer,bufsize,":SOLICITED"); 
      buffer += length;
      bufsize -= length;
   }
   if(send_flags&IBV_SEND_INLINE){
      length = (size_t)snprintf(buffer,bufsize,":INLINE"); 
      buffer += length;
      bufsize -= length;
   }
   length = (size_t)snprintf(buffer,bufsize,"\n"); 
   buffer += length;
   bufsize -= length;

   return length;
}


size_t Flight_CIOS_SigDecoder(size_t bufsize, char* buffer, const BG_FlightRecorderLog_t* log)
{
   siginfo_t * sg = (siginfo_t * )log->data;
   size_t length = (size_t)snprintf(buffer,bufsize,"signal(%ld) errno(%ld) sender-pid=%ld \n" ,(long int)sg->si_signo , (long int)sg->si_errno,(long int)sg->si_pid); 
   buffer += length;
   bufsize -= length;

   return length;
}

size_t Flight_CIOS_MsgDecoder(size_t bufsize, char* buffer, const BG_FlightRecorderLog_t* log, bool doendl=true);

size_t Flight_CIOS_MsgDecoder(size_t bufsize, char* buffer, const BG_FlightRecorderLog_t* log, bool doendl)
{   


    bgcios::MessageHeader * mh = (bgcios::MessageHeader *)log->data;
    char * buffer_start = buffer;

    size_t length = (size_t)snprintf(buffer, bufsize, "entry=%8x timestamp=%16llx %s: ",log->entry_num,(long long unsigned int)log->timeStamp,CIOS_FLIGHTLOG_FMT[log->id]);
    buffer += length;
    bufsize -= length;

    switch(log->id){
      case CFG_PLG_INVL:
        length = (size_t)snprintf(buffer, bufsize, "NO PLUGIN LOADED--INVALID _serviceId=%llu \n",(long long unsigned int)log->data[0]);
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
      break;
      case CFG_PLG_PATH:
        length = (size_t)snprintf(buffer, bufsize, "NO PLUGIN LOADED--PATH _serviceId=%llu \n",(long long unsigned int)log->data[0]);
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
      break;
      case CFG_PLG_SYMS:
        length = (size_t)snprintf(buffer, bufsize, "NO PLUGIN LOADED--MISSING SYMBOLS _serviceId=%llu \n",(long long unsigned int)log->data[0]);
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
      break;
      case CFG_PLGINADD:
        length = (size_t)snprintf(buffer, bufsize, "ADDED PLUGIN _serviceId=%llu fl=%p\n",(long long unsigned int)log->data[0],(void*)log->data[1]);
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
      break;
      case CFG_PLGINDFT:
        length = (size_t)snprintf(buffer, bufsize, "NO PLUGIN LOADED _serviceId=%llu  \n",(long long unsigned int)log->data[0]);
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
        break;
      case BGV_POST_SND:
      case BGV_POST_ERR:
      case BGV_POST_WRR:
      case BGV_POST_RDR:
        postSendDecoder(bufsize, buffer, log);
        return (size_t)strlen(buffer_start);
        break;
      case BGV_RDMA_REG:
      case BGV_RDMA_RMV:
      case BGV_RDMADROP:
        rdmaReg_decoder(bufsize, buffer, log);
        return (size_t)strlen(buffer_start);
      break;
      case BGV_RECV_SIG :
        Flight_CIOS_SigDecoder(bufsize, buffer, log);
        return (size_t)strlen(buffer_start);
        break;
      case BGV_STAT_SIG:
        length = (size_t)snprintf(buffer, 32, "%s",(char *)log->data);
        buffer += length;
        bufsize -= length;
        length = (size_t)snprintf(buffer, bufsize, " sent signal %llu\n",(long long unsigned int)log->ci.other);
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
        break;
      case BGV_RECV_EVT:
        Flight_CIOS_EvtDecoder(bufsize, buffer, log);
        return (size_t)strlen(buffer_start);
        break;

      case SYS_CALL_PWR:
      case SYS_CALL_WRT:
      case SYS_CALL_SND:
      case SYS_CALL_PRD:
      case SYS_CALL_RED:
      case SYS_CALL_RCV:
        length = (size_t)snprintf(buffer, bufsize, "region@=%p length=%p offset=%p xtra=%p\n",(void*)log->data[0],(void*)log->data[1],(void*)log->data[2],(void*)log->data[3]);
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
        break;
      case PROC_RMV_PID:
      case PROC_ADD_PID:
        length = (size_t)snprintf(buffer, bufsize, "pid=%llu \n",(long long unsigned int)log->data[0]);
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
        break;
      case PROC_BGN_SVC:
      {
        long long unsigned int torus = (long long unsigned int)log->data[1];
        length = (size_t)snprintf(buffer, bufsize, "service=%llu --CNtorus=(%llu %llu %llu %llu %llu)\n",(long long unsigned int)log->data[0],((torus>>24) & 0x3F),((torus>>18) & 0x3F),((torus>>12) & 0x3F),((torus>>6) & 0x3F),(torus & 0x3F));
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
      }
        break;
      case SYS_RSLT_PWR:
      case SYS_RSLT_WRT:
      case SYS_RSLT_SND:
      case SYS_RSLT_PRD:
      case SYS_RSLT_RED:
      case SYS_RSLT_RCV:
        length = (size_t)snprintf(buffer, bufsize, "region@=%p length=%p,offset=%p,xtra=%p\n",(void*)log->data[0],(void*)log->data[1],(void*)log->data[2],(void*)log->data[3]);
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
        break;
      case BGV_WORK_CMP:
        {
        struct ibv_wc * wce = (struct ibv_wc *)log->data;
        length = (size_t)snprintf(buffer, bufsize, "wr_id=%p status=%d opcode=%d vendor_err=%d, byte_len=%d,imm_data=%d, qp_num=%d\n",(void *)wce->wr_id,wce->status, wce->opcode, wce->vendor_err, wce->byte_len, wce->imm_data,wce->qp_num);
        buffer += length;
        bufsize -= length;
        return (size_t)strlen(buffer_start);
        } break;
      default: break;
    }

    length = (size_t)snprintf(buffer, bufsize, "svc=%ld type=%ld rank=%d seq=%ld msglen=%ld jobid=%lld",(long int)mh->service,(long int)mh->type,(int)mh->rank,(long int)mh->sequenceId,(long int)mh->length,(long long int)mh->jobId);    
    buffer += length;
    bufsize -= length;

    if (mh->errorCode){     
      switch (mh->errorCode){
       case ENOENT:
         length = (size_t)snprintf(buffer,bufsize,">ENOENT(%ld)",(long int)mh->errorCode); break;
       case EINTR:
         length = (size_t)snprintf(buffer,bufsize,">EINTR(%ld)",(long int)mh->errorCode); break;
       case ECONNREFUSED:
         length = (size_t)snprintf(buffer,bufsize,">ECONNREFUSED(%ld)",(long int)mh->errorCode); break;
       case EHOSTUNREACH:
         length = (size_t)snprintf(buffer,bufsize,">EHOSTUNREACH(%ld)",(long int)mh->errorCode); break;
       case EINVAL:
         length = (size_t)snprintf(buffer,bufsize,">EINVAL(%ld)",(long int)mh->errorCode); break;
       case ESPIPE:
         length = (size_t)snprintf(buffer,bufsize,">ESPIPE(%ld)",(long int)mh->errorCode); break;
       case EEXIST:
         length = (size_t)snprintf(buffer,bufsize,">EEXIST(%ld)",(long int)mh->errorCode); break;
       case ENOTCONN:
         length = (size_t)snprintf(buffer,bufsize,">ENOTCONN(%ld)",(long int)mh->errorCode); break;
       case ESTALE:
         length = (size_t)snprintf(buffer,bufsize,">ESTALE(%ld)",(long int)mh->errorCode); break;
       case EBUSY:
         length = (size_t)snprintf(buffer,bufsize,">EBUSY(%ld)",(long int)mh->errorCode); break;
       case EAGAIN:
         length = (size_t)snprintf(buffer,bufsize,">EAGAIN(%ld)",(long int)mh->errorCode); break;
       case ENOSPC:
         length = (size_t)snprintf(buffer,bufsize,">ENOSPC(%ld)",(long int)mh->errorCode); break;
       default:
         length = (size_t)snprintf(buffer,bufsize,">EC(%ld)",(long int)mh->errorCode);
      }
      buffer += length;
      bufsize -= length;
    }

    if (mh->returnCode){
      length = (size_t)snprintf(buffer,bufsize," rc=%ld",(long int)mh->returnCode);
      buffer += length;
      bufsize -= length;
    }

    char * text = NULL;
    switch(mh->type){
/*
        case iosctl::ErrorAck: text=(char *)"iosctl::ErrorAck";break;
        case jobctl::ErrorAck: text=(char *)"jobctl::ErrorAck";break;

        case iosctl::Ready: text=(char *)"iosctl::Ready";break;
        case iosctl::StartNodeServices: text=(char *)"iosctl::StartNodeServices";break;
        case iosctl::Interrupt: text=(char *)"iosctl::Interrupt";break;

        case jobctl::LoadJob: text=(char *)"jobctl::LoadJob"; break;
        case jobctl::LoadJobAck: text=(char *)"jobctl::LoadJobAck"; break;
        case jobctl::StartJob: text=(char *)"jobctl::StartJob"; break;
        case jobctl::StartJobAck: text=(char *)"jobctl::StartJobAck"; break;
        case jobctl::ExitJob: text=(char *)"jobctl::ExitJob"; break;
        case jobctl::ExitJobAck: text=(char *)"jobctl::ExitJobAck"; break;
        case jobctl::ExitProcess: text=(char *)"jobctl::ExitProcess"; break;
        case jobctl::ExitProcessAck: text=(char *)"jobctl::ExitProcessAck"; break;

        case jobctl::SignalJob: text=(char *)"jobctl::SignalJob";break;
        case jobctl::SignalJobAck: text=(char *)"jobctl::SignalJobAck";break;
        case jobctl::CleanupJob: text=(char *)"jobctl::CleanupJob";break;
        case jobctl::CleanupJobAck: text=(char *)"jobctl::CleanupJobAck";break;
        case jobctl::Authenticate: text=(char *)"jobctl::Authenticate";break;
        case jobctl::AuthenticateAck: text=(char *)"jobctl::AuthenticateAck";break;

        case jobctl::StartTool: text=(char *)"jobctl::StartTool";break;
        case jobctl::StartToolAck: text=(char *)"jobctl::StartToolAck";break;
        case jobctl::EndTool: text=(char *)"jobctl::EndTool";break;
        case jobctl::EndToolAck: text=(char *)"jobctl::EndToolAck";break;
        case jobctl::ExitTool: text=(char *)"jobctl::ExitTool";break;
        case jobctl::ExitToolAck: text=(char *)"jobctl::ExitToolAck";break;
        case jobctl::CheckToolStatus: text=(char *)"jobctl::CheckToolStatus";break;
        case jobctl::CheckToolStatusAck: text=(char *)"jobctl::CheckToolStatusAck";break;

        case jobctl::Reconnect: text=(char *)"jobctl::Reconnect";break;
        case jobctl::ReconnectAck: text=(char *)"jobctl::ReconnectAck";break;
*/
        case stdio::ErrorAck: text=(char *)"ErrorAck";break;
        case stdio::WriteStdout:text=(char *)"stdio::WriteStdout";break;
        case stdio::WriteStdoutAck: text=(char *)"stdio::WriteStdoutAck";break;
        case stdio::WriteStderr: text=(char *)"stdio::WriteStderr";break;
        case stdio::WriteStderrAck: text=(char *)"stdio::WriteStderrAck";break;
        case stdio::CloseStdio: text=(char *)"stdio::CloseStdio";break;
        case stdio::CloseStdioAck: text=(char *)"stdio::CloseStdioAck";break;
        case stdio::Authenticate: text=(char *)"stdio::Authenticate";break;
        case stdio::AuthenticateAck: text=(char *)"stdio::AuthenticateAck";break;
        case stdio::StartJob: text=(char *)"stdio::StartJob";break;
        case stdio::StartJobAck: text=(char *)"stdio::StartJobAck";break;
        case stdio::Reconnect: text=(char *)"stdio::Reconnect";break;
        case stdio::ReconnectAck: text=(char *)"stdio::ReconnectAck";break;
/*
        case jobctl::DiscoverNode: text=(char *)"jobctl::DiscoverNode"; break;
        case jobctl::DiscoverNodeAck: text=(char *)"jobctl::DiscoverNodeAck";break;
        case jobctl::SetupJob: text=(char *)"jobctl::SetupJob"; break;
        case jobctl::SetupJobAck: text=(char *)"jobctl::SetupJobAck";break;
 
        case jobctl::Completed: text=(char *)"jobctl::Completed"; break;

#define JOBCTL(type) case jobctl::type: text=(char *)"jobctl::"#type;break; 
        JOBCTL(Heartbeat);
        JOBCTL(HeartbeatAck);


        case sysio::ErrorAck: text=(char *)"sysio::ErrorAck";break;

        case sysio::WriteKernelInternal: text=(char *)"sysio::WriteKernelInternal";break;
        case sysio::OpenKernelInternal: text=(char *)"sysio::OpenKernelInternal";break;
        case sysio::CloseKernelInternal: text=(char *)"sysio::CloseKernelInternal";break;

        case sysio::SetupJob: text=(char *)"sysio::SetupJob";break;
        case sysio::SetupJobAck: text=(char *)"sysio::SetupJobAck";break;
        case sysio::CleanupJob: text=(char *)"sysio::CleanupJob";break;
        case sysio::CleanupJobAck: text=(char *)"sysio::CleanupJobAck";break;
        case sysio::Open: text=(char *)"sysio::Open";break;
        case sysio::OpenAck: text=(char *)"sysio::OpenAck";break;
        case sysio::Close: text=(char *)"sysio::Close";break;
        case sysio::CloseAck: text=(char *)"sysio::CloseAck";break;
        case sysio::Socket: text=(char *)"sysio::Socket";break;
        case sysio::SocketAck: text=(char *)"sysio::SocketAck";break;

        case sysio::Write: text=(char *)"sysio::Write";break;
        case sysio::WriteAck: text=(char *)"sysio::WriteAck";break;
        case sysio::Pwrite64: text=(char *)"sysio::Pwrite64";break;
        case sysio::Pwrite64Ack: text=(char *)"sysio::Pwrite64Ack";break;

        case sysio::Pread64: text=(char *)"sysio::Pread64";break;
        case sysio::Pread64Ack: text=(char *)"sysio::Pread64Ack";break;

        case sysio::Read: text=(char *)"sysio::Read";break;
        case sysio::ReadAck: text=(char *)"sysio::ReadAck";break;
        case sysio::Recv: text=(char *)"sysio::Recv";break;
        case sysio::RecvAck: text=(char *)"sysio::RecvAck";break;
        case sysio::Send: text=(char *)"sysio::Send";break;
        case sysio::SendAck: text=(char *)"sysio::SendAck";break;

        case sysio::Lseek64: text=(char *)"sysio::Lseek64";break;
        case sysio::Lseek64Ack: text=(char *)"sysio::Lseek64Ack";break;
        case sysio::Stat64: text=(char *)"sysio::Stat64";break;
        case sysio::Stat64Ack: text=(char *)"sysio::Stat64Ack";break;
        case sysio::Fstat64: text=(char *)"sysio::Fstat64";break;
        case sysio::Fstat64Ack: text=(char *)"sysio::Fstat64Ack";break;
        case sysio::Poll: text=(char *)"sysio::Poll";break;
        case sysio::PollAck: text=(char *)"sysio::PollAck";break;
        case sysio::Readlink: text=(char *)"sysio::Readlink";break;
        case sysio::ReadlinkAck: text=(char *)"sysio::ReadlinkAck";break;
        case sysio::Fcntl: text=(char *)"sysio::Fcntl";break;
        case sysio::FcntlAck: text=(char *)"sysio::FcntlAck";break;
        case sysio::Unlink: text=(char *)"sysio::Unlink";break;
        case sysio::UnlinkAck: text=(char *)"sysio::UnlinkAck";break;
        case sysio::Link: text=(char *)"sysio::Link";break;
        case sysio::LinkAck: text=(char *)"sysio::LinkAck";break;
        case sysio::Symlink: text=(char *)"sysio::Symlink";break;
        case sysio::SymlinkAck: text=(char *)"sysio::SymlinkAck";break;
        
#define SYSIO(type) case sysio::type: text=(char *)"sysio::"#type;break; 
         SYSIO(FsetXattr);
         SYSIO(FsetXattrAck);
         SYSIO(FgetXattr);
         SYSIO(FgetXattrAck);
         SYSIO(FremoveXattr);
         SYSIO(FremoveXattrAck);
         SYSIO(FlistXattr);
         SYSIO(FlistXattrAck);

         SYSIO(LsetXattr);
         SYSIO(LsetXattrAck);
         SYSIO(LgetXattr);
         SYSIO(LgetXattrAck);
         SYSIO(LremoveXattr);
         SYSIO(LremoveXattrAck);
         SYSIO(LlistXattr);
         SYSIO(LlistXattrAck);

         SYSIO(PsetXattr);
         SYSIO(PsetXattrAck);
         SYSIO(PgetXattr);
         SYSIO(PgetXattrAck);
         SYSIO(PremoveXattr);
         SYSIO(PremoveXattrAck);
         SYSIO(PlistXattr);
         SYSIO(PlistXattrAck);
         
         SYSIO(GpfsFcntl);
         SYSIO(GpfsFcntlAck);
         SYSIO(Ftruncate64);
         SYSIO(Ftruncate64Ack);
         SYSIO(Truncate64);
         SYSIO(Truncate64Ack);
 */
       

        default:   break;
    }
    if (text){  
       length = (size_t)snprintf(buffer,bufsize," %s",text);
       buffer += length;
       bufsize -= length;      
    }
   
    if (log->id ==BGV_SEND_MSG){
        length = (size_t)snprintf(buffer, bufsize, " lQP=%d ",log->ci.BGV_recv[1] );
        buffer += length;
        bufsize -= length;  
        bgcios::MessageHeader * mh2 = (bgcios::MessageHeader *)BGV_RECV_MSG_entry->data;
        if (mh->sequenceId == mh2->sequenceId){
           long long unsigned int cycles = 
               (long long unsigned int)(log->timeStamp - BGV_RECV_MSG_entry->timeStamp);
           length = (size_t)snprintf(buffer,bufsize," CYC=%llu",cycles);
           buffer += length;
           bufsize -= length; 
        }  
   }
   else if (log->id ==BGV_RECV_MSG){
        length = (size_t)snprintf(buffer, bufsize, " lQP=%d ",log->ci.BGV_recv[1] );
        buffer += length;
        bufsize -= length;   
        BGV_RECV_MSG_entry = (BG_FlightRecorderLog_t *)log; //save reference for BGV_SEND_MSG calcluation
   }
   else if (log->id ==BGV_BLOK_MSG){
        length = (size_t)snprintf(buffer, bufsize, " BLOCKED " );
        buffer += length;
        bufsize -= length;    
   }
   else if (log->id ==BGV_RLSE_MSG){
        length = (size_t)snprintf(buffer, bufsize, " UNBLOCKED " );
        buffer += length;
        bufsize -= length;    
   }
   else {
       // do nothing
   }

   if (doendl){
      length = (size_t)snprintf(buffer,bufsize," \n");
       buffer += length;
       bufsize -= length;
   }
  
     return (size_t)strlen(buffer_start);
}


void printLogMsg(const char * pathname){
    const size_t BUFSIZE = 1024;
    char buffer[BUFSIZE];
    size_t numChars = 0;
    log_fd = open(pathname,O_TRUNC | O_CREAT | O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO );
    if (log_fd == -1)log_fd = 2;
    //printf("fl_index=%d log_fd=%d\n",fl_index,log_fd);
    ssize_t rc=0;
    if (wrapped) {
       for (uint32_t i=fl_index;i<FlightLogSize;i++){
          //printMsg(i, (bgcios::MessageHeader *)FlightLog[i].data);
          numChars = Flight_CIOS_MsgDecoder(BUFSIZE, buffer, &FlightLog[i]);
          rc=write(log_fd,buffer,numChars);
          if (rc==0){
             printf("rc=%d errno=%d \n",(int)rc,errno);
             printf("numChars=%d %s",(int)numChars,buffer);
          }
       }
    }
    for (uint32_t i=0;i<fl_index;i++){
          //printMsg(FlightLog[i].id, (bgcios::MessageHeader *)FlightLog[i].data);
          numChars = Flight_CIOS_MsgDecoder(BUFSIZE, buffer, &FlightLog[i]);
          rc=write(log_fd,buffer,numChars);
          if (rc==0){
             printf("rc=%d errno=%d\n",(int)rc,errno);
             printf("numChars=%d %s",(int)numChars,buffer);
          }
    }
    if (log_fd != 2) close(log_fd);
}

void printLogEntry(uint32_t entry){
    const size_t BUFSIZE = 1024;
    char buffer[BUFSIZE];
    size_t numChars = 0;
    numChars = Flight_CIOS_MsgDecoder(BUFSIZE, buffer, &FlightLog[entry]);
    write(2,buffer,numChars);
}


size_t snprintfLogEntry(size_t bufsize, char* buffer,uint32_t entry ){
    size_t numChars = Flight_CIOS_MsgDecoder(bufsize, buffer, &FlightLog[entry],false);
    return numChars;
}

void printprevLogEntries(uint32_t entry,uint32_t quantity){
   if (quantity>FlightLogSize)quantity=FlightLogSize;
   if (entry>FlightLogSize) return;
   if (quantity==0) return;
   uint32_t spot = entry;
   if (entry >= quantity){
       spot=(entry+1)-quantity; 
   }
   else{
       spot = FlightLogSize - (quantity - (entry + 1));
   }
   for (uint32_t i = 0; i<quantity; i++){
      printLogEntry(spot);
      spot++;
      if (spot==FlightLogSize)spot=0;
   }
}

void printlastLogEntries(uint32_t quantity){
   uint32_t index = fl_index;
   if (index==0)index = FlightLogSize - 1;
   else index--;
   printprevLogEntries(index, quantity);
}

void * FlightLogDumpWaiter::run(void)
{ 
   int done = 0;
   const int pipeForSig   = 0;
   const int numFds       = 1;

   pollfd pollInfo[numFds];
   int timeout = -1; // 10000 == 10 sec
   bgcios::SigWritePipe SigWritePipe(SIGUSR1);

   pollInfo[pipeForSig].fd = SigWritePipe._pipe_descriptor[0];
   pollInfo[pipeForSig].events = POLLIN;
   pollInfo[pipeForSig].revents = 0;
   // Process events until told to stop.
   while (!done) {

      // Wait for an event on one of the descriptors.
      int rc = poll(pollInfo, numFds, timeout);

     // There was no data so try again.
      if (rc == 0) {
         continue;
      } 
            // Check for an event on the pipe for signal.
      if (pollInfo[pipeForSig].revents & POLLIN) {
         pollInfo[pipeForSig].revents = 0;
         siginfo_t siginfo;
         read(pollInfo[pipeForSig].fd,&siginfo,sizeof(siginfo_t));
         const size_t BUFSIZE = 1024;
         char buffer[BUFSIZE];
         const size_t HOSTSIZE = 256;
         char hostname[HOSTSIZE];
         hostname[0]=0;
         gethostname(hostname,HOSTSIZE);
         snprintf(buffer,BUFSIZE,"/var/spool/abrt/fl_%s.%d.%s.log",_daemon_name,getpid(),hostname);
         printf("Attempting to write flight log %s\n",buffer);
         printLogMsg(buffer); //print the log to stdout
      }
   }

   return NULL;
}
