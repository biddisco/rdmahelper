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
#include "rdmahelper_logging.h"
#include <errno.h>
#include <iostream>

using namespace bgcios;

LOG_DECLARE_FILE("cios.common");

bool RdmaDevice::bgvrnic_device = false;

//----------------------------------------------------------------------------
RdmaDevice::RdmaDevice(std::string device, std::string interface)
{
  _context = NULL;

   // Get the list of InfiniBand devices.
   int numDevices = 0;
   _deviceList = ibv_get_device_list(&numDevices);
   if (_deviceList == NULL) {
    //      std::cout << "there are no InfiniBand devices available on the node" << std::endl;
      LOG_ERROR_MSG("there are no InfiniBand devices available on the node");
      RdmaError e(ENODEV, "no InfiniBand devices available");
      throw e;
   }

   // Search for the specified InfiniBand device.
   _myDevice = NULL;
   for (int index = 0; index < numDevices; ++index) {
    LOG_DEBUG_MSG("checking device " << ibv_get_device_name(_deviceList[index]) << " against " << device.c_str());
      if (strcmp(ibv_get_device_name(_deviceList[index]), device.c_str()) == 0) {
         _myDevice = _deviceList[index];
      //
      // on BGQ IO nodes the infiniband device bgvrnic has special properties
      // we set a flag which is used by the memory allocation code to take
      // special action
      //
      if (strstr(device.c_str(), "bgvrnic")) {
        RdmaDevice::bgvrnic_device = true;
      }
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

  _myInterface   = NULL;
  _interfaceList = NULL;

  if (!interface.empty()) {
   // Get the list of network interfaces.
   if (getifaddrs(&_interfaceList) != 0) {
      RdmaError e(errno, "getifaddrs() failed");
      LOG_ERROR_MSG("error getting list of network interfaces: " << bgcios::errorString(e.errcode()));
      throw e;
   }

   // Search for the specified network interface.
   for (_myInterface = _interfaceList; _myInterface != NULL; _myInterface = _myInterface->ifa_next) {
      //      std::cout << "checking interface " << _myInterface->ifa_name << " against " << interface.c_str() << std::endl;
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

}

//----------------------------------------------------------------------------
RdmaDevice::~RdmaDevice()
{
   // Free the list of InfiniBand devices.
   ibv_free_device_list(_deviceList);

  if (_interfaceList) {
   // Free the list of network interfaces.
   freeifaddrs(_interfaceList);
}

  if (_context) {
    if (ibv_close_device(_context)!=0) {
      LOG_ERROR_MSG("Failed to close device");
    }
  }
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
uint64_t
RdmaDevice::getGuid(void)
{
   uint64_t guid = 0;
   if (_myDevice != NULL) {
      guid = ibv_get_device_guid(_myDevice);
   }

   return guid;
}

//----------------------------------------------------------------------------
std::string
RdmaDevice::getInterfaceName(void)
{
   std::string name;
   if (_myInterface != NULL) {
      name = _myInterface->ifa_name;
   }

   return name;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
static const char *guid_str(uint64_t node_guid, char *str)
{
//  node_guid = ntohll(node_guid);
  sprintf(str, "%04x:%04x:%04x:%04x",
    (unsigned) (node_guid >> 48) & 0xffff,
    (unsigned) (node_guid >> 32) & 0xffff,
    (unsigned) (node_guid >> 16) & 0xffff,
    (unsigned) (node_guid >>  0) & 0xffff);
  return str;
}

//----------------------------------------------------------------------------
ibv_context *RdmaDevice::getContext()
{
  // Just return if the interface was not found.
  if (_myDevice == NULL) {
    return NULL;
  }

  if (_context==NULL) {
    struct ibv_context *ctx = ibv_open_device(_myDevice);
    if (!ctx) {
      LOG_ERROR_MSG("Failed to open device");
      return 0;
    }
    LOG_DEBUG_MSG("Created context " << ctx);
    _context = ctx;
  }
  return _context;
}

//----------------------------------------------------------------------------
uint64_t
RdmaDevice::getDeviceInfo(bool verbose)
{
  struct ibv_device_attr device_attr;
  int rc = 0;
  uint8_t port;
  char buf[256];

  ibv_context *ctx = this->getContext();

  if (ibv_query_device(ctx, &device_attr)) {
    LOG_ERROR_MSG("ibv_query_device failed");
  }
  else {
    printf("hca_id:\t%s\n", ibv_get_device_name(_myDevice));
    if (strlen(device_attr.fw_ver))
      printf("\tfw_ver:\t\t\t\t%s\n", device_attr.fw_ver);
    printf("\tnode_guid:\t\t\t%s\n", guid_str(device_attr.node_guid, buf));
    printf("\tsys_image_guid:\t\t\t%s\n", guid_str(device_attr.sys_image_guid, buf));
    printf("\tvendor_id:\t\t\t0x%04x\n", device_attr.vendor_id);
    printf("\tvendor_part_id:\t\t\t%d\n", device_attr.vendor_part_id);
    printf("\thw_ver:\t\t\t\t0x%X\n", device_attr.hw_ver);

    //if (ibv_read_sysfs_file(_myDevice->ibdev_path, "board_id", buf, sizeof buf) > 0)
    //  printf("\tboard_id:\t\t\t%s\n", buf);

    printf("\tphys_port_cnt:\t\t\t%d\n", device_attr.phys_port_cnt);

    if (verbose) {
      printf("\tmax_mr_size:\t\t\t0x%llx\n",
          (unsigned long long) device_attr.max_mr_size);
      printf("\tpage_size_cap:\t\t\t0x%llx\n",
          (unsigned long long) device_attr.page_size_cap);
      printf("\tmax_qp:\t\t\t\t%d\n", device_attr.max_qp);
      printf("\tmax_qp_wr:\t\t\t%d\n", device_attr.max_qp_wr);
      printf("\tdevice_cap_flags:\t\t0x%08x\n", device_attr.device_cap_flags);
      printf("\tmax_sge:\t\t\t%d\n", device_attr.max_sge);
      printf("\tmax_sge_rd:\t\t\t%d\n", device_attr.max_sge_rd);
      printf("\tmax_cq:\t\t\t\t%d\n", device_attr.max_cq);
      printf("\tmax_cqe:\t\t\t%d\n", device_attr.max_cqe);
      printf("\tmax_mr:\t\t\t\t%d\n", device_attr.max_mr);
      printf("\tmax_pd:\t\t\t\t%d\n", device_attr.max_pd);
      printf("\tmax_qp_rd_atom:\t\t\t%d\n", device_attr.max_qp_rd_atom);
      printf("\tmax_ee_rd_atom:\t\t\t%d\n", device_attr.max_ee_rd_atom);
      printf("\tmax_res_rd_atom:\t\t%d\n", device_attr.max_res_rd_atom);
      printf("\tmax_qp_init_rd_atom:\t\t%d\n", device_attr.max_qp_init_rd_atom);
      printf("\tmax_ee_init_rd_atom:\t\t%d\n", device_attr.max_ee_init_rd_atom);
 //     printf("\tatomic_cap:\t\t\t%s (%d)\n",
 //         atomic_cap_str(device_attr.atomic_cap), device_attr.atomic_cap);
      printf("\tmax_ee:\t\t\t\t%d\n", device_attr.max_ee);
      printf("\tmax_rdd:\t\t\t%d\n", device_attr.max_rdd);
      printf("\tmax_mw:\t\t\t\t%d\n", device_attr.max_mw);
      printf("\tmax_raw_ipv6_qp:\t\t%d\n", device_attr.max_raw_ipv6_qp);
      printf("\tmax_raw_ethy_qp:\t\t%d\n", device_attr.max_raw_ethy_qp);
      printf("\tmax_mcast_grp:\t\t\t%d\n", device_attr.max_mcast_grp);
      printf("\tmax_mcast_qp_attach:\t\t%d\n", device_attr.max_mcast_qp_attach);
      printf("\tmax_total_mcast_qp_attach:\t%d\n",
          device_attr.max_total_mcast_qp_attach);
      printf("\tmax_ah:\t\t\t\t%d\n", device_attr.max_ah);
      printf("\tmax_fmr:\t\t\t%d\n", device_attr.max_fmr);
      if (device_attr.max_fmr) {
        printf("\tmax_map_per_fmr:\t\t%d\n", device_attr.max_map_per_fmr);
      }
      printf("\tmax_srq:\t\t\t%d\n", device_attr.max_srq);
      if (device_attr.max_srq) {
        printf("\tmax_srq_wr:\t\t\t%d\n", device_attr.max_srq_wr);
        printf("\tmax_srq_sge:\t\t\t%d\n", device_attr.max_srq_sge);
      }
      printf("\tmax_pkeys:\t\t\t%d\n", device_attr.max_pkeys);
      printf("\tlocal_ca_ack_delay:\t\t%d\n", device_attr.local_ca_ack_delay);
    }


  }
  return 1;
}
//----------------------------------------------------------------------------

