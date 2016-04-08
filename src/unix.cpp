#include "chatter/platform.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace chatter {
namespace platform {

Socket SocketCreate()
{
    return socket(AF_INET, SOCK_DGRAM, 0);
}

void SocketDestroy(Socket socket)
{
    if (socket != CH_SOCKET_NULL)
        close(socket);
}

bool SocketSetOption(Socket socket, SocketOption option, int value)
{
    int result;
    struct timeval timeVal;

    switch (option) {
        case SocketOption::NONBLOCK:
            result = ioctl(socket, FIONBIO, &value);
            break;

        case SocketOption::BROADCAST:
            result = setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char*)&value, sizeof(int));
            break;

        case SocketOption::REUSEADDR:
            result = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(int));
            break;

        case SocketOption::RCVBUF:
            result = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char*)&value, sizeof(int));
            break;

        case SocketOption::SNDBUF:
            result = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)&value, sizeof(int));
            break;

        case SocketOption::RCVTIMEO:
            timeVal.tv_sec = value / 1000;
            timeVal.tv_usec = (value % 1000) * 1000;
            result = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeVal, sizeof(struct timeval));
            break;

        case SocketOption::SNDTIMEO:
            timeVal.tv_sec = value / 1000;
            timeVal.tv_usec = (value % 1000) * 1000;
            result = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeVal, sizeof(struct timeval));
            break;

#if 0
        case SocketOption::NODELAY:
            result = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(int));
            break;
#endif
    }
    return result == 0 ? true : false;
}

bool SocketBind(Socket socket, const HostAddress& address)
{
    struct sockaddr_in sin;
    int rc;

    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_port = HostToNet16(address.port());
    sin.sin_addr.s_addr = address.address();

    rc = bind(socket, (struct sockaddr *)&sin, sizeof(sin));
    return rc == 0 ? true : false;
}

ssize_t SocketSendTo(Socket socket, const void *buf, size_t buf_len, const HostAddress& address)
{
    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_port = HostToNet16(address.port());
    dest.sin_addr.s_addr = address.address();
    return sendto((int)socket, buf, buf_len, 0, (struct sockaddr*)&dest, sizeof(dest));
}

ssize_t SocketRecvFrom(Socket socket, void *buf, size_t buf_len, HostAddress* address)
{
    struct sockaddr_in src = {0};
    socklen_t src_len = sizeof(src);
    ssize_t ret = recvfrom((int)socket, buf, buf_len, 0, (struct sockaddr*)&src, &src_len);
    *address = HostAddress(src.sin_addr.s_addr, NetToHost16(src.sin_port));
    return ret;
}

bool HostAddressStringToNet32(const std::string address, uint32_t* out)
{
    if (!out)
        return false;

    if (address == "") {
        *out = INADDR_ANY;
        return true;
    }

    struct in_addr addr;
    if (inet_aton(address.c_str(), &addr) != 0) {
        *out = addr.s_addr;
        return true;
    }

    return false;
}

std::string Net32ToString(uint32_t address_net)
{
    in_addr address;
    address.s_addr = address_net;

    return inet_ntoa(address);
}

uint16_t HostToNet16(uint16_t host)
{
    return htons(host);
}

uint16_t NetToHost16(uint16_t net)
{
    return ntohs(net);
}

uint32_t HostToNet32(uint32_t host)
{
    return htonl(host);
}

uint32_t NetToHost32(uint32_t net)
{
    return ntohl(net);
}

uint64_t HostToNet64(const uint64_t& host)
{
    int x = 1;
    if (*(char*)&x == x)
        /* little endian */
        return ((((uint64_t)htonl(host)) << 32) + htonl(host >> 32));

    else
        return host;
}

uint64_t NetToHost64(const uint64_t& net)
{
    int x = 1;
    if (*(char*)&x == x)
        /* little endian */
        return ((((uint64_t)ntohl(net)) << 32) + ntohl(net >> 32));

    else
        return net;
}

} // namespace platform
} // namespace chatter
