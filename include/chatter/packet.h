#ifndef _CH_PACKET_H_
#define _CH_PACKET_H_

#include <memory>
#include <vector>

#include "chatter/types.h"

namespace chatter {

class Peer;

struct PacketStats
{
    PacketType type;
    bool reliable;
    bool ordered;
    bool sequenced;
    bool timestamped;
    uint8_t channel;
    SeqNum sequence_number;
    uint32_t data_len;
    uint16_t rto;
    uint16_t send_count;
    bool send_queued;
};

class Packet
{
public:
    typedef std::shared_ptr<Packet> ptr;

    Packet(int flags = 0, ProtocolChannelID channel = 0);
    static Packet::ptr create(int flags = 0, ProtocolChannelID channel = 0);

    bool has_flag(PacketFlag flag);

    void write(bool     data);
    void write(uint8_t  data);
    void write(int8_t   data);
    void write(uint16_t data);
    void write(int16_t  data);
    void write(uint32_t data);
    void write(int32_t  data);
    void write(uint64_t data);
    void write(int64_t  data);
    void write(float    data);
    void write(double   data);

    void read(bool&     data);
    void read(uint8_t&  data);
    void read(int8_t&   data);
    void read(uint16_t& data);
    void read(int16_t&  data);
    void read(uint32_t& data);
    void read(int32_t&  data);
    void read(uint64_t& data);
    void read(int64_t&  data);
    void read(float&    data);
    void read(double&   data);

    const uint8_t* data() const { return &m_data[0]; }
    const std::size_t data_len() const { return m_data.size(); }
    bool has_more_data() const { return m_read_pos < m_data.size(); }

private:
    friend class Peer;
    friend class Protocol;
    friend class Host;

    void              set_type(PacketType type);
    PacketType        get_type();
    bool              is_type(PacketType type);

    ProtocolChannelID get_channel();
    void              set_channel(ProtocolChannelID chan);
    void              set_flag(PacketFlag flag);
    void              unset_flag(PacketFlag flag);

    std::size_t       read_raw(uint8_t* buf, std::size_t buf_size);
    void              append_bytes(const void* data, std::size_t data_len);
    bool              check_bounds(std::size_t data_len);

    std::string       debug_string();

    ProtocolCommand m_cmd = 0;
    Peer* m_peer; /* either source or destination, depending on whether this packet was received or is being sent. */
    std::vector<uint8_t> m_data;
    std::size_t m_read_pos = 0;

    uint16_t m_rto = 0;             //> Retransmission time-out (set by protocol each send)
    uint16_t m_send_count = 0;      //> Send count (set by host each send)
    uint64_t m_last_send_time = 0;  //> Timestamp of last send of this packet (set by host each send)

    bool m_send_queued = false;     //> Set by protocol when send is queued. Avoids re-queuing packets.

    SeqNum m_sequence_num = 0;

    static const int kPacketTypeShift = 11; /* >> 11 */
    static const int kPacketFlagShift = 5;  /* >> 5 */
    static const int kPacketChanShift = 0;  /* >> 0 */
    static const uint16_t kPacketTypeMask = 0xF800; /* XXXXX........... 5 bits */
    static const uint16_t kPacketFlagMask = 0x07E0; /* .....XXXXXX..... 6 bits */
    static const uint16_t kPacketChanMask = 0x001F; /* ...........XXXXX 5 bits */
};

} // namespace chatter

#endif // _CH_PACKET_H_
