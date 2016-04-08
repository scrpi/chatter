#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

#include <chatter/chatter.h>

int main(int argc, char* argv[])
{
    chatter::Host host;
    bool is_client = false;

    chatter::PacketLoggerCSV logger;
    host.register_packet_listener(&logger);

    if (argc >= 2) {
        uint16_t port = 9876;
        if (argc > 2)
            port = atoi(argv[2]);

        chatter::HostAddress client("", 0);
        host.start(client, 1);

        chatter::HostAddress server(argv[1], port);
        host.connect(server);

        is_client = true;
    }
    else {
        chatter::HostAddress server("", 9876);
        host.start(server, 32);
    }

    int i = 0;
    chatter::HostAddress host_address;

    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (is_client && i >= 0 && host_address.address() != 0) {
            int flags = 0;
            flags |= chatter::PacketFlag::RELIABLE;
            flags |= chatter::PacketFlag::ORDERED;
            flags |= chatter::PacketFlag::SEQUENCED;
            flags |= chatter::PacketFlag::TIMESTAMPED;
            auto p = chatter::Packet::create(flags, 2);
            int64_t x = 123;
            for (int j = 0; j < 60; ++j)
                p->write(x);
            host.send(host_address, p);
            i = 0;
        }

        chatter::Event::ptr e;
        while (e = host.get_event()) {
            std::cout << "Received event: " << e->to_string() << std::endl;
            switch (e->type) {
            case chatter::EventType::PEER_CONNECTED:
                host_address = e->address;
                break;

            case chatter::EventType::PACKET_RECEIVED:
                int64_t x;
                e->packet->read(x);
                if (!is_client) {
                    int flags;
                    flags |= chatter::PacketFlag::RELIABLE;
                    flags |= chatter::PacketFlag::ORDERED;
                    flags |= chatter::PacketFlag::SEQUENCED;
                    flags |= chatter::PacketFlag::TIMESTAMPED;
                    auto p = chatter::Packet::create(flags, 2);
                    p->write(x);
                    host.send(e->address, p);
                }
                break;

            default:
                break;
            }
        }

        i++;
    }

    host.shutdown();

    return 0;
}
