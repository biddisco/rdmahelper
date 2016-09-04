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

//! \file  RdmaDevice.h 
//! \brief Declaration and inline methods for bgcios::RdmaDevice class.

#ifndef COMMON_RDMADEVICE_H
#define COMMON_RDMADEVICE_H

// Includes
#include <netinet/in.h>
#include <infiniband/verbs.h>
#include <ifaddrs.h>
#include <memory>
#include <string>

namespace bgcios
{

//! \brief InfiniBand device for RDMA operations.

class RdmaDevice
{
public:

   //! \brief  Default constructor.
   //! \param  device Name of InfiniBand device.
   //! \param  interface Name of network interface.
   //! \throws rdma_error

   RdmaDevice(std::string device, std::string interface);

   //! \brief  Default destructor.

   ~RdmaDevice();

   //! \brief  Get the InfiniBand device name.
   //! \return Device name string.

   std::string getDeviceName(void);

   //! \brief  Get InfiniBand device the global unique identifier.
   //! \return Global unique identifier.

   uint64_t getGuid(void);

   //! \brief  Get the network interface name.
   //! \return Network interface name string.

   std::string getInterfaceName(void);

   //! \brief  Get the IP address of the network interface.
   //! \return IP address for RDMA connection management.

   in_addr_t getAddress(void);

   uint64_t getDeviceInfo(bool verbose);

   struct ibv_context *getContext();

   static bool bgvrnic_device;

private:

   //! Pointer to list of InfiniBand devices.
   struct ibv_device **_deviceList;

   //! Pointer to selected device in the list.
   struct ibv_device *_myDevice;

   //! context, you hould not normally use this
   // as the rdma_cm lib handles it, but for
   // access to nvp, this is provided
   struct ibv_context *_context;

   //! Pointer to list of network interfaces.
   struct ifaddrs *_interfaceList;

   //! Pointer to selected interface in the list.
   struct ifaddrs *_myInterface;

};

//! Smart pointer for RdmaDevice object.
typedef std::shared_ptr<RdmaDevice> RdmaDevicePtr;

} // namespace bgcios

#endif // COMMON_RDMADEVICE_H

