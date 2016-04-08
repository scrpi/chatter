#include "chatter/ipaddress.h"

#include "chatter/platform.h"

namespace chatter {

IPAddress::IPAddress()
    : m_address(0)
{
}

IPAddress::IPAddress(uint32_t address)
    : m_address(address)
{
}

IPAddress::IPAddress(const std::string& address)
{
    platform::HostAddressStringToNet32(address, &m_address);
}

IPAddress::IPAddress(const char* address)
{
    platform::HostAddressStringToNet32(address, &m_address);
}

IPAddress::IPAddress(uint8_t byte0, uint8_t byte1, uint8_t byte2, uint8_t byte3)
{
    m_address = (platform::HostToNet32((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3));
}

std::string IPAddress::to_string() const
{
    return platform::Net32ToString(m_address);
}

uint32_t IPAddress::get_address_n32() const
{
    return m_address;
}

} // namespace chatter
