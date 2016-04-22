#include "chatter/host.h"

#include <sys/socket.h>
#include <iostream>
#include <memory>
#include <bitset>

#include "chatter/config.h"
#include "chatter/packet_listener.h"

namespace chatter {

Host::Host()
    : m_protocol(this)
{
}

Host::~Host()
{
    shutdown();
}

Host::StartResult Host::start(const HostAddress& bind_address, uint16_t max_connections)
{
    if (m_run_threads)
        return ALREADY_RUNNING;

    if (!create_socket())
        return SOCKET_CREATE_FAILED;

    if (!platform::SocketBind(m_socket, bind_address))
        return SOCKET_BIND_FAILED;

    m_max_connections = max_connections > 0 ? max_connections : 1;

    m_peers.clear();
    m_peers.reserve(m_max_connections);
    for (int peer_id = 0; peer_id < m_max_connections; ++peer_id) {
        Peer p;
        p.m_id = peer_id;
        m_peers.push_back(p);
    }

    std::cout << "m_peers allocated: size:" << m_peers.size() << " cap:" << m_peers.capacity() << std::endl;

    m_start_time = std::chrono::high_resolution_clock::now();

    m_run_threads = true;
    m_net_worker.reset(new std::thread(&Host::net_worker, this));
    m_net_worker->detach();
    m_recv_worker.reset(new std::thread(&Host::recv_worker, this));
    m_recv_worker->detach();

    return START_OK;
}

Peer* Host::find_available_peer(const HostAddress& address)
{
    Peer *peer = nullptr;

    for (std::size_t i = 0; i < m_peers.size(); ++i) {
        if (m_peers[i].m_state  == PeerState::DISCONNECTED) {
            peer = &m_peers[i];
            peer->m_id = i;
            peer->m_address = address;
            break;
        }
    }

    return peer;
}

Peer* Host::find_peer_by_address(const HostAddress& address)
{
    // TODO(ben): more efficient search algorithm
    Peer* peer = nullptr;

    for (auto& p : m_peers) {
        if (p.m_address == address) {
            peer = &p;
            break;
        }
    }

    return peer;
}

bool Host::connect(const HostAddress& address)
{
    Peer* peer = find_available_peer(address);

    if (!peer)
        return false;

    return m_protocol.connect(peer);
}

void Host::shutdown()
{
    for (auto &peer : m_peers) {
        if (peer.m_state == PeerState::CONNECTED) {
            m_protocol.disconnect(&peer);
        }
        peer.reset();
    }

    m_run_threads = false;
    m_peers.clear();

    /* TODO(ben): Wait for network thread to stop? */
    destroy_socket();
}

bool Host::is_active()
{
    return m_run_threads;
}

uint64_t Host::timestamp_now()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start_time);
    return int_ms.count();
}

bool Host::create_socket()
{
    if (m_socket != CH_SOCKET_NULL)
        destroy_socket();

    if ((m_socket = platform::SocketCreate()) == CH_SOCKET_NULL)
        return false;

    return true;
}

void Host::destroy_socket()
{
    if (m_socket != CH_SOCKET_NULL)
        platform::SocketDestroy(m_socket);
    m_socket = CH_SOCKET_NULL;
}

void Host::receive_message(const RecvMsg& msg)
{
    /* Our protocol will always be at least ProtocolCommand size */
    if (msg.msg_size < static_cast<ssize_t>(sizeof(ProtocolCommand)))
        return;

    Peer* peer = find_peer_by_address(msg.address);

    if (!peer)
        /* No peer matching this address is currently connected or connecting.
         * Try to find an avaliable peer slot for this message */
        peer = find_available_peer(msg.address);

    if (!peer)
        /* No connections available */
        /* TODO(ben): Send packet to user informing no connections available */
        return;

    m_protocol.handle_message(peer, msg.msg, msg.msg_size);
}

void Host::queue_outgoing_packet(const Packet::ptr packet, bool immediate /* = false */)
{
    if (!packet)
        return;

    if (!immediate) {
        std::lock_guard<std::mutex> lock(m_send_queue_mutex);
        m_send_queue.push_back(packet);
    }
    else {
        send_packet_internal(packet);
    }
}

void Host::net_worker()
{
    while (m_run_threads) {
        /* Service recv queue */
        {
            std::unique_lock<std::mutex> lock(m_recv_queue_mutex);
            m_recv_queue_cv.wait_for(
                    lock,
                    std::chrono::milliseconds(10),
                    [&](){return m_recv_queue.size();});


            for (auto& msg : m_recv_queue)
                receive_message(msg);

            m_recv_queue.clear();
        }

        /* Service send queue */
        {
            std::lock_guard<std::mutex> lock(m_send_queue_mutex);
            auto itr = m_send_queue.begin();
            Packet::ptr p = nullptr;
            while (itr != m_send_queue.end()) {
                p = *itr;
                if (!p->m_peer->congestion_window_full()) {
                    send_packet_internal(p);
                    itr = m_send_queue.erase(itr);
                }
                else {
                    ++itr;
                }
            }
        }

        /* Protocol periodic stuff */
        for (int peer_id = 0; peer_id < m_max_connections; peer_id++) {
            if (m_peers[peer_id].m_state != PeerState::DISCONNECTED)
                m_protocol.update(&m_peers[peer_id], timestamp_now());
        }
    }
}

