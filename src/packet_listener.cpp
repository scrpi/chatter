#include "chatter/packet_listener.h"
#include "chatter/packet.h"
#include "chatter/peer.h"

#include <iostream>
#include <iomanip>
#include <sstream>

namespace chatter {

std::string PacketListener::debug_string(const PacketStats &packet_stats, const PeerStats &peer_stats)
{
    std::ostringstream os;
    os << std::left;
    os << "len:" << std::setw(5) <<  packet_stats.data_len;
    os << "peer:" << std::setw(22) << peer_stats.address.to_string();
    os << " [";
    if (packet_stats.reliable)    os << "R"; else os << "-";
    if (packet_stats.ordered)     os << "O"; else os << "-";
    if (packet_stats.sequenced)   os << "S"; else os << "-";
    if (packet_stats.timestamped) os << "T"; else os << "-";
    os << "]";
    os << " CH:" << std::setw(2) << std::right;
    if (packet_stats.ordered)     os << static_cast<int>(packet_stats.channel);
    else                          os << "--";
    os << " SQ:" << std::setw(11) << std::left;
    if (packet_stats.reliable)    os << packet_stats.sequence_number;
    else                          os << "--";

    switch (packet_stats.type) {
    case PacketType::CONNECT_REQUEST:
        os << " CON_REQ";
        break;
    case PacketType::CONNECT_RESPONSE:
        os << " CON_RES";
        break;
    case PacketType::CONNECT_ACKNOWLEDGE:
        os << " CON_ACK";
        break;
    case PacketType::CONNECT_COMPLETE:
        os << " CON_COM";
        break;
    case PacketType::PROTO_ACK:
        os << " ACK(" << static_cast<int>(packet_stats.channel) << ":" << packet_stats.sequence_number << ")";
        break;
    case PacketType::PROTO_PING:
        os << " PING";
        break;
    case PacketType::PROTO_PONG:
        os << " PONG";
        break;
    case PacketType::USER_DATA:
        os << " DATA";
        break;
    default:
        break;
    }

    if (packet_stats.reliable && packet_stats.send_queued)
        os << " (rto: " << packet_stats.rto << " send_count: " << packet_stats.send_count << ")";

    os << " CWND:" << peer_stats.congestion_window << " BOW: " << peer_stats.bytes_on_wire;

    return os.str();
}

PacketLoggerConsole::~PacketLoggerConsole()
{
}

void PacketLoggerConsole::on_send(uint64_t ts, const PacketStats& packet_stats, const PeerStats& peer_stats)
{
    std::cout << ts << " SEND>>>  " << debug_string(packet_stats, peer_stats) << std::endl;
}

void PacketLoggerConsole::on_recv(uint64_t ts, const PacketStats& packet_stats, const PeerStats& peer_stats)
{
    std::cout << ts << " RECV<<<  " << debug_string(packet_stats, peer_stats) << std::endl;
}

PacketLoggerCSV::PacketLoggerCSV()
{
    m_file.open("log.csv");
}

PacketLoggerCSV::~PacketLoggerCSV()
{
}

void PacketLoggerCSV::on_send(uint64_t ts, const PacketStats& packet_stats, const PeerStats& peer_stats)
{
    if (!m_file.is_open())
        return;
    m_file << ts << " SEND>>>  " << debug_string(packet_stats, peer_stats) << std::endl;
}

void PacketLoggerCSV::on_recv(uint64_t ts, const PacketStats& packet_stats, const PeerStats& peer_stats)
{
    if (!m_file.is_open())
        return;
    m_file << ts << " RECV<<<  " << debug_string(packet_stats, peer_stats) << std::endl;
}

} // namespace chatter
