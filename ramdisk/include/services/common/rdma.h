/*                                                                  */
/* Licensed Materials - Property of IBM                             */
/*                                                                  */
/* Blue Gene/Q                                                      */
/*                                                                  */
/* (c) Copyright IBM Corp. 2012, 2012 All Rights Reserved           */
/*                                                                  */
/* US Government Users Restricted Rights - Use, duplication or      */
/* disclosure restricted by GSA ADP Schedule Contract with IBM      */
/* Corporation.                                                     */
/*                                                                  */
/* This software is available to you under either the GNU General   */
/* Public License (GPL) version 2 or the Eclipse Public License     */
/* (EPL) at your discretion.                                        */
/*                                                                  */

#ifndef	_KERNEL_RDMA_H_ /* Prevent multiple inclusion */
#define	_KERNEL_RDMA_H_

#define USER_MAX_RDMA_REGIONS 4
/*! \file  RDMA.h
 * \brief Declaration of special programming interfaces.
 * \remarks Intended for an abbreviated version of RDMA CM from the 
 * compute node to its IO node
 * 
 * \par \b Server on IO Node for compute node
 * The compute node application programming permits connecting to a server program on the IO node serving the compute node for function-ship IO operations.  
 * The IO node follows the typical "Server Operation" noted in the Linux man page "rdma_cm(7): RDMA communications manager" which can be found by searching
 * the world-wide web.
 * The server data operations are limited to receiving and sending packets and RDMA read and write:
 * \li \c  ibv_post_recv()
 * \li \c  ibv_post_send() OPCODE IBV_WR_SEND
 * \li \c  ibv_post_send() OPCODE IBV_WR_RDMA_READ
 * \li \c  ibv_post_send() OPCODE IBV_WR_RDMA_WRITE
 * 
 * \par \b Client Operation on Compute Node
 * The compute node is restricted to one RDMA CM client application.  The client connects to its IO node by using 
 * Kernel_RDMAConnect() to the port of the server application on the IO node.
 * The client may register memory for RDMA operations but the RDMA operations are conducted on the IO node only.  
 * The client application is restricted to sending and receiving packets:
 * \li \c  ibv_post_recv()
 * \li \c  ibv_post_send() OPCODE IBV_WR_SEND
 * \par
 * The client can send the registered memory information to the IO node for RDMA operations done by the IO node server program.  
 * The minimum requirement is to send a virtual address and the memory key.  The length may be implicit or sent.
 * \par  
 * The ending of the client application will automatically clean up and close the connection.  
 * The client application may close the file descriptor, close(rdma_fd), like any other file descriptor.
 * The ending of the connection includes automatic deregistration of any registered memory and sending a disconnect to the server program.
*/

#include "kernel_impl.h" 
#include "spi/include/kernel/memory.h"
#include "hwi/include/common/compiler_support.h"



__BEGIN_DECLS


/*!
 * \brief Open a descriptor for purposes of talking to an OFED server on the IO node
 * \details The RDMA file descriptor is required for all operations.  
 *
 *
 * \retval 0 success
 * \retval >0 is the file descriptor
 * \retval -1 check errno:
 * \li \c EINVAL Invalid Argument
 * \li \c EEXIST RDMA file descriptor already exists
 * 
 * \note To disconnect and automatically cleanup, use close.  Also, application cleanup will close file descriptors, inclding this one
 */
__INLINE__
int Kernel_RDMAOpen(int* socket);

/*!
 * \brief Connect to the IO node servicing this compute node
 *
 * \param[in] The RDMA_fd is the file descriptor from RDMAOpen
 * \param[in] The destination_port is the listening port of a listening RDMA_CM process on the IO node for this compute node. 
 * 
 *
 * \retval 0 success
 * \retval errno
 * \li \c EINVAL Invalid Argument
 * \li \c EADDRNOTAVAIL Address not available (port)
 * \li \c EADDRINUSE Address already in use (port)
 * 
 * \note Private data is not supported
 * \note To disconnect, use close.  Application cleanup will close file descriptors, inclding this one
 * \note If the server is sending a data message immediately after accepting the connection (e.g., ibv_post_send opcode IBV_WR_SEND), 
 *  need to have a posted receive buffer in place before the connect is requested
 */

__INLINE__
int Kernel_RDMAConnect(int RDMA_fd, int destination_port);

