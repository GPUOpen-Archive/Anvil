//
// Copyright (c) 2017-2018 Advanced Micro Devices, Inc. All rights reserved.
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
#include "misc/buffer_create_info.h"
#include "misc/dummy_window.h"
#include "misc/image_create_info.h"
#include "misc/io.h"
#include "misc/swapchain_create_info.h"
#include "wrappers/buffer.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/semaphore.h"
#include "wrappers/swapchain.h"
#include <sstream>
#include <thread>

#include "miniz/miniz.c"

/** Please see header for specification */
Anvil::WindowUniquePtr Anvil::DummyWindow::create(const std::string&      in_title,
                                                  unsigned int            in_width,
                                                  unsigned int            in_height,
                                                  PresentCallbackFunction in_present_callback_func)
{
    WindowUniquePtr result_ptr(nullptr,
                               std::default_delete<Anvil::Window>() );

    result_ptr.reset(
        new Anvil::DummyWindow(in_title,
                               in_width,
                               in_height,
                               in_present_callback_func)
    );

    if (result_ptr)
    {
        if (!dynamic_cast<DummyWindow*>(result_ptr.get() )->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/** Please see header for specification */
Anvil::DummyWindow::DummyWindow(const std::string&      in_title,
                                unsigned int            in_width,
                                unsigned int            in_height,
                                PresentCallbackFunction in_present_callback_func)
    :Window(in_title,
            in_width,
            in_height,
            false, /* in_closable */
            in_present_callback_func)
{
    m_window_owned = true;
}

/** Please see header for specification */
void Anvil::DummyWindow::close()
{
    if (!m_window_should_close)
    {
        m_window_should_close = true;
    }
}

/** Creates a new system window and prepares it for usage. */
bool Anvil::DummyWindow::init()
{
    return true;
}

/* Please see header for specification */
void Anvil::DummyWindow::run()
{
    if (m_present_callback_func)
    {
        bool running = true;

        while (running && !m_window_should_close)
        {
            m_present_callback_func();

            running = !m_window_should_close;
        }
    }
    else
    {
        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(30) );
        }
        while (!m_window_should_close);
    }

    m_window_close_finished = true;
}


/** Please see header for specification */
Anvil::WindowUniquePtr Anvil::DummyWindowWithPNGSnapshots::create(const std::string&      in_title,
                                                                  unsigned int            in_width,
                                                                  unsigned int            in_height,
                                                                  PresentCallbackFunction in_present_callback_func)
{
    WindowUniquePtr result_ptr(nullptr,
                               std::default_delete<Anvil::Window>() );

    result_ptr.reset(
        new Anvil::DummyWindowWithPNGSnapshots(in_title,
                                               in_width,
                                               in_height,
                                               in_present_callback_func)
    );

    if (result_ptr)
    {
        if (!dynamic_cast<DummyWindowWithPNGSnapshots*>(result_ptr.get() )->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

/** Please see header for specification */
Anvil::DummyWindowWithPNGSnapshots::DummyWindowWithPNGSnapshots(const std::string&      in_title,
                                                                unsigned int            in_width,
                                                                unsigned int            in_height,
                                                                PresentCallbackFunction in_present_callback_func)
    :DummyWindow(in_title,
                 in_width,
                 in_height,
                 in_present_callback_func)
{
    m_height             = in_height;
    m_n_frames_presented = 0;
    m_swapchain_ptr      = nullptr;
    m_title              = in_title;
    m_width              = in_width;
    m_window_owned       = true;
}

/** Please see header for specification */
std::unique_ptr<uint8_t[]> Anvil::DummyWindowWithPNGSnapshots::get_swapchain_image_raw_r8g8b8a8_unorm_data(Anvil::Image* in_swapchain_image_ptr)
{
    const Anvil::BaseDevice*       device_ptr                       (m_swapchain_ptr->get_create_info_ptr()->get_device() );
    std::unique_ptr<uint8_t[]>     result_ptr;
    VkFormat                       swapchain_image_format           (in_swapchain_image_ptr->get_create_info_ptr()->get_format() );
    const VkImageSubresourceRange  swapchain_image_subresource_range(in_swapchain_image_ptr->get_subresource_range            () );

    /* Sanity checks .. */
    ANVIL_REDUNDANT_VARIABLE(swapchain_image_format);

    anvil_assert(swapchain_image_subresource_range.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);

    /* Initialize storage for the raw R8G8B8A8 image data */
    Anvil::BufferUniquePtr raw_image_buffer_ptr;
    uint32_t               raw_image_size         = 0;
    uint32_t               swapchain_image_height = 0;
    uint32_t               swapchain_image_width  = 0;

    in_swapchain_image_ptr->get_image_mipmap_size(0, /* n_mipmap */
                                                 &swapchain_image_width,
                                                 &swapchain_image_height,
                                                 nullptr); /* out_opt_depth_ptr */

    raw_image_size = 4 /* RGBA8 */ * swapchain_image_width * swapchain_image_height;

    result_ptr.reset(new unsigned char[raw_image_size]);

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_nonsparse_alloc(device_ptr,
                                                                               raw_image_size,
                                                                               Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
                                                                               VK_SHARING_MODE_EXCLUSIVE,
                                                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                                               Anvil::MEMORY_FEATURE_FLAG_MAPPABLE);

        create_info_ptr->set_mt_safety(MT_SAFETY_DISABLED);

        raw_image_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr) );
    }

    /* 2. Create the intermediate image */
    VkImageBlit    intermediate_image_blit;
    ImageUniquePtr intermediate_image_ptr;

    {
        auto create_info_ptr = Anvil::ImageCreateInfo::create_nonsparse_alloc(device_ptr,
                                                                              VK_IMAGE_TYPE_2D,
                                                                              VK_FORMAT_R8G8B8A8_UNORM,
                                                                              VK_IMAGE_TILING_OPTIMAL,
                                                                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                                              VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                                              swapchain_image_width,
                                                                              swapchain_image_height,
                                                                              1,      /* in_base_mipmap_depth */
                                                                              1,      /* in_n_layers          */
                                                                              VK_SAMPLE_COUNT_1_BIT,
                                                                              Anvil::QUEUE_FAMILY_GRAPHICS_BIT,
                                                                              VK_SHARING_MODE_EXCLUSIVE,
                                                                              false,  /* in_use_full_mipmap_chain */
                                                                              0,      /* in_memory_features       */
                                                                              0,      /* in_create_flags          */
                                                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        create_info_ptr->set_mt_safety(MT_SAFETY_DISABLED);

        intermediate_image_ptr = Anvil::Image::create(std::move(create_info_ptr) );
    }

    /* 3. Set up the command buffer */
    auto                command_buffer_ptr           = Anvil::PrimaryCommandBufferUniquePtr();
    Anvil::CommandPool* universal_command_pool_ptr   = nullptr;
    Anvil::Queue*       universal_queue_ptr          = device_ptr->get_universal_queue            (0);
    const uint32_t      universal_queue_family_index = universal_queue_ptr->get_queue_family_index();

    universal_command_pool_ptr = device_ptr->get_command_pool_for_queue_family_index(universal_queue_ptr->get_queue_family_index() );
    command_buffer_ptr         = universal_command_pool_ptr->alloc_primary_level_command_buffer();

    command_buffer_ptr->start_recording(true,   /* one_time_submit          */
                                        false); /* simultaneous_use_allowed */
    {
        VkBufferImageCopy   buffer_image_copy_region;

        Anvil::ImageBarrier general_to_transfer_src_image_barrier(
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT, /* source_access_mask      */
            VK_ACCESS_TRANSFER_READ_BIT,                                                                     /* destination_access_mask */
            false,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            universal_queue_family_index,
            universal_queue_family_index,
            in_swapchain_image_ptr,
            swapchain_image_subresource_range
        );

        Anvil::ImageBarrier transfer_dst_to_transfer_src_image_barrier(
            VK_ACCESS_TRANSFER_WRITE_BIT, /* source_access_mask      */
            VK_ACCESS_TRANSFER_READ_BIT,  /* desitnation_access_mask */
            false,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            universal_queue_family_index,
            universal_queue_family_index,
            intermediate_image_ptr.get(),
            swapchain_image_subresource_range);

        Anvil::ImageBarrier transfer_src_to_general_image_barrier(
            VK_ACCESS_TRANSFER_READ_BIT, /* source_access_mask      */
            0,                           /* destination_access_mask */
            false,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            universal_queue_family_index,
            universal_queue_family_index,
            in_swapchain_image_ptr,
            swapchain_image_subresource_range);

        command_buffer_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, /* src_stage_mask                 */
                                                    VK_PIPELINE_STAGE_TRANSFER_BIT,                                                 /* dst_stage_mask                 */
                                                    false,                                                                          /* in_by_region                   */
                                                    0,                                                                              /* in_memory_barrier_count        */
                                                    nullptr,                                                                        /* in_memory_barrier_ptrs         */
                                                    0,                                                                              /* in_buffer_memory_barrier_count */
                                                    nullptr,                                                                        /* in_buffer_memory_barrier_ptrs  */
                                                    1,                                                                              /* in_image_memory_barrier_count  */
                                                   &general_to_transfer_src_image_barrier);

        intermediate_image_blit.dstOffsets[0].x               = 0;
        intermediate_image_blit.dstOffsets[0].y               = 0;
        intermediate_image_blit.dstOffsets[0].z               = 0;
        intermediate_image_blit.dstOffsets[1].x               = static_cast<int32_t>(swapchain_image_width);
        intermediate_image_blit.dstOffsets[1].y               = static_cast<int32_t>(swapchain_image_height);
        intermediate_image_blit.dstOffsets[1].z               = 1;
        intermediate_image_blit.dstSubresource.baseArrayLayer = 0;
        intermediate_image_blit.dstSubresource.layerCount     = 1;
        intermediate_image_blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        intermediate_image_blit.dstSubresource.mipLevel       = 0;
        intermediate_image_blit.srcOffsets[0]                 = intermediate_image_blit.dstOffsets[0];
        intermediate_image_blit.srcOffsets[1]                 = intermediate_image_blit.dstOffsets[1];
        intermediate_image_blit.srcSubresource                = intermediate_image_blit.dstSubresource;

        command_buffer_ptr->record_blit_image(in_swapchain_image_ptr,
                                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                              intermediate_image_ptr.get(),
                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                              1, /* regionCount */
                                             &intermediate_image_blit,
                                              VK_FILTER_NEAREST);

        command_buffer_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, /* src_stage_mask */
                                                    VK_PIPELINE_STAGE_TRANSFER_BIT, /* dst_stage_mask */
                                                    VK_FALSE,       /* in_by_region                   */
                                                    0,              /* in_memory_barrier_count        */
                                                    nullptr,        /* in_memory_barrier_ptrs         */
                                                    0,              /* in_buffer_memory_barrier_count */
                                                    nullptr,        /* in_buffer_memory_barrier_ptrs  */
                                                    1,              /* in_image_memory_barrier_count  */
                                                   &transfer_dst_to_transfer_src_image_barrier);

        buffer_image_copy_region.bufferImageHeight               = 0; /* assume tight packing */
        buffer_image_copy_region.bufferOffset                    = 0;
        buffer_image_copy_region.bufferRowLength                 = 0; /* assume tight packing */
        buffer_image_copy_region.imageExtent.depth               = 1;
        buffer_image_copy_region.imageExtent.height              = swapchain_image_height;
        buffer_image_copy_region.imageExtent.width               = swapchain_image_width;
        buffer_image_copy_region.imageOffset.x                   = 0;
        buffer_image_copy_region.imageOffset.y                   = 0;
        buffer_image_copy_region.imageOffset.z                   = 0;
        buffer_image_copy_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        buffer_image_copy_region.imageSubresource.baseArrayLayer = 0;
        buffer_image_copy_region.imageSubresource.layerCount     = 1;
        buffer_image_copy_region.imageSubresource.mipLevel       = 0;

        command_buffer_ptr->record_copy_image_to_buffer(intermediate_image_ptr.get(),
                                                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                        raw_image_buffer_ptr.get(),
                                                        1, /* regionCount */
                                                       &buffer_image_copy_region);

        command_buffer_ptr->record_pipeline_barrier(VK_PIPELINE_STAGE_TRANSFER_BIT,    /* src_stage_mask */
                                                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, /* dst_stage_mask */
                                                    VK_FALSE,       /* in_by_region                   */
                                                    0,              /* in_memory_barrier_count        */
                                                    nullptr,        /* in_memory_barrier_ptrs         */
                                                    0,              /* in_buffer_memory_barrier_count */
                                                    nullptr,        /* in_buffer_memory_barrier_ptrs  */
                                                    1,              /* in_image_memory_barrier_count  */
                                                   &transfer_src_to_general_image_barrier);
    }
    command_buffer_ptr->stop_recording();

    /* Execute the command buffer */
    {
        Anvil::CommandBufferBase* cmd_buffer_raw_ptr = command_buffer_ptr.get();

        universal_queue_ptr->submit(Anvil::SubmitInfo::create_execute(&cmd_buffer_raw_ptr,
                                                                      1,    /* in_n_cmd_buffers */
                                                                      true) /* should_block     */
        );
    }

    /* Read back the result data */
    raw_image_buffer_ptr->read(0, /* offset */
                               raw_image_size,
                               result_ptr.get() );

    return result_ptr;
}

/** Please see header for specification */
void Anvil::DummyWindowWithPNGSnapshots::run()
{
    bool running = true;

    while (running && !m_window_should_close)
    {
        m_present_callback_func();

        store_swapchain_frame();

        running = !m_window_should_close;
    }

    m_window_close_finished = true;
}

/** Please see header for specification */
void Anvil::DummyWindowWithPNGSnapshots::set_swapchain(Anvil::Swapchain* in_swapchain_ptr)
{
    anvil_assert(m_swapchain_ptr == nullptr);

    m_swapchain_ptr = in_swapchain_ptr;
}

/** Please see header for specification */
void Anvil::DummyWindowWithPNGSnapshots::store_swapchain_frame()
{
    anvil_assert(m_swapchain_ptr != nullptr);

    /* Retrieve image contents */
    const uint32_t swapchain_image_index        = m_swapchain_ptr->get_last_acquired_image_index();
    Anvil::Image*  swapchain_image_ptr          = m_swapchain_ptr->get_image                    (swapchain_image_index);
    auto           swapchain_image_raw_data_ptr = get_swapchain_image_raw_r8g8b8a8_unorm_data   (swapchain_image_ptr);
    uint32_t       swapchain_image_size[2];

    swapchain_image_ptr->get_image_mipmap_size(0, /* n_mipmap */
                                               swapchain_image_size + 0,
                                               swapchain_image_size + 1,
                                               nullptr); /* out_opt_depth_ptr */

    /* Determine what name should be used for the snapshot file */
    std::stringstream snapshot_file_name_sstream;

    snapshot_file_name_sstream << m_title
                               << "_"
                               << m_n_frames_presented++
                               << ".png";

    /* Convert the retrieved data to a PNG blob */
    void*  result_data_ptr  = nullptr;
    size_t result_data_size = 0;

    result_data_ptr = tdefl_write_image_to_png_file_in_memory(swapchain_image_raw_data_ptr.get(),
                                                              static_cast<int32_t>(swapchain_image_size[0]),
                                                              static_cast<int32_t>(swapchain_image_size[1]),
                                                              4, /* num_chans */
                                                             &result_data_size);

    anvil_assert(result_data_ptr != nullptr);

    /* Store it in a file */
    Anvil::IO::write_binary_file(snapshot_file_name_sstream.str(),
                                 result_data_ptr,
                                 static_cast<uint32_t>(result_data_size) );

    /* Clean up */
    free(result_data_ptr);
}