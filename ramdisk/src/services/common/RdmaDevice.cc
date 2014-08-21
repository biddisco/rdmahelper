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

//! \file  RdmaDevice.cc 
//! \brief Methods for bgcios::RdmaDevice class.

// Includes
#include <ramdisk/include/services/common/RdmaDevice.h>
#include <ramdisk/include/services/common/RdmaError.h>
#include <ramdisk/include/services/ServicesConstants.h>
#include <ramdisk/include/services/common/logging.h>
#include <errno.h>
#include <sstream>

using namespace bgcios;

LOG_DECLARE_FILE("cios.common");


RdmaDevice::RdmaDevice(std::string device, std::string interface)
{
   // Get the list of InfiniBand devices.
   int numDevices = 0;
   _deviceList = ibv_get_device_list(&numDevices);
   if (_deviceList == NULL) {
      LOG_ERROR_MSG("there are no InfiniBand devices available on the node");
      RdmaError e(ENODEV, "no InfiniBand devices available");
      throw e;
   }

   // Search for the specified InfiniBand device.
   _myDevice = NULL;
   for (int index = 0; index < numDevices; ++index) {
      if (strcmp(ibv_get_device_name(_deviceList[index]), device.c_str()) == 0) {
         _myDevice = _deviceList[index];
         break;
      }
   }

   // See if the specifed InfiniBand device was found.
   if (_myDevice == NULL) {
      LOG_ERROR_MSG("InfiniBand device " << device << " was not found");
      std::ostringstream what;
      what << "InfiniBand device " << device << " not found";
      RdmaError e(ENODEV, what.str());
      throw e;
   }
   LOG_CIOS_DEBUG_MSG("found InfiniBand device " << getDeviceName());

   // Get the list of network interfaces.
   if (getifaddrs(&_interfaceList) != 0) {
      RdmaError e(errno, "getifaddrs() failed");
      LOG_ERROR_MSG("error getting list of network interfaces: " << bgcios::errorString(e.errcode()));
      throw e;
   }

   // Search for the specified network interface.
   for (_myInterface = _interfaceList; _myInterface != NULL; _myInterface = _myInterface->ifa_next) {
      if ((strcmp(_myInterface->ifa_name, interface.c_str()) == 0) && (_myInterface->ifa_addr->sa_family == AF_INET)) {
         break;
      }
   }

   // See if the specified network interface was found.
   if (_myInterface == NULL) {
      LOG_ERROR_MSG("network interface " << interface << " was not found on the node");
      std::ostringstream what;
      what << "network interface " << interface << " not found";
      RdmaError e(ENOENT, what.str());
      throw e;
   }
   LOG_CIOS_DEBUG_MSG("found network interface " << getInterfaceName());
}

RdmaDevice::~RdmaDevice()
{
   // Free the list of InfiniBand devices.
   ibv_free_device_list(_deviceList);

   // Free the list of network interfaces.
   freeifaddrs(_interfaceList);
}

std::string
RdmaDevice::getDeviceName(void)
{
   std::string name;
   if (_myDevice != NULL) {
      const char *namep = ibv_get_device_name(_myDevice);
      if (namep != NULL) {
         name = namep;
      }
   }

   return name;
}

uint64_t
RdmaDevice::getGuid(void)
{
   uint64_t guid = 0;
   if (_myDevice != NULL) {
      guid = ibv_get_device_guid(_myDevice);
   }

   return guid;
}

std::string
RdmaDevice::getInterfaceName(void)
{
   std::string name;
   if (_myInterface != NULL) {
      name = _myInterface->ifa_name;
   }

   return name;
}

in_addr_t
RdmaDevice::getAddress(void)
{
   // Just return if the interface was not found.
   if (_myInterface == NULL) {
      return 0;
   }

   sockaddr_in *addr = (sockaddr_in *)_myInterface->ifa_addr;
   return addr->sin_addr.s_addr;
}


