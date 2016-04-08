#ifndef _CH_HOST_H_
#define _CH_HOST_H_

#include <string>
#include <vector>
#include <queue>
#include <cstdint>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

#include "chatter/platform.h"
#include "chatter/config.h"
#include "chatter/peer.h"
#include "chatter/ipaddress.h"
#include "chatter/protocol.h"
#include "chatter/event.h"

namespace chatter
{

struct RecvMsg
{
    uint32_t timestamp;
    uint8_t msg[kMTU];
    std::size_t msg_size;
    HostAddress address;
};

class PacketListener;

class Host
{
public:
    Host();
    ~Host();

    enum StartResult {
        START_OK,
        ALREADY_RUNNING,
        SOCKET_CREATE_FAILED,
        SOCKET_BIND_FAILED
    };

    StartResult start(const HostAddress& bind_address, uint16_t max_connections);
    bool connect(const HostAddress& host_address);
    void shutdown();
    bool is_active(); // True if network thread is running.
    uint64_t timestamp_now();
    Event::ptr get_event();
    void send(const HostAddress& address, Packet::ptr packet);
    void register_packet_listener(PacketListener *listener);
    void build_packet_stats(Packet::ptr packet, PacketStats &packet_s, PeerStats &peer_s);

private:
    friend class Protocol;

    Peer* find_available_peer(const HostAddress& address);
    Peer* find_peer_by_address(const HostAddress& address);
    bool create_socket();
    void destroy_socket();
    void net_worker();
    void recv_worker();
    void receive_message(const RecvMsg& msg);
    void queue_outgoing_packet(const Packet::ptr packet, bool immediate = false);
    void send_packet_internal(const Packet::ptr packet);
    void queue_event(const Event::ptr event);

    Socket m_socket = CH_SOCKET_NULL;
    uint16_t m_max_connections = 1;

    bool m_run_threads = false;
    std::unique_ptr<std::thread> m_net_worker;
    std::unique_ptr<std::thread> m_recv_worker;

    std::vector<Peer> m_peers;

    Protocol m_protocol;

    std::chrono::high_resolution_clock::time_point m_start_time;

    std::vector<RecvMsg> m_recv_queue;
    std::mutex m_recv_queue_mutex;
    std::condition_variable m_recv_queue_cv;

    std::list<Packet::ptr> m_send_queue;
    std::mutex m_send_queue_mutex;

    std::queue<Event::ptr> m_event_queue;
    std::mutex m_event_queue_mutex;

    PacketListener *m_packet_listener = nullptr;
};

} // namespace chatter

#endif // _CH_HOST_H_
