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

//! \file  ServicesConstants.h
//! \brief Constants and other helpful things for services.

#ifndef SERVICES_CONSTANTS_H
#define SERVICES_CONSTANTS_H

// Includes
#include <inttypes.h>
#include <string.h>
#include <string>

namespace bgcios
{

//! Special value for simulation id to indicate simulation is not active.
const uint16_t SimulationDisabled = 0;

//! Special value for simulation id to indicate I/O node simulation is active.
const uint16_t SimulationWithION = 1;

//! Work directory for services objects created at runtime.
const std::string WorkDirectory = "/var/cios/";

//! Work directory for services when simulation is active.
const std::string SimulationDirectory = "/tmp/cios";

//! Jobs directory for storing info about running jobs.
const std::string JobsDirectory = "/jobs/";

//! Tool control subdirectory under jobs directory for finding data channel by rank.
const std::string ToolctlRankDirectory = "/toolctl_rank/";

//! Tool control subdirectory under jobs directory for finding data channel by node.
const std::string ToolctlNodeDirectory = "/toolctl_node/";

//! Tools subdirectory under jobs directory for finding active tools.
const std::string ToolsDirectory = "/tools/";

//! Tools status subdirectory under jobs directory for checking active tool status.
const std::string ToolsStatusDirectory = "/tools_status/";

//! Name of services control command channel object.
const std::string IosctlCommandChannelName = "ioscontrol";

//! Name of job control command channel object.
const std::string JobctlCommandChannelName = "jobcontrol";

//! Name of standard i/o command channel object.
const std::string StdioCommandChannelName = "standardio";

//! Name of system i/o command channel object.
const std::string SysioCommandChannelName = "systemio";

//! Name of tool control command channel object.
const std::string ToolctlCommandChannelName = "toolcontrol";

//! Name of tool control data channel object.
const std::string ToolctlDataChannelName = "toolcontrol.data";

//! Default size of large message memory regions.
const uint32_t DefaultLargeRegionSize = 1048576;

//! Minimum size of large message memory regions.
const uint32_t MinLargeRegionSize = 65536;

//! Maximum size of large message memory regions.
const uint32_t MaxLargeRegionSize = 16777216;

//! Default number of large message memory regions.
const uint16_t DefaultNumLargeRegions = 16;

//! Default unique id for simulation mode.
const uint16_t DefaultSimulationId = 0;

//! Number of microseconds in a second.
const uint64_t MicrosecondsPerSecond = 1000000;

//! Maximum number of ranks per compute node.
const uint16_t MaxRanksPerNode = 64;

//! \brief  Return a string describing an errno value.
//! \param  error Errno value.
//! \return Pointer to string describing errno value.

inline char *errorString(int error)
{
   char errorBuffer[100];
   return strerror_r(error, errorBuffer, sizeof(errorBuffer));
}

//! \brief  Check if simulation mode is enabled.
//! \return True if simulation mode is enabled, false if running on hardware.

inline bool isSimulation(uint16_t simId) { return simId != bgcios::SimulationDisabled ? true : false; }

//! \brief  Check if running on hardware.
//! \return True if running on hardware, false if simulation mode is enabled.

inline bool isHardware(uint16_t simId) { return simId == bgcios::SimulationDisabled ? true : false; }

//! \brief  Check if I/O node simulation is enabled.
//! \return True if I/O node simulation is enabled, otherwise false.

inline bool isIONSimulation(uint16_t simId) { return simId == bgcios::SimulationWithION ? true : false; }

} // namespace bgcios

#endif // SERVICES_CONSTANTS_H

