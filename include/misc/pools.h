//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/** Provide pool implementations for various wrapper objects.
 *
 *  Each pool is implemented in a layered manner which greatly simplifies the process of adding
 *  new pool types:
 *
 *  1. At the lowest level, we have a pool worker. This is an implementation of the IPoolWorker interface
 *     which implements handlers for such events as "new pool item is needed", "a pool item needs to be
 *     resetted", etc.
 *  2. Next, we have a generic pool class called Pool which provides manipulation mechanisms. Things like
 *     a generic getter and tear-down executor are handled at this level.
 *  3. Finally, at the top we have specialized classes which inherit from Pool. At instantiation time,
 *     they initialize a worker's instance and pass it down to the middle layer.
 *
 *  Pools are NOT thread-safe at the moment. This will be added in the near future.
 */
#ifndef WRAPPERS_POOLS_H
#define WRAPPERS_POOLS_H

#include "misc/types.h"
#include <forward_list>


namespace Anvil
{
    /** A generic pool item interface which provides life-time control & reset facilities
     *  to the pool.
     **/
    template<class PoolItem>
    class IPoolWorker
    {
    public:
        /* Stub destructor */
        virtual ~IPoolWorker()
        {
            /* Stub */
        }

        virtual PoolItem* create_item ()                   = 0;
        virtual void      release_item(PoolItem* item_ptr) = 0;
        virtual void      reset_item  (PoolItem* item_ptr) = 0;
    };

    /** Generic pool implementation  */
    template <class PoolItem>
    class GenericPool
    {
    public:
        /* Public functions */

        /** Constructor
         *
         *  NOTE: The constructor takes over ownership of @param in_worker_ptr. The worker will
         *        be deleted at pool tear-down time.
         *
         *  @param in_n_items_to_preallocate Number of pool items to preallocate.
         *  @param in_worker_ptr             Pointer to the pool item worker implementation.
         *                                   Must not be nullptr. Also see the note above.
         *
         **/
        GenericPool(uint32_t               in_n_items_to_preallocate,
                    IPoolWorker<PoolItem>* in_worker_ptr)
            :m_capacity  (in_n_items_to_preallocate),
             m_worker_ptr(in_worker_ptr)
        {
            for (uint32_t n_item = 0;
                          n_item < in_n_items_to_preallocate;
                        ++n_item)
            {
                PoolItem* new_item_ptr = m_worker_ptr->create_item();

                m_pool_items.push_front(new_item_ptr);
            }
        }

        /** Destructor.
         *
         *  Releases the worker provided at creation time, as well as releases all pool items
         *  currently stored in the pool.
         *
         *  NOTE: Items not stored in the pool at call time will *leak*.
         **/
        virtual ~GenericPool()
        {
            PoolItem* current_item_ptr = nullptr;

            while (!m_pool_items.empty())
            {
                PoolItem* current_item_ptr = m_pool_items.front();

                m_worker_ptr->release_item(current_item_ptr);

                m_pool_items.pop_front();
            }

            if (m_worker_ptr != nullptr)
            {
                delete m_worker_ptr;

                m_worker_ptr = nullptr;
            }
        }

        /** Returns pool capacity */
        uint32_t get_capacity() const
        {
            return m_capacity;
        }

        /** Returns a pool item instance.
         *
         *  If no items are currently available in the pool, a new instance will be created. Otherwise,
         *  an existing pool item will be popped & returned from the pool.
         *
         *  Once returned, the item MUST be returned to the pool when no longer needed. Otherwise, the
         *  instance will leak.
         *
         *  Callers must NOT release the retrieved instances.
         *
         *  @return As per description. */
        PoolItem* get_item()
        {
            PoolItem* result_ptr = nullptr;

            if (!m_pool_items.empty())
            {
                result_ptr = m_pool_items.front();

                m_pool_items.pop_front();
            }
            else
            {
                result_ptr = m_worker_ptr->create_item();
            }

            m_worker_ptr->reset_item(result_ptr);

            return result_ptr;
        }

