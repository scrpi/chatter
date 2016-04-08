#ifndef _CH_HOSTADDRESS_H_
#define _CH_HOSTADDRESS_H_

#include <cstdint>
#include <string>

namespace chatter {

class Peer;

class HostAddress
{
public:
    HostAddress();
    HostAddress(uint32_t address, uint16_t port);
    HostAddress(const std::string& address, uint16_t port);
    HostAddress(const char* address, uint16_t port);

    std::string to_string() const;
    uint32_t address() const;
    uint16_t port() const;

    inline bool operator ==(const HostAddress& other)
    {
        return (m_address == other.address() && m_port == other.port());
    }

private:
    friend class Host;

    uint32_t m_address;
    uint16_t m_port;
};

} // namespace chatter

#endif // _CH_HOSTADDRESS_H_
