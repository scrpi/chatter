#ifndef _CH_PACKET_LISTENER_H_
#define _CH_PACKET_LISTENER_H_

#include <string>
#include <fstream>

namespace chatter {

class PacketStats;
class PeerStats;

class PacketListener
{
public:
    PacketListener() = default;
    virtual ~PacketListener() {}

    virtual void on_send(uint64_t ts, const PacketStats &packet_stats, const PeerStats &peer_stats) = 0;
    virtual void on_recv(uint64_t ts, const PacketStats &packet_stats, const PeerStats &peer_stats) = 0;

    std::string debug_string(const PacketStats &packet_stats, const PeerStats &peer_stats);
};

class PacketLoggerConsole : public PacketListener
{
public:
    ~PacketLoggerConsole();
    virtual void on_send(uint64_t ts, const PacketStats& packet_stats, const PeerStats& peer_stats);
    virtual void on_recv(uint64_t ts, const PacketStats& packet_stats, const PeerStats& peer_stats);
};

class PacketLoggerCSV: public PacketListener
{
public:
    PacketLoggerCSV();
    ~PacketLoggerCSV();
    virtual void on_send(uint64_t ts, const PacketStats& packet_stats, const PeerStats& peer_stats);
    virtual void on_recv(uint64_t ts, const PacketStats& packet_stats, const PeerStats& peer_stats);

private:
    std::ofstream m_file;
};

} // namespace chatter

#endif // _CH_PACKET_LISTENER_H_