void Host::recv_worker()
{
    while (m_run_threads) {
        /* Receive packets! */
        RecvMsg msg;

        /* Blocking recvfrom */
        msg.msg_size = platform::SocketRecvFrom(m_socket, (void*)&msg.msg, kMaxUDPPayloadSize, &msg.address);

        if (msg.msg_size > 0) {
            {
                std::lock_guard<std::mutex> lock(m_recv_queue_mutex);
                m_recv_queue.push_back(msg);
            }
            m_recv_queue_cv.notify_one();
        }
    }
}

void Host::send_packet_internal(const Packet::ptr packet)
{
    if (!packet->m_peer)
        /* TODO(ben): Generate error - no destination */
        return;

    if (packet->m_peer->m_state == PeerState::DISCONNECTED)
        return;

    uint8_t buf[kMaxUDPPayloadSize];
    std::size_t raw_len = packet->read_raw(buf, kMaxUDPPayloadSize);

    packet->m_last_send_time = timestamp_now();
    packet->m_send_count++;

    if (packet->has_flag(PacketFlag::RELIABLE)) {
        /* Reliable packets contribute to congestion control */
        packet->m_peer->m_bytes_on_wire += packet->data_len();
    }

    if (m_packet_listener) {
        PacketStats packet_s;
        PeerStats peer_s;
        build_packet_stats(packet, packet_s, peer_s);
        m_packet_listener->on_send(timestamp_now(), packet_s, peer_s);
    }

    packet->m_send_queued = false;

    if (raw_len)
        platform::SocketSendTo(m_socket, buf, raw_len, packet->m_peer->m_address);
}

Event::ptr Host::get_event()
{
    Event::ptr ret;

    std::lock_guard<std::mutex> lock(m_event_queue_mutex);
    if (m_event_queue.empty())
        ret = nullptr;

    else {
        ret = m_event_queue.front();
        m_event_queue.pop();
    }

    return ret;
}

void Host::send(const HostAddress& address, Packet::ptr packet)
{
    if (!packet)
        return;

    packet->m_peer = find_peer_by_address(address);

    if (!packet->m_peer)
        return;
    /* TODO(ben): function should allow returning error */

    m_protocol.send(packet);
}

void Host::queue_event(const Event::ptr event)
{
    std::lock_guard<std::mutex> lock(m_event_queue_mutex);
    m_event_queue.push(event);
}

void Host::register_packet_listener(PacketListener* listener)
{
    m_packet_listener = listener;
}

void Host::build_packet_stats(Packet::ptr packet, PacketStats &packet_s, PeerStats &peer_s)
{
    if (!packet)
        return;

    Peer* peer = packet->m_peer;
    if (!peer)
        return;

    packet_s.type = packet->get_type();
    packet_s.reliable = packet->has_flag(PacketFlag::RELIABLE);
    packet_s.ordered = packet->has_flag(PacketFlag::ORDERED);
    packet_s.sequenced = packet->has_flag(PacketFlag::SEQUENCED);
    packet_s.timestamped = packet->has_flag(PacketFlag::TIMESTAMPED);
    if (packet_s.type == PacketType::PROTO_ACK) {
        packet->m_read_pos = 0;
        packet->read(packet_s.sequence_number);
        if (packet->has_more_data())
            packet->read(packet_s.channel);
        else
            packet_s.channel = 32;
        packet->m_read_pos = 0;
    }
    else {
        packet_s.channel = packet->get_channel();
        packet_s.sequence_number = packet->m_sequence_num;
    }
    packet_s.data_len = packet->data_len();
    packet_s.rto = packet->m_rto;
    packet_s.send_count = packet->m_send_count;
    packet_s.send_queued = packet->m_send_queued;

    peer_s.id = peer->m_id;
    peer_s.address = peer->m_address;
    peer_s.incoming_connection = peer->m_is_incoming_connection;
    peer_s.connect_time = peer->m_connect_ts;
    peer_s.rtt_avg = peer->m_rtt_avg;
    peer_s.rtt_dev = peer->m_rtt_dev;
    peer_s.congestion_window = peer->m_congestion_window;
    peer_s.bytes_on_wire = peer->m_bytes_on_wire;
}

} // namespace chatter
