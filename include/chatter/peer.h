#ifndef _CH_PEER_H_
#define _CH_PEER_H_

#include <vector>

#include "chatter/types.h"
#include "chatter/hostaddress.h"
#include "chatter/protocol.h"

namespace chatter {

struct PeerStats
{
    PeerID id;
    HostAddress address;
    bool incoming_connection;
    uint64_t connect_time;
    uint16_t rtt_avg;
    uint16_t rtt_dev;
    uint32_t congestion_window;
    uint32_t bytes_on_wire;
};

class Peer
{
public:
    Peer();
    ~Peer() {}

    const HostAddress& get_address();

private:
    friend class Host;
    friend class Protocol;

    Packet::ptr ack_packet(ProtocolChannelID channel_id, SeqNum sequence);
    void reset();

    /* Calculates retransmission timeout to be set for a packet */
    uint16_t get_rto();

    bool congestion_window_full() { return m_bytes_on_wire >= m_congestion_window; }

    PeerState       m_state = PeerState::DISCONNECTED;
    PeerID          m_id = 0;
    HostAddress     m_address;
    bool            m_is_incoming_connection;   //> This peer was an incoming connection (they connected to us)

    uint64_t        m_connect_ts;               //> Timestamp of initial connection

    uint64_t        m_last_recv_ts;             //> Timestamp of last received packet of any kind
    uint64_t        m_last_ping_ts;             //> Timestamp of last ping sent
    uint64_t        m_last_rtt_ts;              //> Timestamp of last rtt calculation

    uint16_t        m_rtt_avg;                  //> Round trip time average
    uint16_t        m_rtt_dev;                  //> Round trip time deviation

    uint32_t        m_congestion_window;        //> Limit bytes in flight on the wire
    uint32_t        m_bytes_on_wire;

    /* 0 -> 31 for ordered packets. 32 for unordered reliable */
    ProtocolChannel m_channels[33];

};

} // namespace chatter

#endif // _CH_PEER_H_
