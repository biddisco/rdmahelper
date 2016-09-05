//  Copyright (c) 2015-2016 John Biddiscombe
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_PARCELSET_POLICIES_VERBS_MEMORY_REGION_HPP
#define HPX_PARCELSET_POLICIES_VERBS_MEMORY_REGION_HPP

// Includes
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_logging.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_error.hpp>
#include <plugins/parcelport/verbs/rdmahelper/include/rdma_protection_domain.hpp>
#include <infiniband/verbs.h>
#include <string.h>
#include <errno.h>
#include <memory>

namespace hpx {
namespace parcelset {
namespace policies {
namespace verbs {

    class rdma_memory_region
    {
    public:

        rdma_memory_region()
        {
            region_     = NULL;
            used_space_ = 0;
            flags_      = 0;
        }

        rdma_memory_region(struct ibv_mr *region, uint32_t messageLength)
        {
            region_     = region;
            used_space_ = messageLength;
            flags_      = 0;
        }

        // create a memory region object by registering an existing address buffer
        rdma_memory_region(bgcios::rdma_protection_domainPtr pd,
            const void *buffer, const uint64_t length)
        {
            used_space_ = length;
            flags_      = BLOCK_USER;

            region_ = ibv_reg_mr(
                pd->getDomain(),
                const_cast<void*>(buffer), used_space_,
                IBV_ACCESS_LOCAL_WRITE |
                IBV_ACCESS_REMOTE_WRITE |
                IBV_ACCESS_REMOTE_READ);

            if (region_ == NULL) {
                int err = errno;
                LOG_ERROR_MSG(
                    "error registering user mem ibv_reg_mr " << hexpointer(buffer) << " "
                    << hexlength(length) << " error/message: " << err << "/"
                    << rdma_error::error_string(err));
            }
            else {
                LOG_DEBUG_MSG(
                    "OK registering memory =" << hexpointer(buffer) << " : "
                    << hexpointer(_region->addr) << " length " << hexlength(length));
            }

        }

        ~rdma_memory_region()
        {
            release();
        }

        // overload operator->
        inline unsigned char* operator->() const
        {
            return region_ != NULL ? (unsigned char*) (region_->addr) : NULL;
        }

        // overload operator*
        inline unsigned char* operator*() const
        {
            return region_ != NULL ? (unsigned char*) (region_->addr) : NULL;
        }

        //! \brief  Check if memory region has been allocated.
        //! \return True if memory region is allocated, otherwise false.

        inline bool isAllocated(void) const {
            return region_ != NULL ? true : false;
        }

        //! \brief  Allocate and register a memory region.
        //! \param  pd Protection domain for memory region.
        //! \param  length Length of the memory region.
        //! \return 0 when successful, errno when unsuccessful.

        int allocate(bgcios::rdma_protection_domainPtr pd, size_t length);

        //! \brief  Allocate and register a memory region from the bgvrnic device
        //! \param  pd Protection domain for memory region.
        //! \param  length Length of the memory region.
        //! \return 0 when successful, errno when unsuccessful.

        //! \brief  Allocate and register a 64KB memory region.
        //! \param  pd Protection domain for memory region.
        //! \return 0 when successful, errno when unsuccessful.
        int allocate64kB(bgcios::rdma_protection_domainPtr pd);

        //! \brief  Deregister and free the memory region.
        //! \return 0 when successful, errno when unsuccessful.

        int release(void);

        //! \brief  Get the address of the memory region.
        //! \return Address value.

        inline void *getAddress(void) const {
            return region_->addr;
        }

        //! \brief  Get the length of the memory region.
        //! \return Length value.

        inline uint32_t getLength(void) const {
            return (uint32_t) region_->length;
        }

        //! \brief  Get the local key of the memory region.
        //! \return Local key value.

        inline uint32_t getLocalKey(void) const {
            return region_->lkey;
        }

        //! \brief  Get the remote key of the memory region.
        //! \return Remote key value.

        inline uint32_t getRemoteKey(void) const {
            return region_->rkey;
        }

        //! \brief  Set the length of a message in the memory region.
        //! \param  length New message length value.
        //! \return Nothing.

        inline void setMessageLength(uint32_t length) {
            used_space_ = length;
        }

        //! \brief  Get the length of a message in the memory region.
        //! \return Message length value.

        inline uint32_t getMessageLength(void) const {
            return used_space_;
        }

        //! \brief  Return indicator if a message is ready.
        //! \return True if a message is ready, otherwise false.

        inline bool isMessageReady(void) const {
            return used_space_ > 0 ? true : false;
        }

        enum {
            BLOCK_USER = 1,
            BLOCK_TEMP = 2,
        };

        //! Check is this region was constructed from user allocated memory
        inline bool isUserRegion() {
            return (flags_ & BLOCK_USER) == BLOCK_USER;
        }
        inline bool isTempRegion() {
            return (flags_ & BLOCK_TEMP) == BLOCK_TEMP;
        }
        inline void setTempRegion() {
            flags_ |= BLOCK_TEMP;
        }

    private:

        //! Memory region.
        struct ibv_mr *region_;

        //! flags to control lifetime of blocks
        int flags_;

        //! Length of a message in the memory region.
        uint32_t used_space_;

    };

    // Smart pointer for rdma_memory_region object.
    typedef std::shared_ptr<rdma_memory_region> rdma_memory_region_ptr;

}}}}

#endif // COMMON_RDMAMEMORYREGION_H