/*!
 * \brief Send a packet to the remote end of the connection (nonblocking)
 *
 * \param[in] The RDMA_fd is the file descriptor from RDMAOpen
 * \param[in] The len is a nonzero value not more than max_packet_size from Kernel_RDMA_CONNECT.
 * \param[in] The buf pointer is the start of the packet to send 
 * \param[in] The lkey is the lkey from the Kernel_RDMARegion_t sent & returned on Kernel_RDMARegisterMem
 * \param[out] The requestID is returned so that the completion entry can be harvested on a poll
 *
 * \retval 0, success
 * \retval errno
 * \li \c EBADF Bad file descriptor
 * \li \c ENOTCONN Not connected
 * \li \c EINVAL Invalid Argument
 * 
 */

__INLINE__
int Kernel_RDMASend(int RDMA_fd, const void* buf, size_t len, uint32_t lkey);


/*!
 * \brief Post a receive for receiving data from the remote end of the connection (nonblocking)
 *
 * \param[in] The RDMA_fd is the file descriptor from Kernel_RDMA_CONNECT
 * \param[in] The len equals max_packet_size from Kernel_RDMA_CONNECT.
 * \param[in] The buf pointer points to the start of the memory region in RDMA_CONNECT plus a multiple of max_packet_size still within the memory region. 
 * \param[in] The lkey is the lkey from the Kernel_RDMARegion_t sent & returned on Kernel_RDMARegisterMem
 * \param[out] The requestID is returned so that the completion entry can be harvested on a poll
 *
 *
 * \retval 0, success
 * \retval errno
 * \li \c EBADF Bad file descriptor
 * \li \c ENOTCONN Not connected
 * \li \c EINVAL Invalid Argument
 * \li \c EMSGSIZE The len value is 0 or larger than the connection packet size
 * 
 */

__INLINE__
int Kernel_RDMARecv(int RDMA_fd, const void* buf, size_t len, uint32_t key);

/*!
 * \brief Check for completion queue entries (nonblocking)
 *
 * \param[in] The RDMA_fd is the file descriptor from Kernel_RDMA_CONNECT
 * \param[in,out] The num_entries is the number of RDMA_wc_t supplied on input, number of entries returned on output
 * \param[in] The WorkCompletionList points to the array of work completions available
 *
 * \retval 0, success
 * \retval errno:
 * \li \c EBADF Bad file descriptor
 * \li \c ENOTCONN Not connected
 * \li \c EINVAL Invalid Argument
 */
typedef
struct Kernel_RDMAWorkCompletion {
  void*    buf;    /* buffer address of data for completed operation */
  uint32_t len;    /* length of data for completed operation         */
  uint32_t opcode; /* opcode receive=1  send=2                       */
  uint32_t status; /* =0, successful, >0 errno                       */
  uint32_t flags;    /* reserved for future use                      */
  uint64_t reserved; /* reserved for future use                      */
} Kernel_RDMAWorkCompletion_t;

__INLINE__
int Kernel_RDMAPollCQ(int RDMA_fd, int* num_entries, Kernel_RDMAWorkCompletion_t* WorkCompletionList);


typedef 
struct Kernel_RDMARegion{
   void * address;                  /*> The start of the memory region    */
   size_t length;                   /*> The length of the memory region   */
   uint32_t lkey;                   /*> contains or will contain the key for the memory region for RDMA between IO node and compute node */             
} Kernel_RDMARegion_t;

/*!
 * \brief Register application memory for RDMA use by the IO node for the compute node
 *
 * \param[in] The RDMA_fd is the file descriptor from Kernel_RDMA_CONNECT
 * \param[in,out] The usingRegion4RDMA points to a structure with the created memory region and will have the physical address and key set on a succcessful return
 *
 * \retval 0, success
 * \retval errno:
 * \li \c EBADF Bad file descriptor
 * \li \c EINVAL Invalid Argument
 * \li \c EFAULT  Address is an invalid user space address.
 * \li \c ENOBUFS The kernel has no more space for registrations until deregistrations happen.
 * 
 * \note Application exit or close will remove memory registrations if deregister is not done
 * \note Maximum number of registered regions is USER_MAX_RDMA_REGIONS
 */
__INLINE__
int Kernel_RDMARegisterMem(int RDMA_fd,Kernel_RDMARegion_t * usingRegion4RDMA);

/*!
 * \brief Deregister application memory for RDMA
 *
 * \param[in] The RDMA_fd is the file descriptor from Kernel_RDMA_CONNECT
 * \param[in] The usingRegion4RDMA points to a structure of a previously registered memory region
 *
 * \retval 0, success
 * \retval errno:
 * \li \c EBADF Bad file descriptor
 * \li \c EINVAL Invalid Argument
 * \li \c EFAULT  Address is an invalid user space address.
 */
__INLINE__
int Kernel_RDMADeregisterMem(int RDMA_fd, Kernel_RDMARegion_t *  usingRegion4RDMA);

#include "rdma_impl.h"

__END_DECLS

#endif /* _KERNEL_RDMA_H_ */
