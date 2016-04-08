#include "chatter/packet.h"

#include <cstring>
#include <sstream>
#include <iostream>
#include "chatter/platform.h"
#include "chatter/peer.h"

namespace chatter {

Packet::Packet(int flags /* = 0 */, ProtocolChannelID channel /* = 0 */)
{
    set_type(PacketType::USER_DATA);
    set_flag(static_cast<PacketFlag>(flags));
    if (channel > 31)
        channel = 31;
    set_channel(channel);
}

Packet::ptr Packet::create(int flags /* = 0 */, ProtocolChannelID channel /* = 0 */)
{
    return std::make_shared<Packet>(flags, channel);
}

bool Packet::has_flag(PacketFlag flag)
{
    return (((m_cmd & kPacketFlagMask) >> kPacketFlagShift) & flag) > 0;
}

ProtocolChannelID Packet::get_channel()
{
    if (has_flag(PacketFlag::ORDERED))
        return static_cast<ProtocolChannelID>((m_cmd & kPacketChanMask) >> kPacketChanShift);
    return kReliableUnorderedChannel;
}

void Packet::set_channel(ProtocolChannelID chan)
{
    if (chan > 31)
        chan = 31;

    /* Zero the field */
    m_cmd &= ~kPacketChanMask;

    /* Set the field */
    m_cmd |= static_cast<uint16_t>(chan) << kPacketChanShift;
}

void Packet::append_bytes(const void* data, std::size_t data_len)
{
    if (!data || data_len <= 0)
        return;

    std::size_t start = m_data.size();
    m_data.resize(start + data_len);
    std::memcpy(&m_data[start], data, data_len);
}

bool Packet::check_bounds(std::size_t data_len)
{
    return m_read_pos + data_len <= m_data.size();
}

void Packet::write(bool data)
{
    uint8_t val = static_cast<uint8_t>(data);
    append_bytes(&val, sizeof(val));
}

void Packet::write(uint8_t data)
{
    append_bytes(&data, sizeof(data));
}

void Packet::write(int8_t data)
{
    append_bytes(&data, sizeof(data));
}

void Packet::write(uint16_t data)
{
    uint16_t net = platform::HostToNet16(data);
    append_bytes(&net, sizeof(net));
}

void Packet::write(int16_t data)
{
    int16_t net = platform::HostToNet16(data);
    append_bytes(&net, sizeof(net));
}

void Packet::write(uint32_t data)
{
    uint32_t net = platform::HostToNet32(data);
    append_bytes(&net, sizeof(net));
}

void Packet::write(int32_t data)
{
    int32_t net = platform::HostToNet32(data);
    append_bytes(&net, sizeof(net));
}

void Packet::write(uint64_t data)
{
    uint64_t net = platform::HostToNet64(data);
    append_bytes(&net, sizeof(net));
}

void Packet::write(int64_t data)
{
    int64_t net = platform::HostToNet64(data);
    append_bytes(&net, sizeof(net));
}

void Packet::write(float data)
{
    append_bytes(&data, sizeof(data));
}

void Packet::write(double data)
{
    append_bytes(&data, sizeof(data));
}

void Packet::read(bool& data)
{
    uint8_t val;
    if (!check_bounds(sizeof(val)))
        return;

    val = *reinterpret_cast<const uint8_t*>(&m_data[m_read_pos]);
    m_read_pos += sizeof(val);
    data = (val == true);
}

void Packet::read(uint8_t& data)
{
    if (!check_bounds(sizeof(data)))
        return;

    data = *reinterpret_cast<const uint8_t*>(&m_data[m_read_pos]);
    m_read_pos += sizeof(data);
}

void Packet::read(int8_t& data)
{
    if (!check_bounds(sizeof(data)))
        return;

    data = *reinterpret_cast<const int8_t*>(&m_data[m_read_pos]);
    m_read_pos += sizeof(data);
}

void Packet::read(uint16_t& data)
{
    if (!check_bounds(sizeof(data)))
        return;

    data = platform::NetToHost16(*reinterpret_cast<const uint16_t*>(&m_data[m_read_pos]));
    m_read_pos += sizeof(data);
}

void Packet::read(int16_t& data)
{
    if (!check_bounds(sizeof(data)))
        return;

    data = platform::NetToHost16(*reinterpret_cast<const int16_t*>(&m_data[m_read_pos]));
    m_read_pos += sizeof(data);
}

void Packet::read(uint32_t& data)
{
    if (!check_bounds(sizeof(data)))
        return;

    data = platform::NetToHost32(*reinterpret_cast<const uint32_t*>(&m_data[m_read_pos]));
    m_read_pos += sizeof(data);
}

void Packet::read(int32_t& data)
{
    if (!check_bounds(sizeof(data)))
        return;

    data = platform::NetToHost32(*reinterpret_cast<const int32_t*>(&m_data[m_read_pos]));
    m_read_pos += sizeof(data);
}

void Packet::read(uint64_t& data)
{
    if (!check_bounds(sizeof(data)))
        return;

    data = platform::NetToHost64(*reinterpret_cast<const uint64_t*>(&m_data[m_read_pos]));
    m_read_pos += sizeof(data);
}

void Packet::read(int64_t& data)
{
    if (!check_bounds(sizeof(data)))
        return;

    data = platform::NetToHost64(*reinterpret_cast<const int64_t*>(&m_data[m_read_pos]));
    m_read_pos += sizeof(data);
}

void Packet::read(float& data)
{
    if (!check_bounds(sizeof(data)))
        return;

    data = *reinterpret_cast<const float*>(&m_data[m_read_pos]);
    m_read_pos += sizeof(data);
}

void Packet::read(double& data)
{
    if (!check_bounds(sizeof(data)))
        return;

    data = *reinterpret_cast<const double*>(&m_data[m_read_pos]);
    m_read_pos += sizeof(data);
}

void Packet::set_type(PacketType type)
{
    /* Zero the field */
    m_cmd &= ~kPacketTypeMask;

    /* Set the field */
    m_cmd |= static_cast<uint16_t>(type) << kPacketTypeShift;
}

PacketType Packet::get_type()
{
    return static_cast<PacketType>((m_cmd & kPacketTypeMask) >> kPacketTypeShift);
}

bool Packet::is_type(PacketType type)
{
    return get_type() == type;
}

void Packet::set_flag(PacketFlag flag)
{
    m_cmd |= (flag << kPacketFlagShift) & kPacketFlagMask;
}

void Packet::unset_flag(PacketFlag flag)
{
    m_cmd &= ~(flag << kPacketFlagShift);
}

std::size_t Packet::read_raw(uint8_t* buf, std::size_t buf_size)
{
    std::size_t write_pos = 0;

    /* Write command */
    ProtocolCommand cmd_n = platform::HostToNet16(m_cmd);
    std::memcpy(&buf[write_pos], &cmd_n, sizeof(cmd_n));
    write_pos += sizeof(cmd_n);

    if (has_flag(PacketFlag::RELIABLE)) {
        /* Write sequence number */
        SeqNum seq_net = platform::HostToNet32(m_sequence_num);
        std::memcpy(&buf[write_pos], &seq_net, sizeof(seq_net));
        write_pos += sizeof(seq_net);
    }

    /* Write data */
    if (m_data.size()) {
        std::memcpy(&buf[write_pos], &m_data[0], m_data.size());
        write_pos += m_data.size();
    }

    return write_pos;
}

} // namespace chatter
