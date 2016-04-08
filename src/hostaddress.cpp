#include "chatter/hostaddress.h"
#include "chatter/platform.h"

#include <sstream>

namespace chatter
{

HostAddress::HostAddress()
{
    HostAddress(static_cast<uint32_t>(0), static_cast<uint16_t>(0));
}

HostAddress::HostAddress(uint32_t address, uint16_t port)
    : m_address(address), m_port(port)
{}

HostAddress::HostAddress(const std::string& address, uint16_t port)
    : m_port(port)
{
    platform::HostAddressStringToNet32(address, &m_address);
}

HostAddress::HostAddress(const char* address, uint16_t port)
    : m_port(port)
{
    platform::HostAddressStringToNet32(address, &m_address);
}

std::string HostAddress::to_string() const
{
    std::ostringstream os;
    os << platform::Net32ToString(m_address) << ":" << m_port;
    return os.str();
}

uint32_t HostAddress::address() const
{
    return m_address;
}

uint16_t HostAddress::port() const
{
    return m_port;
}

} // namespace chatter
