#include <mimalloc.h>

#include <so_5/all.hpp>

#include <memory_resource>
#include <stdexcept>
#include <iostream>

namespace mimalloc_case
{

class memory_pool_t final : public std::pmr::memory_resource
{
    /// Heap to be used.
    mi_heap_t * _heap;

public:
    /// Main constructor.
    ///
    /// Creates a new mimalloc heap.
    ///
    /// Throws an exception on failure.
    memory_pool_t();
    ~memory_pool_t();

    memory_pool_t(const memory_pool_t &) = delete;
    memory_pool_t &
    operator=(const memory_pool_t &) = delete;

    memory_pool_t(memory_pool_t &&) = delete;
    memory_pool_t &
    operator=(memory_pool_t &&) = delete;

protected:
    void *
    do_allocate(std::size_t bytes, std::size_t alignment) override;

    void
    do_deallocate(void * p, std::size_t bytes, std::size_t alignment) override;

    bool
    do_is_equal(const std::pmr::memory_resource & other) const noexcept override;
};

memory_pool_t::memory_pool_t()
    : _heap{ mi_heap_new() }
{
    if(!_heap)
        throw std::runtime_error{ "mi_heap_new() returns NULL" };
}

memory_pool_t::~memory_pool_t()
{
    std::cout << "=== before mi_heap_destroy ===" << std::endl;
    mi_heap_destroy(_heap);
    std::cout << "=== after mi_heap_destroy ===" << std::endl;
}

void *
memory_pool_t::do_allocate(std::size_t bytes, std::size_t alignment)
{
    void * r = mi_heap_malloc_aligned(_heap, bytes, alignment);
    if(!r)
        throw std::bad_alloc();
    return r;
}

void
memory_pool_t::do_deallocate(
    void * p,
    std::size_t /*bytes*/,
    std::size_t /*alignment*/)
{
    mi_free(p);
}

bool
memory_pool_t::do_is_equal(
    const std::pmr::memory_resource & /*other*/) const noexcept
{
    return false;
}

struct next_loop_t final : public so_5::message_t
{
    memory_pool_t * _pool_to_use;

    next_loop_t(memory_pool_t * pool_to_use) : _pool_to_use{ pool_to_use }
    {}
};

struct loop_completed_t final : public so_5::signal_t {};

} /* namespace mimalloc_case */

int main()
{
    using namespace mimalloc_case;

    // SObjectizer to be used.
    so_5::wrapped_env_t sobj;

    // Variables for interaction with child thread.
    so_5::mchain_t pool_chain = so_5::create_mchain(sobj);
    so_5::mchain_t completion_chain = so_5::create_mchain(sobj);

    // The child thread.
    std::thread child_thread{
        [&]() {
            // The main loop of child thread.
            so_5::receive(
                // Handle all incoming messages until the chain will be closed.
                so_5::from(pool_chain).handle_all(),
                // Reaction to next_loop command.
                [&completion_chain](const next_loop_t & cmd)
                {
                    // Allocations/deallocations.
                    for(int i = 0; i < 1000; ++i)
                    {
                        cmd._pool_to_use->deallocate(
                                cmd._pool_to_use->allocate(16, 8),
                                16, 8);
                    }

                    std::cout << "allocations/deallocations completed"
                            << std::endl;

                    // Inform the main thread that another loop iteration
                    // completed.
                    so_5::send<loop_completed_t>(completion_chain);
                });
        }
    };
    // Join the child thread on exit.
    auto thread_joiner = so_5::auto_join(child_thread);

    // All chains has to be closed on exit.
    auto chain_closer = so_5::auto_close_drop_content(
            pool_chain,
            completion_chain);

    // Local function for doing a loop iteration in the child thread.
    const auto iteration = [&]() {
        // Temporary pool to be used.
        memory_pool_t pool;

        // Sending the data to the child thread.
        so_5::send<next_loop_t>(pool_chain, &pool);

        // Wait for the completion signal.
        so_5::receive(
            so_5::from(completion_chain).handle_n(1),
            [](so_5::mhood_t<loop_completed_t>) {});

        std::cout << "iteration completed" << std::endl;
    };

    // Some iterations...
    for(int i = 0; i < 100; ++i)
    {
        std::cout << "Iteration #" << (i+1) << std::endl;
        iteration();
    }

    std::cout << "Test completed" << std::endl;
}

// vim:expandtab:ts=4:sw=4:sts=4:
