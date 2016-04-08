#ifndef _CH_PROTOCOL_H_
#define _CH_PROTOCOL_H_

#include <list>

#include "chatter/packet.h"

namespace chatter {

class Host;
class Peer;

struct ProtocolChannel
{
    ProtocolChannelID id;
    SeqNum next_sequence;
    std::list<Packet::ptr> sent_reliable;
};

class Protocol
{
public:
    Protocol(Host* host);
    ~Protocol();

    bool connect(Peer* peer);
    bool disconnect(Peer* peer);
    void handle_message(Peer* peer, const uint8_t* msg, std::size_t msg_size);
    void update(Peer* peer, uint64_t timestamp);
    void send(Packet::ptr packet, bool immediate = false);

private:
    std::vector<Packet::ptr> parse_message(Peer* peer, const uint8_t* msg, std::size_t msg_size);
    bool handle_ping(const Packet::ptr packet);
    bool handle_pong(const Packet::ptr packet);
    bool handle_ack(const Packet::ptr packet);
    bool handle_connect_request(const Packet::ptr packet);
    bool handle_connect_response(const Packet::ptr packet);
    bool handle_connect_acknowledge(const Packet::ptr packet);
    bool handle_connect_complete(const Packet::ptr packet);
    bool handle_disconnect_notify(const Packet::ptr packet);
    bool handle_user_data(const Packet::ptr packet);
    void send_ack(Packet::ptr packet);
    bool detect_disconnect(Peer* peer, uint64_t timestamp);
    void service_rtt(Peer* peer, uint64_t timestamp);
    void do_resends(Peer* peer, uint64_t timestamp);
    void calculate_rtt(Peer* peer, uint64_t measurement);
    uint16_t limit_rto(uint16_t rto);

    Host* m_host;
};

} // namespace chatter
#endif // _CH_PROTOCOL_H_
