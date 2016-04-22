#include "chatter/protocol.h"

#include <cmath>
#include <cstring>
#include <iostream>
#include <bitset>

#include "chatter/host.h"
#include "chatter/packet_listener.h"

namespace chatter {

Protocol::Protocol(Host* host)
    : m_host(host)
{
}

Protocol::~Protocol()
{
}

void Protocol::send(Packet::ptr packet, bool immediate /* = false */)
{
    if (!packet)
        return;

    Peer* peer = packet->m_peer;

    if (packet->has_flag(PacketFlag::RELIABLE)) {
        /* Assign a sequence number for this packet and track it.*/
        ProtocolChannel& chan = peer->m_channels[packet->get_channel()];
        packet->m_sequence_num = chan.next_sequence++;
        chan.sent_reliable.push_back(packet);
    }

    packet->m_rto = limit_rto(peer->get_rto());
    packet->m_send_queued = true;

    m_host->queue_outgoing_packet(packet, immediate);
}

bool Protocol::connect(Peer* peer)
{
    if (peer->m_state != PeerState::DISCONNECTED)
        return false;

    peer->m_state = PeerState::CONNECTION_REQUESTED;
    peer->m_connect_ts = m_host->timestamp_now();

    auto p = Packet::create();
    p->m_peer = peer;
    p->set_type(PacketType::CONNECT_REQUEST);
    p->set_flag(PacketFlag::RELIABLE);

    send(p, true);

    return true;
}

bool Protocol::disconnect(Peer* peer)
{
    if (peer->m_state != PeerState::CONNECTED)
        return false;

    auto p = Packet::create();
    p->m_peer = peer;
    p->set_type(PacketType::DISCONNECT_NOTIFY);
    p->set_flag(PacketFlag::RELIABLE);

    send(p, true);

    return true;
}

std::vector<Packet::ptr> Protocol::parse_message(Peer* peer, const uint8_t* msg, std::size_t msg_size)
{
    std::vector<Packet::ptr> packets;
    std::size_t msg_cursor = 0;

    while (msg_cursor < msg_size) {
        packets.push_back(std::make_shared<Packet>());
        Packet::ptr p = packets.back();
        p->m_peer = peer;

        p->m_cmd = platform::NetToHost16(*reinterpret_cast<const ProtocolCommand*>(&msg[msg_cursor]));
        msg_cursor += sizeof(ProtocolCommand);

        if (p->has_flag(PacketFlag::RELIABLE)) {
            /* Read sequence number */
            p->m_sequence_num = platform::NetToHost32(*reinterpret_cast<const SeqNum*>(&msg[msg_cursor]));
            msg_cursor += sizeof(SeqNum);
        }

        int data_len = msg_size - msg_cursor;
        if (data_len > 0) {
            p->m_data.resize(data_len);
            std::memcpy(&p->m_data[0], &msg[msg_cursor], data_len);
        }

        /* TODO(ben): Do proper multi-packet message parsing later */
        msg_cursor = msg_size;
    }

    return packets;
}

bool Protocol::handle_ping(const Packet::ptr packet)
{
    Peer* peer = packet->m_peer;
    if (peer->m_state != PeerState::CONNECTED)
        /* TODO(ben): Peer not connected error */
        return false;

    peer->m_last_ping_ts = m_host->timestamp_now();

    uint64_t remote_timestamp;
    packet->read(remote_timestamp);

    auto p = Packet::create();
    p->m_peer = peer;
    p->set_type(PacketType::PROTO_PONG);
    p->write(remote_timestamp);
    send(p, true); /* Send immediate to get accurate round-trip time (TODO) */

    return true;
}

bool Protocol::handle_pong(const Packet::ptr packet)
{
    Peer* peer = packet->m_peer;
    if (peer->m_state != PeerState::CONNECTED)
        /* TODO(ben): Peer not connected error */
        return false;

    uint64_t ts;
    packet->read(ts);

    calculate_rtt(peer, m_host->timestamp_now() - ts);

    return true;
}

bool Protocol::handle_ack(const Packet::ptr packet)
{
    uint8_t channel_id = kReliableUnorderedChannel; /* default to unordered reliable channel */
    SeqNum seq_num;

    packet->read(seq_num);

    if (packet->has_more_data())
        packet->read(channel_id);

    if (packet->m_peer->m_state == PeerState::DISCONNECT_PENDING) {
        /* Treat any ack coming in after DISCONNECT_PENDING state is set as
         * a disconnect acknowledgement.
         */
        packet->m_peer->reset();
        auto e = Event::create(EventType::PEER_DISCONNECTED);
        e->address = packet->m_peer->m_address;
        e->packet = packet;
        m_host->queue_event(e);
        return true;
    }

    auto sent = packet->m_peer->ack_packet(channel_id, seq_num);

    if (!sent) {
        /* TODO(ben): check for dup ack - perform fast recovery etc etc */
    }
    else {
        /* Acked packet - decrement bytes on wire */
        sent->m_peer->m_bytes_on_wire -= sent->data_len();

        if (sent->m_send_count == 1) {
            /* No retransmissions were made for this packet, we can use it to calc RTT */
            calculate_rtt(sent->m_peer, m_host->timestamp_now() - sent->m_last_send_time);

            /* Also use this ack to increase congestion window */
            packet->m_peer->m_congestion_window += kCongestionInc;
            if (packet->m_peer->m_congestion_window > kMaxCongestionWindow)
                packet->m_peer->m_congestion_window = kMaxCongestionWindow;
        }
    }

    return true;
}

bool Protocol::handle_connect_request(const Packet::ptr packet)
{
    Peer* peer = packet->m_peer;
    if (peer->m_state != PeerState::DISCONNECTED)
        /* TODO(ben): Peer already connected error */
        return false;

    peer->m_state = PeerState::CONNECTION_RESPONDED;
    peer->m_is_incoming_connection = true;

    peer->m_connect_ts = m_host->timestamp_now();

    /* Send response back to peer */
    auto p = Packet::create();
    p->m_peer = peer;
    p->set_type(PacketType::CONNECT_RESPONSE);
    p->set_flag(PacketFlag::RELIABLE);
    p->write(packet->m_sequence_num); /* Sequence number of incoming packet */
    p->write(static_cast<bool>(true));
    send(p);

    return true;
}

bool Protocol::handle_connect_response(const Packet::ptr packet)
{
    Peer* peer = packet->m_peer;
    if (peer->m_state != PeerState::CONNECTION_REQUESTED)
        /* TODO(ben): Peer didn't initiate a connection error ? */
        return false;

    peer->m_rtt_avg = m_host->timestamp_now() - peer->m_connect_ts;
    peer->m_last_rtt_ts = m_host->timestamp_now();

    SeqNum seq_num;
    packet->read(seq_num);
    bool ok;
    packet->read(ok);

    /* Remove packet from sent_reliable */
    auto sent = peer->ack_packet(kReliableUnorderedChannel, seq_num);
    if (sent)
        peer->m_bytes_on_wire -= sent->data_len();

    if (!ok) {
        peer->m_state = PeerState::DISCONNECTED;
        return true;
    }

    peer->m_state = PeerState::CONNECTION_ACKNOWLEDGED;

    /* Send Acknowledge packet back to peer */
    auto p = Packet::create();
    p->m_peer = peer;
    p->set_type(PacketType::CONNECT_ACKNOWLEDGE);
    p->set_flag(PacketFlag::RELIABLE);
    p->write(packet->m_sequence_num); /* Sequence number of incoming packet */
    send(p, true);

    return true;
}

bool Protocol::handle_connect_acknowledge(const Packet::ptr packet)
{
    Peer* peer = packet->m_peer;
    if (peer->m_state != PeerState::CONNECTION_RESPONDED)
        /* TODO(ben): Peer didn't respond to a connection error ? */
        return false;

    peer->m_rtt_avg = m_host->timestamp_now() - peer->m_connect_ts;
    peer->m_last_rtt_ts = m_host->timestamp_now();

    SeqNum seq_num;
    packet->read(seq_num);

    /* Remove packet from sent_reliable */
    auto sent = peer->ack_packet(kReliableUnorderedChannel, seq_num);
    if (sent)
        peer->m_bytes_on_wire -= sent->data_len();

    peer->m_state = PeerState::CONNECTED;

    auto p = Packet::create();
    p->m_peer = peer;
    p->set_type(PacketType::CONNECT_COMPLETE);
    p->set_flag(PacketFlag::RELIABLE);
    p->write(packet->m_sequence_num); /* Sequence number of incoming packet */
    send(p, true);

    auto e = Event::create(EventType::PEER_CONNECTED);
    e->address = packet->m_peer->m_address;
    m_host->queue_event(e);

    return true;
}

bool Protocol::handle_connect_complete(const Packet::ptr packet)
{
    Peer* peer = packet->m_peer;
    if (peer->m_state != PeerState::CONNECTION_ACKNOWLEDGED)
        /* TODO(ben): Peer didn't initiate a connection error ? */
        return false;

    SeqNum seq_num;
    packet->read(seq_num);

    /* Remove packet from sent_reliable */
    auto sent = peer->ack_packet(kReliableUnorderedChannel, seq_num);
    if (sent)
        peer->m_bytes_on_wire -= sent->data_len();

    peer->m_state = PeerState::CONNECTED;

    auto e = Event::create(EventType::PEER_CONNECTED);
    e->address = packet->m_peer->m_address;
    m_host->queue_event(e);

    return true;
};

bool Protocol::handle_disconnect_notify(const Packet::ptr packet)
{
    Peer* peer = packet->m_peer;
    if (peer->m_state != PeerState::CONNECTED)
        /* TODO(ben): Peer didn't initiate a connection error ? */
        return false;

    peer->reset();

    auto e = Event::create(EventType::PEER_DISCONNECTED);
    e->address = packet->m_peer->m_address;
    m_host->queue_event(e);

    return true;
}

bool Protocol::handle_user_data(const Packet::ptr packet)
{
    Peer* peer = packet->m_peer;
    if (peer->m_state != PeerState::CONNECTED)
        /* TODO(ben): Peer didn't initiate a connection error ? */
        return false;

    if (!packet->m_data.size())
        return false;

    auto e = Event::create(EventType::PACKET_RECEIVED);
    e->address = packet->m_peer->m_address;
    e->packet = packet;
    m_host->queue_event(e);

    return true;
}

void Protocol::send_ack(Packet::ptr packet)
{
    switch (packet->get_type()) {
        /* These packets should not generate an ack as they are handled
         * specifically in the protocol.
         */
        case PacketType::PROTO_PING:
        case PacketType::PROTO_PONG:
        case PacketType::PROTO_ACK:
        case PacketType::CONNECT_REQUEST:
        case PacketType::CONNECT_RESPONSE:
        case PacketType::CONNECT_ACKNOWLEDGE:
            return;
        /* These packets should generate normal acks. */
        case PacketType::CONNECT_COMPLETE:
        case PacketType::DISCONNECT_NOTIFY:
        case PacketType::USER_DATA:
        default:
            break;
    }

    auto ack = Packet::create();
    ack->m_peer = packet->m_peer;
    ack->set_type(PacketType::PROTO_ACK);

    ack->write(packet->m_sequence_num);
    if (packet->has_flag(PacketFlag::ORDERED))
        ack->write(static_cast<uint8_t>(packet->get_channel()));

    /* TODO(ben): send duplicate acks if unordered sequence received (fast retransmission support) */
    send(ack, true);
}

void Protocol::handle_message(Peer* peer, const uint8_t* msg, std::size_t msg_size)
{
    std::vector<Packet::ptr> packets = parse_message(peer, msg, msg_size);

    if (packets.size())
        peer->m_last_recv_ts = m_host->timestamp_now();

    for (auto& p : packets) {

        if (m_host->m_packet_listener) {
            PacketStats packet_s;
            PeerStats peer_s;
            m_host->build_packet_stats(p, packet_s, peer_s);
            m_host->m_packet_listener->on_recv(m_host->timestamp_now(), packet_s, peer_s);
        }
#ifdef CHATTER_DEBUG
        /*
         *
        std::cout << m_host->timestamp_now() << " RECV<<<";
        std::cout << p->debug_string();
        std::cout << std::endl;
        */
#endif

        if (p->has_flag(RELIABLE))
            send_ack(p);

        switch (p->get_type()) {
        case PacketType::PROTO_PING:
            handle_ping(p);
            break;

        case PacketType::PROTO_PONG:
            handle_pong(p);
            break;

        case PacketType::PROTO_ACK:
            handle_ack(p);
            break;

        case PacketType::CONNECT_REQUEST: /* C -> S */
            handle_connect_request(p);
            break;

        case PacketType::CONNECT_RESPONSE: /* S -> C */
            handle_connect_response(p);
            break;

        case PacketType::CONNECT_ACKNOWLEDGE: /* C -> S */
            handle_connect_acknowledge(p);
            break;

        case PacketType::CONNECT_COMPLETE: /* S -> C */
            handle_connect_complete(p);
            break;

        case PacketType::DISCONNECT_NOTIFY: /* S -> C */
            handle_disconnect_notify(p);
            break;

        case PacketType::USER_DATA:
            handle_user_data(p);
            break;

        default:
            break;
        }
    }
}

bool Protocol::detect_disconnect(Peer* peer, uint64_t timestamp)
{
    uint64_t time_since_last_recv = timestamp - peer->m_last_recv_ts;

    return (time_since_last_recv > kPeerTimeOut);
}

void Protocol::service_rtt(Peer* peer, uint64_t timestamp)
{
    /* RTT calculations are triggered by pings or acks - so we generate pings
     * if RTT has not been calculated for a while */
    uint64_t time_since_last_ping = timestamp - peer->m_last_ping_ts;

    if (time_since_last_ping > kPingInterval) {
        /* Send Ping */
        auto p = Packet::create();
        p->m_peer = peer;
        p->set_type(PacketType::PROTO_PING);
        p->write(timestamp);
        send(p, true);
        peer->m_last_ping_ts = timestamp;
    }
}

void Protocol::do_resends(Peer* peer, uint64_t timestamp)
{
    uint64_t time_since_last_send = 0;

    for (auto& chan : peer->m_channels) {
        for (auto& p : chan.sent_reliable) {
            time_since_last_send = timestamp - p->m_last_send_time;

            if (!p->m_send_queued && time_since_last_send > p->m_rto) {
                /* Packet timed out, need to retransmit */
                /* Calculate the new time-out */
                p->m_rto = limit_rto(p->m_rto * kRetransmissionBackOffFactor);

                /* Retransmissions trigger a congestion window decrease */
                peer->m_congestion_window /= kCongestionDecFactor;
                if (peer->m_congestion_window < kMinCongestionWindow)
                    peer->m_congestion_window = kMinCongestionWindow;

                p->m_send_queued = true;

                m_host->queue_outgoing_packet(p);
            }
        }
    }
}

void Protocol::update(Peer* peer, uint64_t timestamp)
{
    if (!peer)
        return;

    switch (peer->m_state) {
        case PeerState::DISCONNECTED:
            return; /* We shouldn't be given a disconnected peer o.O */

        case PeerState::CONNECTION_REQUESTED:
        case PeerState::CONNECTION_RESPONDED:
        case PeerState::CONNECTION_ACKNOWLEDGED:
            {
                /* Check for long-running connection attempts and kill them */
                uint64_t time_since_connect = timestamp - peer->m_connect_ts;
                if (time_since_connect > kConnectTimeOut) {
                    if (!peer->m_is_incoming_connection) {
                        auto e = Event::create(EventType::PEER_UNABLE_TO_CONNECT);
                        e->address = peer->m_address;
                        m_host->queue_event(e);
                    }
                    peer->reset();
                    return;
                }
                /* Reliable packet re-sends */
                do_resends(peer, timestamp);
            }
            break;

        case PeerState::CONNECTED:
            {
                /* Disconnect detection */
                if (detect_disconnect(peer, timestamp)) {
                    auto e = Event::create(EventType::PEER_TIMED_OUT);
                    e->address = peer->m_address;
                    m_host->queue_event(e);
                    peer->reset();
                }

                /* Keep RTT calculations happening */
                uint64_t time_since_last_rtt = timestamp - peer->m_last_rtt_ts;
                if (time_since_last_rtt > kPingInterval)
                    service_rtt(peer, timestamp);

                /* Reliable packet re-sends */
                do_resends(peer, timestamp);
            }
            break;

        default:
            break;
    }
}

void Protocol::calculate_rtt(Peer* peer, uint64_t measurement)
{
    static const float avg_gain = 0.125;
    static const float dev_gain = 0.25;

    if (!peer)
        return;

    float error = measurement - peer->m_rtt_avg;

    peer->m_rtt_avg += static_cast<uint64_t>(avg_gain * error);
    peer->m_rtt_dev += static_cast<uint64_t>(dev_gain * (std::abs(error) - peer->m_rtt_dev));
    peer->m_last_rtt_ts = m_host->timestamp_now();
}

uint16_t Protocol::limit_rto(uint16_t rto)
{
    if (rto > kMaxRetransmissionInterval)
        rto = kMaxRetransmissionInterval;
    if (rto < kMinRetransmissionInterval)
        rto = kMinRetransmissionInterval;
    return rto;
}

} // namespace chatter
