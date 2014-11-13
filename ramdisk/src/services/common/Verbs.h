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

/*
 * Copyright (c) 2011 Open Fabrics Alliance, Inc. All rights reserved.
 *
 * This software is licensed under the OpenFabrics BSD license below:
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer.
 *
 * - Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials
 *   provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef CNVERBS_H
#define CNVERBS_H

#include <inttypes.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Opaque internal device context.
struct cnv_context;

//! Protection domain.
struct cnv_pd
{
   struct cnv_context  *context;       //!< Device context.
   uint32_t             handle;        //!< Internal handle to protection domain.
};

//! Access flag values for cnv_reg_mr().
enum cnv_access_flags {
   CNV_ACCESS_LOCAL_WRITE  = 1,        //!< Enable local write access.
   CNV_ACCESS_REMOTE_WRITE = (1<<1),   //!< Enable remote write access.
   CNV_ACCESS_REMOTE_READ  = (1<<2)    //!< Enable remote read access.
};

//! Memory region.
struct cnv_mr
{
   struct cnv_context  *context;       //!< Device context.
   struct cnv_pd       *pd;            //!< Protection domain.
   void                *addr;          //!< Start address of memory region.
   uint64_t             length;        //!< Length of memory region.
   uint32_t             handle;        //!< Internal handle to memory region.
   uint32_t             lkey;          //!< Local key to memory region.
   uint32_t             rkey;          //!< Remote key to memory region.
};

//! Completion queue.
struct cnv_cq
{
   struct cnv_context  *context;       //!< Device context.
   uint32_t             handle;        //!< Internal handle to completion queue.
   int                  cqe;           //!< Number of completion queue entries.
   // Synchronization?
};

//! Attributes for initializing a queue pair.
struct cnv_qp_init_attr {
// void          *qp_context;
   struct cnv_cq       *send_cq;       //!< Completion queue associated with send queue.
   struct cnv_cq       *recv_cq;       //!< Completion queue associated with receive queue.
// struct cnv_qp_cap  cap;
};

//! Current state of queue pair.
enum cnv_qp_state {
   CNV_QPS_RESET,                      //!< Reset.
   CNV_QPS_INIT,                       //!< Initialized.
   CNV_QPS_RTR,                        //!< Ready to receive.
   CNV_QPS_RTS,                        //!< Ready to send.
   CNV_QPS_SQD,                        //!< Send queue drain.
   CNV_QPS_SQE,                        //!< Send queue error.
   CNV_QPS_ERR                         //!< Error.
};

//! Queue pair.
struct cnv_qp
{
   struct cnv_context  *context;       //!< Device context.
   struct cnv_pd       *pd;            //!< Protection domain.
   struct cnv_cq       *send_cq;       //!< Completion queue for send operations.
   struct cnv_cq       *recv_cq;       //!< Completion queue for recv operations.
   uint32_t             handle;        //!< Internal handle to queue pair.
   uint32_t             qp_num;        //!< Queue pair number.
   enum cnv_qp_state    state;         //!< Current state of queue pair.
   // Synchronization?
};

//! Scatter/gather element.
struct cnv_sge
{
   uint64_t             addr;          //! Start address of memory.
   uint32_t             length;        //! Length of memory.
   uint32_t             lkey;          //! Local key of memory.
};

//! Work request operation codes.
enum cnv_wr_opcode {
   CNV_WR_RDMA_WRITE,                  //!< Put data to an address on remote node.
   CNV_WR_SEND,                        //!< Send data to a remote node.
   CNV_WR_SEND_WITH_IMM,               //!< Send data with an immediate value to a remote node.
   CNV_WR_RDMA_READ,                   //!< Get data from an address on a remote node.
};

//! Send work request flags.
enum cnv_send_flags {
   CNV_SEND_SOLICTED = 1,              //!< Set solicited event indicator.
};

//! Send work request.
struct cnv_send_wr
{
   uint64_t             wr_id;         //!< User defined work request id.
   struct cnv_send_wr  *next;          //!< Pointer to next work request, NULL if last work request.
   struct cnv_sge      *sg_list;       //!< List of scatter/gather elements.
   int                  num_sge;       //!< Number of scatter/gather elements in list.
   enum cnv_wr_opcode   opcode;        //!< Operation type.
   int                  send_flags;    //!< Flags (currently unused).
   uint32_t             imm_data;      //!< Immediate data value.
   uint64_t             remote_addr;   //!< Start address of remote memory region.
   uint32_t             rkey;          //!< Key of remote memory region.
};

//! Receive work request.
struct cnv_recv_wr
{
   uint64_t             wr_id;         //!< User defined work request id.
   struct cnv_recv_wr  *next;          //!< Pointer to next work request, NULL if last work request.
   struct cnv_sge      *sg_list;       //!< List of scatter/gather elements.
   int                  num_sge;       //!< Number of scatter/gather elements in list.
};

//! Status of completed work request.
enum cnv_wc_status {
   CNV_WC_SUCCESS,
   CNV_WC_LOC_LEN_ERR,
   CNV_WC_LOC_QP_OP_ERR,
   CNV_WC_LOC_EEC_OP_ERR,
   CNV_WC_LOC_PROT_ERR,
   CNV_WC_WR_FLUSH_ERR,
   CNV_WC_MW_BIND_ERR,
   CNV_WC_BAD_RESP_ERR,
   CNV_WC_LOC_ACCESS_ERR,
   CNV_WC_REM_INV_REQ_ERR,
   CNV_WC_REM_ACCESS_ERR,
   CNV_WC_REM_OP_ERR,
   CNV_WC_RETRY_EXC_ERR,
   CNV_WC_RNR_RETRY_EXC_ERR,
   CNV_WC_LOC_RDD_VIOL_ERR,
   CNV_WC_REM_INV_RD_REQ_ERR,
   CNV_WC_REM_ABORT_ERR,
   CNV_WC_INV_EECN_ERR,
   CNV_WC_INV_EEC_STATE_ERR,
   CNV_WC_FATAL_ERR,
   CNV_WC_RESP_TIMEOUT_ERR,
   CNV_WC_GENERAL_ERR
};

//! Operation type of completed work request.
enum cnv_wc_opcode {
   CNV_WC_SEND,                        //!< Data sent to a remote node.
   CNV_WC_RDMA_WRITE,                  //!< Data put to an address on a remote node.
   CNV_WC_RDMA_READ,                   //!< Data gotten from an address on a remote node.
   CNV_WC_RECV        = 1 << 7,        //!< Data received from a remote node (value set so can test using (opcode & CNV_WC_RECV).
};

//! Flags for the completed work request.
enum cnv_wc_flags {
   CNV_WC_FLAGS_IMM = 1,               //!< Immediate data value is valid.
};

//! Work completion.
struct cnv_wc
{
   uint64_t             wr_id;         //!< User defined work request id.
   enum cnv_wc_status   status;        //!< Status of completed work request.
   enum cnv_wc_opcode   opcode;        //!< Operation type of completed work request.
   uint32_t             vendor_err;    //!< Vendor error syndrome value.
   uint32_t             byte_len;      //!< Number of bytes transferred.
   uint32_t             imm_data;      //!< Immediate data value.
   uint32_t             qp_num;        //!< Local queue pair number of completed work request.
   uint32_t             src_qp;        //!< Source (remote) queue pair number of completed work request.
   enum cnv_wc_flags    wc_flags;      //!< Flags for the completed work request.
};

//! Special value for immediate data to direct a receive to a hardware thread.
//! When this flag is set in immediate data received from a remote node, the immediate data is interpreted as a
//! processor id and the receive is directed to the specified hardware thread that posted a receive work request.

#define CNV_DIRECTED_RECV (0x80000000)

//! Special value to .

#define CNV_ANY_PROC_ID (0xffffffff)

//! special value to characterize completion queue operation synchronization on sequence ID
//! Requires MessageHeader.h and tracking into data for sequence field for immediate packets
//! and requires sequence field be in immediate data for remote packets.
#define CNVERBS_SEQUENCEID_CQ_CHAR 1
#define CNVERBS_DEFAULT_CQ_CHAR    0

//! \brief  Initialize verbs module.
//!         When successful, the context structure is initialized for use by other verbs.
//! \param  context Pointer to context structure pointer.
//! \return 0 when successful, errno when unsuccessful.

int cnv_open_dev(struct cnv_context **context);

//! \brief  Allocate a protection domain.
//!         When successful, the protection domain structure is initialized for use by other verbs.
//! \param  pd Pointer to protection domain structure.
//! \param  context Pointer to context structure.
//! \return 0 when successful, errno when unsuccessful.

int cnv_alloc_pd(struct cnv_pd *pd, struct cnv_context *context);

//! \brief  Deallocate a protection domain.
//!         When successful, the protection domain structure is no longer usable by other verbs.
//! \param  pd Pointer to protection domain structure.
//! \return 0 when successful, errno when unsuccessful.

int cnv_dealloc_pd(struct cnv_pd *pd);

//! \brief  Register a memory region.
//!         When successful, the memory region structure is initialized for use by other verbs.
//! \param  mr Pointer to memory region structure.
//! \param  pd Pointer to protection domain structure.
//! \param  addr Pointer to start of memory region.
//! \param  length Length of memory region.
//! \param  flags Flags describing access to memory region.
//! \return 0 when successful, errno when unsuccessful.

int cnv_reg_mr(struct cnv_mr *mr, struct cnv_pd *pd, void *addr, size_t length, enum cnv_access_flags flags);

//! \brief  Deregister a memory region.
//!         When successful, the memory region structure is no long usable by other verbs.
//! \param  mr Pointer to memory region structure.
//! \return 0 when successful, errno when unsuccessful.

int cnv_dereg_mr(struct cnv_mr *mr);

//! \brief  Create a completion queue.
//!         When successful, the completion queue structure is initialized for use by other verbs.
//! \param  cq Pointer to completion queue structure.
//! \param  context Pointer to context structure.
//! \param  cqe Number of completion queue entries allowed on completion queue.
//! \param  characterization Sets the characteristic use of the cq
//! \return 0 when successful, errno when unsuccessful.

int cnv_create_cq(struct cnv_cq *cq, struct cnv_context *context, int cqe);

//! \brief  Modify completion queue operation charcteristics
//! \param  cq Pointer to completion queue structure.
//! \param  characteristic value setting
//! \return 0 when successful, errno when unsuccessful.

int cnv_modify_cq_character(struct cnv_cq *cq, uint32_t character);

//! \brief  Destroy a completion queue.
//!         When successful, the completion queue structure is no longer usable by other verbs.
//! \param  cq Pointer to completion queue structure.
//! \return 0 when successful, errno when unsuccessful.

int cnv_destroy_cq(struct cnv_cq *cq);

//! \brief  Create a queue pair.
//!         When successful, the queue pair structure is initialized for use by other verbs.
//! \param  qp Pointer to queue pair structure.
//! \param  pd Pointer to protection domain structure.
//! \param  attr Pointer to attributes structure for initializing queue pair.
//! \return 0 when successful, errno when unsuccessful.

int cnv_create_qp(struct cnv_qp *qp, struct cnv_pd *pd, struct cnv_qp_init_attr *attr);

//! \brief  Destroy a queue pair.
//!         When successful, the queue pair structure is no long usable by other verbs.
//! \param  qp Pointer to queue pair structure.
//! \return 0 when successful, errno when unsuccessful.

int cnv_destroy_qp(struct cnv_qp *qp);

//! \brief  Establish a connection to a remote queue pair.
//! \param  qp Pointer to local queue pair structure.
//! \param  remote_addr Address of remote queue pair.
//! \return 0 when successful, errno when unsuccessful.

int cnv_connect(struct cnv_qp *qp, struct sockaddr *remote_addr);

//! \brief  Disconnect from a remote queue pair.
//! \param  qp Pointer to local queue pair structure.
//! \return 0 when successful, errno when unsuccessful.

int cnv_disconnect(struct cnv_qp *qp);

//! \brief  Post a work request to a send queue.
//! \param  qp Pointer to queue pair structure.
//! \param  wr Pointer to send work request structure.
//! \param  bad_wr Pointer to first work structure in error.
//! \return 0 when successful, errno when unsuccessful.

int cnv_post_send(struct cnv_qp *qp, struct cnv_send_wr *wr, struct cnv_send_wr **bad_wr);

//! \brief  Post a work request to a send queue with a sequence ID
//! \param  qp Pointer to queue pair structure.
//! \param  wr Pointer to send work request structure.
//! \param  bad_wr Pointer to first work structure in error.
//! \param  An associated sequence ID relating to the message sequence ID
//! \return 0 when successful, errno when unsuccessful.

int cnv_post_send_seqID(struct cnv_qp *qp, struct cnv_send_wr *wr_list, struct cnv_send_wr **bad_wr,uint32_t seqID);

//! \brief  Post a list of work requests to a receive queue.
//! \param  qp Pointer to queue pair structure.
//! \param  wr Pointer to receive work request structure.
//! \param  bad_wr Pointer to first work structure in error.
//! \return 0 when successful, errno when unsuccessful.

int cnv_post_recv(struct cnv_qp *qp, struct cnv_recv_wr *wr, struct cnv_recv_wr **bad_wr);
int cnv_post_recv_proc(struct cnv_qp *qp, struct cnv_recv_wr *wr, struct cnv_recv_wr **bad_wr, uint32_t proc_id);
int cnv_post_recv_seqID(struct cnv_qp *qp, struct cnv_recv_wr *wr_list, struct cnv_recv_wr **bad_wr, uint32_t seq_ID);


//! \brief  Poll a completion queue.
//! \param  cq Pointer to completion queue structure.
//! \param  num_entries Number of entries in work completion structure array.
//! \param  wc Pointer to work completion structure array.
//! \param  num_returned Pointer to number of entries returned in work completion structure array.
//! \return 0 when successful, errno when unsuccessful.

int cnv_poll_cq(struct cnv_cq *cq, int num_entries, struct cnv_wc *wc, int *num_returned, uint32_t proc_id);

int cnverbs_depost_by_id(struct cnv_qp *qp, uint32_t proc_id);

#ifdef __cplusplus
}
#endif

#endif // CNVERBS_H

