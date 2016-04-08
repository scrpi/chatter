#ifndef _CH_PACKETPOOL_H_
#define _CH_PACKETPOOL_H_

namespace chatter {

struct PoolChunk
{

}

template <typename T, size_t chunk_size = 128>
class PacketPool
{
public:
    typedef T value_type;
    typedef T* pointer;
    typedef T& reference;
    typedef const T* const_pointer;
    typedef const T& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::false_type propagate_on_container_copy_assignment;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::true_type propagate_on_container_swap;

    template <typename U> struct rebind
    {
        typedef PacketPool<U> other;
    };

    PacketPool() noexcept;
    PacketPool(const PacketPool& packet_pool) noexcept;
    PacketPool(PacketPool&& packet_pool) noexcept;
    template <class U> PacketPool(const PacketPool<U>& packet_pool) noexcept;
    ~PacketPool() noexcept;

    PacketPool& operator=(const PacketPool& packet_pool) = delete;
    PacketPool& operator=(PacketPool&& packet_pool) noexcept;

    pointer address(reference t) const noexcept;
    const_pointer address(const_reference t) const noexcept;
    pointer allocate(size_type n = 1, const_pointer hint = nullptr);
    void deallocate(pointer p, size_type n = 1);
    size_type max_size() const noexcept;

    template <class U, class... Args> void construct(U* p, Args&&... args);
    template <class U> void destroy(U* p);
};


} // namespace chatter

#endif //  _CH_PACKETPOOL_H_
