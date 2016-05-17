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

#include "misc/debug.h"
#include "misc/pools.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/pipeline_layout.h"

/* Please see header for specification */
Anvil::PrimaryCommandBufferPoolWorker::PrimaryCommandBufferPoolWorker(Anvil::CommandPool* parent_command_pool_ptr)
    :m_parent_command_pool_ptr(parent_command_pool_ptr)
{
    anvil_assert(m_parent_command_pool_ptr != nullptr)

    m_parent_command_pool_ptr->retain();
}

/* Please see header for specification */
Anvil::PrimaryCommandBufferPool::PrimaryCommandBufferPool(Anvil::CommandPool* parent_command_pool_ptr,
                                                           uint32_t             n_preallocated_items)
    :GenericPool(n_preallocated_items,
                 new PrimaryCommandBufferPoolWorker(parent_command_pool_ptr) )
{
    /* Stub */
}

/* Please see header for specification */
Anvil::PrimaryCommandBufferPoolWorker::~PrimaryCommandBufferPoolWorker()
{
    m_parent_command_pool_ptr->release();
}

/* Please see header for specification */
Anvil::PrimaryCommandBuffer* Anvil::PrimaryCommandBufferPoolWorker::create_item()
{
    return m_parent_command_pool_ptr->alloc_primary_level_command_buffer();
}

/* Please see header for specification */
void Anvil::PrimaryCommandBufferPoolWorker::reset_item(PrimaryCommandBuffer* item_ptr)
{
    item_ptr->reset(false /* should_release_resources */);
}

/* Please see header for specification */
void Anvil::PrimaryCommandBufferPoolWorker::release_item(PrimaryCommandBuffer* item_ptr)
{
    item_ptr->release();
}