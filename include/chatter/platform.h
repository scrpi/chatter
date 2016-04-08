#ifndef _CH_PLATFORM_H_
#define _CH_PLATFORM_H_

#include <string>
#include <cstdint>
#include <memory>

#include "chatter/hostaddress.h"

#define CH_SOCKET_NULL -1
#define CH_ADDR_ANY INADDR_ANY
#define CH_PORT_ANY 0

namespace chatter {

typedef int Socket;

enum class SocketOption
{
    NONBLOCK,
    BROADCAST,
    REUSEADDR,
    RCVBUF,
    SNDBUF,
    RCVTIMEO,
    SNDTIMEO,
#if 0
    NODELAY
#endif
};

namespace platform {

Socket SocketCreate();
void SocketDestroy(Socket socket);
bool SocketSetOption(Socket socket, SocketOption option, int value);
bool SocketBind(Socket socket, const HostAddress& address);
ssize_t SocketSendTo(Socket socket, const void *buf, size_t buf_len, const HostAddress& address);
ssize_t SocketRecvFrom(Socket socket, void *buf, size_t buf_len, HostAddress* address);

bool HostAddressStringToNet32(const std::string address, uint32_t *out);
std::string Net32ToString(uint32_t address_net);

uint16_t HostToNet16(uint16_t host);
uint16_t NetToHost16(uint16_t net);
uint32_t HostToNet32(uint32_t host);
uint32_t NetToHost32(uint32_t net);
uint64_t HostToNet64(const uint64_t& host);
uint64_t NetToHost64(const uint64_t& net);

} // namespace platform
} // namespace chatter

#endif // _CH_PLATFORM_H_
