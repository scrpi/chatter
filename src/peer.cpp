#include "chatter/peer.h"

#include <algorithm>

#include "chatter/config.h"

namespace chatter {

Peer::Peer()
{
    reset();
}

Packet::ptr Peer::ack_packet(ProtocolChannelID channel_id, SeqNum sequence_num)
{
    /* 0 -> 31 are ordered channels, 32 is used for unordered reliable */
    if (channel_id > 32)
        return nullptr;

    Packet::ptr packet = nullptr;
    ProtocolChannel& chan = m_channels[channel_id];

    std::list<Packet::ptr>::iterator itr;
    itr = std::find_if(
            chan.sent_reliable.begin(),
            chan.sent_reliable.end(),
            [&](Packet::ptr p){return p->m_sequence_num == sequence_num;}
        );

    if (itr != chan.sent_reliable.end()) {
        /* Found the packet! */
        packet = *itr;
        chan.sent_reliable.erase(itr);
    }

    return packet;
}

void Peer::reset()
{
    m_state = PeerState::DISCONNECTED;
    m_address = HostAddress();
    m_is_incoming_connection = false;
    m_connect_ts = 0;
    m_last_recv_ts = 0;
    m_last_ping_ts = 0;
    m_last_rtt_ts = 0;
    m_rtt_avg = 0;
    m_rtt_dev = 0;
    m_congestion_window = kMinCongestionWindow;
    m_bytes_on_wire = 0;

    for (int i = 0; i < 33; ++i) {
        m_channels[i].sent_reliable.clear();
        m_channels[i].next_sequence = 0;
        m_channels[i].id = i;
    }
}

uint16_t Peer::get_rto()
{
    return static_cast<uint16_t>(m_rtt_avg + 4 * m_rtt_dev);
}

const HostAddress& Peer::get_address()
{
    return m_address;
}

} // namespace chatter
