#ifndef _CH_TYPES_H_
#define _CH_TYPES_H_

#include <cstdint>

namespace chatter {

const int kReliableUnorderedChannel = 32;

typedef uint32_t SeqNum;
typedef uint32_t PeerID;
typedef uint16_t ProtocolCommand;
typedef uint8_t  ProtocolChannelID;

enum class PeerState {
    DISCONNECTED,
    CONNECTION_REQUESTED,
    CONNECTION_RESPONDED,
    CONNECTION_ACKNOWLEDGED,
    CONNECTED,
    DISCONNECT_PENDING,
};

enum class PacketType
{                        /* C S */
    CONNECT_REQUEST,     /*  -> */
    CONNECT_RESPONSE,    /* <-  */
    CONNECT_ACKNOWLEDGE, /*  -> */
    CONNECT_COMPLETE,    /* <-  */
    PROTO_ACK,
    PROTO_PING,
    PROTO_PONG,
    DISCONNECT_NOTIFY,
    USER_DATA,      /* <-> */
};

enum PacketFlag
{
    RELIABLE    = 1 << 0,
    ORDERED     = 1 << 1,
    SEQUENCED   = 1 << 2,
    TIMESTAMPED = 1 << 3,
};

} // namespace chatter

#endif // _CH_TYPES_H_
