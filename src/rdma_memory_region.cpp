//  Copyright (c) 2015-2016 John Biddiscombe
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <plugins/parcelport/verbs/rdmahelper/include/rdma_logging.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_memory_region.hpp>
//
#include <memory>

using namespace hpx::parcelset::policies::verbs;

// --------------------------------------------------------------------
int rdma_memory_region::allocate(bgcios::rdma_protection_domainPtr pd, size_t length)
{
    // Allocate storage for the memory region.
    void *buffer = malloc(length);
    if (buffer != NULL) {
        LOG_DEBUG_MSG("allocated storage for memory region with malloc OK "
            << hexnumber(length));
    }

    region_ = ibv_reg_mr(
        pd->getDomain(),
        buffer, length,
        IBV_ACCESS_LOCAL_WRITE |
        IBV_ACCESS_REMOTE_WRITE |
        IBV_ACCESS_REMOTE_READ);

    if (region_ == NULL) {
        LOG_ERROR_MSG("error registering ibv_reg_mr : "
            << " " << errno << " " << rdma_error::error_string(errno));
        return -1;
    }
    else {
        LOG_DEBUG_MSG("OK registering ibv_reg_mr with flags : "
            << " " << hexnumber(accessFlags));
    }

    LOG_DEBUG_MSG("allocated memory region " << hexpointer(this) << " with local key "
        << getLocalKey() << " at address " << getAddress() << " with length "
        << hexlength(getLength()));
    return 0;
}

// --------------------------------------------------------------------
int rdma_memory_region::allocate64kB(bgcios::rdma_protection_domainPtr pd)
{
    size_t length = 65536;
    // Allocate storage for the memory region.
    void *buffer = malloc(length);
    if (buffer != NULL) {
        LOG_DEBUG_MSG("allocated 64kB storage for memory region with malloc OK " << length);
    }

    region_ = ibv_reg_mr(
        pd->getDomain(),
        buffer,
        length,
        IBV_ACCESS_LOCAL_WRITE |
        IBV_ACCESS_REMOTE_WRITE |
        IBV_ACCESS_REMOTE_READ);

    if (region_ == NULL) {
        LOG_ERROR_MSG("error registering memory region");
        int err = errno;
        LOG_ERROR_MSG("ibv_reg_mr error message : " << rdma_error::error_string(err));
        free(buffer);
        return -1;
    }
    return 0;
}

// --------------------------------------------------------------------
int rdma_memory_region::release(void) {
    LOG_DEBUG_MSG("About to release memory region with local key " << getLocalKey());
    if (region_ != NULL) {
        void *buffer = getAddress();
        uint32_t length = getLength();
        if (ibv_dereg_mr (region_)) {
            LOG_ERROR_MSG("Error, ibv_dereg_mr() failed\n");
            return -1;
        }
        else {
            LOG_DEBUG_MSG("released memory region with local key " << getLocalKey() << " at address " << buffer << " with length " << hexlength(length));
        }
        if (!isUserRegion()) {
            free(buffer);
        }
        region_ = NULL;
    }

    return 0;
}