        /** Stores the provided instance back in the pool. */
        void return_item(PoolItem* item_ptr)
        {
            m_pool_items.push_front(item_ptr);
        }

    protected:
        /* Protected type declarations */
        typedef std::forward_list<PoolItem*> PoolItems;

        /* Protected functions */

        /** Retrieves the underlying pool worker instance */
        const IPoolWorker<PoolItem>* get_worker_ptr() const
        {
            return m_worker_ptr;
        }

        /* Protected variables */
        PoolItems m_pool_items;

    private:
        /* Private variables */
        uint32_t               m_capacity;
        IPoolWorker<PoolItem>* m_worker_ptr;
    };


    /** Implements IPoolWorker interface for primary command buffers. */
    class PrimaryCommandBufferPoolWorker : public IPoolWorker<PrimaryCommandBuffer>
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  Retains @param parent_command_pool_ptr.
         *
         *  @param parent_command_pool_ptr Command pool instance, from which command buffers
         *                                 should be spawned. Must not be nullptr.
         **/
        PrimaryCommandBufferPoolWorker(Anvil::CommandPool* parent_command_pool_ptr);

        /** Destructor.
         *
         *  Releases the encapsulated parent command pool instance.
         **/
        virtual ~PrimaryCommandBufferPoolWorker();

        /** Creates a new primary command buffer instance. The command buffer
         *  is taken from the pool specified at worker instantiation time.
         **/
        PrimaryCommandBuffer* create_item();

        /** Resets contents of the specified command buffer.
         *
         *  @param item_ptr Command buffer to reset. Must not be nullptr.
         **/
        void reset_item(PrimaryCommandBuffer* item_ptr);

        /** Releases the specified command buffer.
         *
         *  NOTE: Command buffers are ref-counted which implies the object does not necessarily
         *        have to be physically destroyed by this function.
         **/
        void release_item(PrimaryCommandBuffer* item_ptr);

    private:
        /* Private functions */
        PrimaryCommandBufferPoolWorker(const PrimaryCommandBufferPoolWorker&);
        bool operator=                (const PrimaryCommandBufferPoolWorker&);

        /* Private variables */
        Anvil::CommandPool* m_parent_command_pool_ptr;
    };

    /** Implements a primary command buffer pool */
    class PrimaryCommandBufferPool : public GenericPool<PrimaryCommandBuffer>
    {
    public:
        /** Constructor
         *
         *  @param parent_command_pool_ptr Command pool instance, from which command buffers
         *                                 should be spawned. Must not be nullptr.
         *  @param n_preallocated_items    Number of command buffers to preallocate at creation time.
         *
         **/
        PrimaryCommandBufferPool(Anvil::CommandPool* parent_command_pool_ptr,
                                 uint32_t            n_preallocated_items);

        /** Stub destructor */
        virtual ~PrimaryCommandBufferPool()
        {
            /* Stub */
        }
    };

    /** A functor which returns an object back to the pool.
     *
     *  Useful if you need to wrap an object in an auto pointer, retrieved from a command buffer pool.
     *  By using the functor, you can have it automatically returned to the pool whenever the pointer
     *  gets out of scope.
     *
     **/
    template<class Pool,
             class PoolItem>
    struct ReturnToPoolFunctor
    {
        /** Constructor.
         *
         *  @param in_pool_ptr Pointer to the command buffer pool, to which the command buffer
         *                     should be returned when the auto pointer goes out of scope. Must
         *                     not be nullptr.
         **/
        ReturnToPoolFunctor(Pool* in_pool_ptr)
        {
            pool_ptr = in_pool_ptr;
        }

        void operator()(PoolItem* command_buffer_ptr)
        {
            pool_ptr->return_item(command_buffer_ptr);
        }

    private:
        Pool* pool_ptr;
    };

    typedef ReturnToPoolFunctor<PrimaryCommandBufferPool, PrimaryCommandBuffer> ReturnPrimaryCommandBufferToPoolFunctor;

}; /* namespace Anvil */

#endif /* WRAPPERS_POOLS_H */