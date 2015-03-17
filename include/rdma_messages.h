#ifndef CSCS_MESSAGES_H
#define CSCS_MESSAGES_H

//
// only declare this to C++ code
//
#ifdef __cplusplus

//#include <UserMessages.h>

namespace CSCS_user_message {

  // small packet                                = 512,
  // bgcios::ImmediateMessageSize                = 512
  // sizeof(bgcios::MessageHeader)               = 32
  // sizeof(CSCS_user_message::User_RDMA_Header) = 24

  struct User_RDMA_Header {
      uint32_t tag;
      uint32_t expected;
  };

//# define CSCS_UserMessageHeaderSize (sizeof(bgcios::MessageHeader) + sizeof(CSCS_user_message::User_RDMA_Header))
# define CSCS_UserMessageHeaderSize (sizeof(CSCS_user_message::User_RDMA_Header))
# define CSCS_UserMessageDataSize (512 - CSCS_UserMessageHeaderSize)

  // For now we are using a borrowed header structure from bgcios, but in fact we don't need
  // most of it and will change to a simpler smaller structure in future
  struct UserRDMA_message {
      struct CSCS_user_message::User_RDMA_Header  header;
      char MessageData[CSCS_UserMessageDataSize];
      //!< Message data, length in header = sizeof(struct MessageHeader) + amount of data
  };

  const uint8_t ExpectedMessage   = 110;
  const uint8_t UnexpectedMessage = 111;
  const uint8_t TextMessage = 02;

};
#endif

#endif
