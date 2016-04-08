#ifndef _CH_IPADDRESS_H_
#define _CH_IPADDRESS_H_

#include <cstdint>
#include <string>

namespace chatter {

class IPAddress
{
public:
    IPAddress();
    IPAddress(uint32_t address);
    IPAddress(const std::string& address);
    IPAddress(const char* address);
    IPAddress(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3);

    std::string to_string() const;
    uint32_t get_address_n32() const;

    bool operator ==(const IPAddress& other)
    {
        return m_address == other.get_address_n32();
    }

private:
    uint32_t m_address;
};

} // namespace chatter

#endif // _CH_IPADDRESS_H_
