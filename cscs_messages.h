#ifndef CSCS_MESSAGES_H
#define CSCS_MESSAGES_H

//
// only declare this to C++ code
//
#ifdef __cplusplus

#include <ramdisk/include/services/UserMessages.h>

namespace CSCS_user_message {

  // small packet                                = 512,
  // bgcios::ImmediateMessageSize                = 512
  // sizeof(bgcios::MessageHeader)               = 32
  // sizeof(CSCS_user_message::User_RDMA_Header) = 24

  struct User_RDMA_Header {
      uint32_t tag;
  };

#define CSCS_UserMessageDataSize (512 - sizeof(bgcios::MessageHeader) - sizeof(CSCS_user_message::User_RDMA_Header))

  // For now we are using a borrowed header structure from bgcios, but in fact we don't need
  // most of it and will change to a simpler smaller structure in future
  struct UserRDMA_message {
      struct bgcios::MessageHeader                header;
      struct CSCS_user_message::User_RDMA_Header  header2;
      char MessageData[CSCS_UserMessageDataSize];   //!< Message data, length in header = sizeof(struct MessageHeader) + amount of data
  };

  const uint8_t WriteMessage = 1; // we will write something into BGAS memory
  const uint8_t ReadMessage  = 2; // request BGAS to read from our memory
  const uint8_t TextMessage  = 3; // Tell BGAS to print out some info (debug)
  const uint8_t UnexpectedMessage = 4;
  const uint8_t ExpectedMessage   = 5;

};
#endif

#endif
