#ifndef _CH_EVENT_H_
#define _CH_EVENT_H_

#include "chatter/hostaddress.h"
#include "chatter/packet.h"

namespace chatter {

enum EventType
{
    PEER_CONNECTED,
    PEER_DISCONNECTED,
    PEER_TIMED_OUT,
    PEER_UNABLE_TO_CONNECT,
    PACKET_RECEIVED,
    PACKET_NOT_DELIVERED,
};

struct Event
{
    typedef std::shared_ptr<Event> ptr;
    EventType type;
    HostAddress address;
    Packet::ptr packet = nullptr;

    static Event::ptr create(EventType t)
    {
        Event::ptr e = std::make_shared<Event>();
        e->type = t;
        return e;
    }

    std::string to_string()
    {
        switch (type) {
            case PEER_CONNECTED:         return "PEER_CONNECTED";
            case PEER_DISCONNECTED:      return "PEER_DISCONNECTED";
            case PEER_TIMED_OUT:         return "PEER_TIMED_OUT";
            case PEER_UNABLE_TO_CONNECT: return "PEER_UNABLE_TO_CONNECT";
            case PACKET_RECEIVED:        return "PACKET_RECEIVED";
            case PACKET_NOT_DELIVERED:   return "PACKET_NOT_DELIVERED";
            default:
                return "";
        }
    }
};

} // namespace chatter

#endif // _CH_EVENT_H_
