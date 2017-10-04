//
// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
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

/** Defines a command buffer wrapper classes which simplify the following processes:
 *
 *  - Automatic object management (any object referred to from a command automatically
*                that object's reference counter).
 *  - Debugging (commands are stored internally in debug builds for quick investigation;
 *               cumbersome Vulkan descriptor arrays are converted to vectors;
 *               raw Vulkan object handles now refer to wrapper objects)
 *  - Embedded verification of command usage validity (eg. any attempt to set an event
 *               from within a render-pass will throw an assertion failure)
 *  - Life-time management
 *
 **/
#ifndef WRAPPERS_COMMAND_BUFFER_H
#define WRAPPERS_COMMAND_BUFFER_H

#include "misc/callbacks.h"
#include "misc/debug_marker.h"
#include "misc/types.h"

#ifdef _DEBUG
    #define STORE_COMMAND_BUFFER_COMMANDS
#endif

namespace Anvil
{
    /* Forward declarations */
    struct BufferBarrier;
    struct ImageBarrier;

    /** Enumerates available Vulkan command buffer types */
    typedef enum
    {
        COMMAND_BUFFER_TYPE_PRIMARY,
        COMMAND_BUFFER_TYPE_SECONDARY
    } CommandBufferType;

    /** Enumerates available Vulkan command buffer commands */
    typedef enum
    {
        COMMAND_TYPE_BEGIN_RENDER_PASS,
        COMMAND_TYPE_BEGIN_QUERY,
        COMMAND_TYPE_BIND_DESCRIPTOR_SETS,
        COMMAND_TYPE_BIND_INDEX_BUFFER,
        COMMAND_TYPE_BIND_PIPELINE,
        COMMAND_TYPE_BIND_VERTEX_BUFFER,
        COMMAND_TYPE_BLIT_IMAGE,
        COMMAND_TYPE_CLEAR_ATTACHMENTS,
        COMMAND_TYPE_CLEAR_COLOR_IMAGE,
        COMMAND_TYPE_CLEAR_DEPTH_STENCIL_IMAGE,
        COMMAND_TYPE_COPY_BUFFER,
        COMMAND_TYPE_COPY_BUFFER_TO_IMAGE,
        COMMAND_TYPE_COPY_IMAGE,
        COMMAND_TYPE_COPY_IMAGE_TO_BUFFER,
        COMMAND_TYPE_COPY_QUERY_POOL_RESULTS,
        COMMAND_TYPE_DEBUG_MARKER_BEGIN_EXT,
        COMMAND_TYPE_DEBUG_MARKER_END_EXT,
        COMMAND_TYPE_DEBUG_MARKER_INSERT_EXT,
        COMMAND_TYPE_DISPATCH,
        COMMAND_TYPE_DISPATCH_INDIRECT,
        COMMAND_TYPE_DRAW,
        COMMAND_TYPE_DRAW_INDEXED,
        COMMAND_TYPE_DRAW_INDEXED_INDIRECT,
        COMMAND_TYPE_DRAW_INDEXED_INDIRECT_COUNT_AMD,
        COMMAND_TYPE_DRAW_INDIRECT,
        COMMAND_TYPE_DRAW_INDIRECT_COUNT_AMD,
        COMMAND_TYPE_END_QUERY,
        COMMAND_TYPE_END_RENDER_PASS,
        COMMAND_TYPE_EXECUTE_COMMANDS,
        COMMAND_TYPE_FILL_BUFFER,
        COMMAND_TYPE_NEXT_SUBPASS,
        COMMAND_TYPE_PIPELINE_BARRIER,
        COMMAND_TYPE_PUSH_CONSTANTS,
        COMMAND_TYPE_RESET_EVENT,
        COMMAND_TYPE_RESET_QUERY_POOL,
        COMMAND_TYPE_RESOLVE_IMAGE,
        COMMAND_TYPE_SET_BLEND_CONSTANTS,
        COMMAND_TYPE_SET_DEPTH_BIAS,
        COMMAND_TYPE_SET_DEPTH_BOUNDS,
        COMMAND_TYPE_SET_EVENT,
        COMMAND_TYPE_SET_LINE_WIDTH,
        COMMAND_TYPE_SET_SCISSOR,
        COMMAND_TYPE_SET_STENCIL_COMPARE_MASK,
        COMMAND_TYPE_SET_STENCIL_REFERENCE,
        COMMAND_TYPE_SET_STENCIL_WRITE_MASK,
        COMMAND_TYPE_SET_VIEWPORT,
        COMMAND_TYPE_UPDATE_BUFFER,
        COMMAND_TYPE_WAIT_EVENTS,
        COMMAND_TYPE_WRITE_TIMESTAMP,

    } CommandType;

    /** Base structure for a Vulkan command.
     *
     *  Parent structure for all specialized Vulkan command structures which describe
     *  actual Vulkan commands.
     **/
    typedef struct Command
    {
        CommandType type;

        /** Constructor.
         *
         *  @param in_type Type of the command encapsulated by the structure. */
        Command(CommandType in_type)
        {
            type = in_type;
        }

        /** Stub destructor. */
        virtual ~Command()
        {
            /* Stub */
        }

        Command(const Command& in)
        {
            type = in.type;
        }
    } Command;

    enum CommandBufferCallbackID
    {
        /* Call-back issued whenever a vkCmdPipelineBarrier() is recorded.
         *
         * callback_arg: PipelineBarrierCommandRecordedCallback instance.
         */
        COMMAND_BUFFER_CALLBACK_ID_PIPELINE_BARRIER_COMMAND_RECORDED,

        /* Always last */
        COMMAND_BUFFER_CALLBACK_ID_COUNT
    };

    /** Holds all arguments passed to a vkCmdBeginRenderPass() command.
     *
     *  Raw Vulkan object handles have been replaced with pointers to wrapper objects.
     *  These objects are retained at construction time, and released at descriptor
     *  destruction time.
     */
    typedef struct BeginRenderPassCommand : public Command
    {
        std::vector<VkClearValue>                          clear_values;
        VkSubpassContents                                  contents;
        std::shared_ptr<Anvil::Framebuffer>                fbo_ptr;
        std::vector<std::weak_ptr<Anvil::PhysicalDevice> > physical_devices;
        std::vector<VkRect2D>                              render_areas;
        std::shared_ptr<Anvil::RenderPass>                 render_pass_ptr;

        /** Constructor.
         *
         *  Retains @param in_fbo_ptr and @param in_render_pass_ptr objects.
         *
         *  Arguments as per Vulkan API.
         **/
        explicit BeginRenderPassCommand(uint32_t                                    in_n_clear_values,
                                        const VkClearValue*                         in_clear_value_ptrs,
                                        std::shared_ptr<Anvil::Framebuffer>         in_fbo_ptr,
                                        uint32_t                                    in_n_physical_devices,
                                        const std::weak_ptr<Anvil::PhysicalDevice>* in_physical_devices,
                                        const VkRect2D*                             in_render_areas,
                                        std::shared_ptr<Anvil::RenderPass>          in_render_pass_ptr,
                                        VkSubpassContents                           in_contents);

        /** Destructor.
         *
         *  Releases the encapsulated Framebuffer and RenderPass instances.
         **/
        virtual ~BeginRenderPassCommand()
         {
             /* Stub */
         }


    private:
        BeginRenderPassCommand           (const BeginRenderPassCommand&);
        BeginRenderPassCommand& operator=(const BeginRenderPassCommand&);
    } BeginRenderPassCommand;
        
    /* Structure passed as a COMMAND_BUFFER_CALLBACK_ID_BEGIN_RENDER_PASS_COMMAND_RECORDED call-back argument */
    typedef struct BeginRenderPassCommandRecordedCallbackData
    {
        CommandBufferBase*            command_buffer_ptr;
        const BeginRenderPassCommand* command_details_ptr;

        /** Constructor.
         *
         *  @param in_command_buffer_ptr  Command buffer instance the command is being recorded for.
         *  @param in_command_details_ptr Structure holding all arguments to be passed to the vkCmdBeginRenderPass() call.
         **/
        explicit BeginRenderPassCommandRecordedCallbackData(CommandBufferBase*            in_command_buffer_ptr,
                                                            const BeginRenderPassCommand* in_command_details_ptr)
            :command_buffer_ptr (in_command_buffer_ptr),
             command_details_ptr(in_command_details_ptr) 
        {
            /* Stub */
        }
    } BeginRenderPassCommandRecordedCallbackData;

    /** Holds all arguments passed to a vkCmdEndRenderPass() command. */
    typedef struct EndRenderPassCommand : public Command
    {
        /** Constructor. */
        explicit EndRenderPassCommand();

        virtual ~EndRenderPassCommand()
        {
            /* Stub */
        }
    } EndRenderPassCommand;

    /* Structure passed as a COMMAND_BUFFER_CALLBACK_ID_END_RENDER_PASS_COMMAND_RECORDED call-back argument */
    typedef struct EndRenderPassCommandRecordedCallbackData
    {
        CommandBufferBase*          command_buffer_ptr;
        const EndRenderPassCommand* command_details_ptr;

        /** Constructor.
         *
         *  @param in_command_buffer_ptr  Command buffer instance the command is being recorded for.
         *  @param in_command_details_ptr Structure holding all arguments to be passed to the vkCmdEndRenderPass() call.
         **/
        explicit EndRenderPassCommandRecordedCallbackData(CommandBufferBase*          in_command_buffer_ptr,
                                                          const EndRenderPassCommand* in_command_details_ptr)
            :command_buffer_ptr (in_command_buffer_ptr),
             command_details_ptr(in_command_details_ptr) 
        {
            /* Stub */
        }
    } EndRenderPassCommandRecordedCallbackData;

    /** Holds all arguments passed to a vkCmdPipelineBarrier() command.
     *
     *  Takes an array of Barrier descriptors instead of void* pointers, as is the case
     *  with the original Vulkan API. Each buffer in a buffer barrier, and each image in
     *  an image barrier, is retained.
     **/
    typedef struct PipelineBarrierCommand : public Command
    {
        std::vector<BufferBarrier> buffer_barriers;
        std::vector<ImageBarrier>  image_barriers;
        std::vector<MemoryBarrier> memory_barriers;

        VkDependencyFlagsVariable(flags);

        VkPipelineStageFlagsVariable(dst_stage_mask);
        VkPipelineStageFlagsVariable(src_stage_mask);

        /** Constructor.
         *
         *  Please see the general note for PipelineBarrierCommand structure for more details.
         *
         *  Arguments as per Vulkan API.
         **/
        explicit PipelineBarrierCommand(VkPipelineStageFlags       in_src_stage_mask,
                                        VkPipelineStageFlags       in_dst_stage_mask,
                                        VkDependencyFlags          in_flags,
                                        uint32_t                   in_memory_barrier_count,
                                        const MemoryBarrier* const in_memory_barriers_ptr_ptr,
                                        uint32_t                   in_buffer_memory_barrier_count,
                                        const BufferBarrier* const in_buffer_memory_barrier_ptr_ptr,
                                        uint32_t                   in_image_memory_barrier_count,
                                        const ImageBarrier*  const in_image_memory_barrier_ptr_ptr);

        virtual ~PipelineBarrierCommand()
        {
            /* Stub */
        }
    } PipelineBarrierCommand;

    /* Structure passed as a COMMAND_BUFFER_CALLBACK_ID_PIPELINE_BARRIER_COMMAND_RECORDED call-back argument */
    typedef struct PipelineBarrierCommandRecordedCallbackData
    {
        CommandBufferBase*            command_buffer_ptr;
        const PipelineBarrierCommand* command_details_ptr;

        /** Constructor.
         *
         *  @param in_command_buffer_ptr  Command buffer instance the command is being recorded for.
         *  @param in_command_details_ptr Structure holding all arguments to be passed to the vkCmdPipelineBarrier() call.
         **/
        explicit PipelineBarrierCommandRecordedCallbackData(CommandBufferBase*            in_command_buffer_ptr,
                                                            const PipelineBarrierCommand* in_command_details_ptr)
            :command_buffer_ptr (in_command_buffer_ptr),
             command_details_ptr(in_command_details_ptr) 
        {
            /* Stub */
        }
    } PipelineBarrierCommandRecordedCallbackData;

    /** Implements base functionality of a command buffer object, such as common command registration
     *  support or validation. Also encapsulates command wrapper structure declarations.
     *
     *  The command buffer baking process is not deferred, which means you can use the wrapped Vulkan
     *  command buffer instance right after recording finishes without any additional performance cost.
     *
     *  Provides core functionality for the PrimaryCommandBuffer and SecondaryCommandBuffer classes.
     */
    class CommandBufferBase : public DebugMarkerSupportProvider<CommandBufferBase>,
                              public CallbacksSupportProvider
    {
    public:
        /* Public functions */

        /* Disables internal command stashing which is enbled for builds created with
         * STORE_COMMAND_BUFFER_COMMANDS enabled.
         *
         * Nop for builds that were not built with the definition enabled.
         */
        static void disable_comand_stashing()
        {
            m_command_stashing_disabled = true;
        }

        /** Returns a handle to the raw Vulkan command buffer instance, encapsulated by the object */
        const VkCommandBuffer get_command_buffer() const
        {
            return m_command_buffer;
        }

        /** Returns a pointer to the handle to the raw Vulkan command buffer instance, as encapsulated
         *  by the object.
         **/
        const VkCommandBuffer* get_command_buffer_ptr() const
        {
            return &m_command_buffer;
        }

        /** Tells the type of the command buffer instance */
        CommandBufferType get_command_buffer_type() const
        {
            return m_type;
        }

        /** Returns the parent command pool */
        std::weak_ptr<Anvil::CommandPool> get_parent_command_pool() const
        {
            return m_parent_command_pool_ptr;
        }

        /** Issues a vkCmdBeginQuery() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_begin_query(std::shared_ptr<Anvil::QueryPool> in_query_pool_ptr,
                                Anvil::QueryIndex                 in_entry,
                                VkQueryControlFlags               in_flags);

        /** Issues a vkCmdBindDescriptorSets() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_bind_descriptor_sets(VkPipelineBindPoint                    in_pipeline_bind_point,
                                         std::shared_ptr<Anvil::PipelineLayout> in_layout_ptr,
                                         uint32_t                               in_first_set,
                                         uint32_t                               in_set_count,
                                         std::shared_ptr<Anvil::DescriptorSet>* in_descriptor_set_ptrs,
                                         uint32_t                               in_dynamic_offset_count,
                                         const uint32_t*                        in_dynamic_offset_ptrs);

        /** Issues a vkCmdBindIndexBuffer() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_bind_index_buffer(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                      VkDeviceSize                   in_offset,
                                      VkIndexType                    in_index_type);

        /** Issues a vkCmdBindPipeline() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_bind_pipeline(VkPipelineBindPoint in_pipeline_bind_point,
                                  Anvil::PipelineID   in_pipeline_id);

        /** Issues a vkCmdBindVertexBuffers() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_bind_vertex_buffers(uint32_t                        in_start_binding,
                                        uint32_t                        in_binding_count,
                                        std::shared_ptr<Anvil::Buffer>* in_buffer_ptrs,
                                        const VkDeviceSize*             in_offset_ptrs);

        /** Issues a vkCmdBlitImage() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_blit_image(std::shared_ptr<Anvil::Image> in_src_image_ptr,
                               VkImageLayout                 in_src_image_layout,
                               std::shared_ptr<Anvil::Image> in_dst_image_ptr,
                               VkImageLayout                 in_dst_image_layout,
                               uint32_t                      in_region_count,
                               const VkImageBlit*            in_region_ptrs,
                               VkFilter                      in_filter);

        /** Issues a vkCmdClearAttachments() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when not recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_clear_attachments(uint32_t                 in_n_attachments,
                                      const VkClearAttachment* in_attachment_ptrs,
                                      uint32_t                 in_n_rects,
                                      const VkClearRect*       in_rect_ptrs);

        /** Issues a vkCmdClearColorImage() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when not recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_clear_color_image(std::shared_ptr<Anvil::Image>  in_image_ptr,
                                      VkImageLayout                  in_image_layout,
                                      const VkClearColorValue*       in_color_ptr,
                                      uint32_t                       in_range_count,
                                      const VkImageSubresourceRange* in_range_ptrs);

        /** Issues a vkCmdClearDepthStencilImage() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_clear_depth_stencil_image(std::shared_ptr<Anvil::Image>   in_image_ptr,
                                              VkImageLayout                   in_image_layout,
                                              const VkClearDepthStencilValue* in_depth_stencil_ptr,
                                              uint32_t                        in_range_count,
                                              const VkImageSubresourceRange*  in_range_ptrs);

        /** Issues a vkCmdCopyBuffer() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_copy_buffer(std::shared_ptr<Anvil::Buffer> in_src_buffer_ptr,
                                std::shared_ptr<Anvil::Buffer> in_dst_buffer_ptr,
                                uint32_t                       in_region_count,
                                const VkBufferCopy*            in_region_ptrs);

        /** Issues a vkCmdCopyBufferToImage() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_copy_buffer_to_image(std::shared_ptr<Anvil::Buffer> in_src_buffer_ptr,
                                         std::shared_ptr<Anvil::Image>  in_dst_image_ptr,
                                         VkImageLayout                  in_dst_image_layout,
                                         uint32_t                       in_region_count,
                                         const VkBufferImageCopy*       in_region_ptrs);

        /** Issues a vkCmdCopyImage() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_copy_image(std::shared_ptr<Anvil::Image> in_src_image_ptr,
                               VkImageLayout                 in_src_image_layout,
                               std::shared_ptr<Anvil::Image> in_dst_image_ptr,
                               VkImageLayout                 in_dst_image_layout,
                               uint32_t                      in_region_count,
                               const VkImageCopy*            in_region_ptrs);

        /** Issues a vkCmdCopyImageToBuffer() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_copy_image_to_buffer(std::shared_ptr<Anvil::Image>  in_src_image_ptr,
                                         VkImageLayout                  in_src_image_layout,
                                         std::shared_ptr<Anvil::Buffer> in_dst_buffer_ptr,
                                         uint32_t                       in_region_count,
                                         const VkBufferImageCopy*       in_region_ptrs);

        /** Issues a vkCmdCopyQueryPoolResults() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_copy_query_pool_results(std::shared_ptr<Anvil::QueryPool> in_query_pool_ptr,
                                            Anvil::QueryIndex                 in_start_query,
                                            uint32_t                          in_query_count,
                                            std::shared_ptr<Anvil::Buffer>    in_dst_buffer_ptr,
                                            VkDeviceSize                      in_dst_offset,
                                            VkDeviceSize                      in_dst_stride,
                                            VkQueryResultFlags                in_flags);

        /** Issues a vkCmdDebugMarkerBeginEXT() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per VK_EXT_debug_marker specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_debug_marker_begin_EXT(const std::string& in_marker_name,
                                           const float*       in_opt_color);

        /** Issues a vkCmdDebugMarkerEndEXT() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per VK_EXT_debug_marker specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_debug_marker_end_EXT();

        /** Issues a vkCmdDebugMarkerInsertEXT() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per VK_EXT_debug_marker specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_debug_marker_insert_EXT(const std::string& in_marker_name,
                                            const float*       in_opt_color);

        /** Issues a vkCmdDispatch() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_dispatch(uint32_t in_x,
                             uint32_t in_y,
                             uint32_t in_z);

        /** Issues a vkCmdDispatchIndirect() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_dispatch_indirect(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                      VkDeviceSize                   in_offset);

        /** Issues a vkCmdDraw() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when not recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_draw(uint32_t in_vertex_count,
                         uint32_t in_instance_count,
                         uint32_t in_first_vertex,
                         uint32_t in_first_instance);

        /** Issues a vkCmdDrawIndexed() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when not recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_draw_indexed(uint32_t in_index_count,
                                 uint32_t in_instance_count,
                                 uint32_t in_first_index,
                                 int32_t  in_vertex_offset,
                                 uint32_t in_first_instance);

        /** Issues a vkCmdDrawIndexedIndirect() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when not recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_draw_indexed_indirect(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                          VkDeviceSize                   in_offset,
                                          uint32_t                       in_draw_count,
                                          uint32_t                       in_stride);

        /** Issues a vkCmdDrawIndexedIndirectCount() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when not recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  This function is only available if VK_AMD_draw_indirect_count is supported by the Vulkan
         *  device AND if the extension has been requested at creation time.
         *
         *  Argument meaning is as per VK_AMD_draw_indirect_count specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_draw_indexed_indirect_count_AMD(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                                    VkDeviceSize                   in_offset,
                                                    std::shared_ptr<Anvil::Buffer> in_count_buffer_ptr,
                                                    VkDeviceSize                   in_count_offset,
                                                    uint32_t                       in_max_draw_count,
                                                    uint32_t                       in_stride);
        
        /** Issues a vkCmdDrawIndirect() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when not recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_draw_indirect(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                  VkDeviceSize                   in_offset,
                                  uint32_t                       in_count,
                                  uint32_t                       in_stride);

        /** Issues a vkCmdDrawIndirectCount() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when not recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  This function is only available if VK_AMD_draw_indirect_count is supported by the Vulkan
         *  device AND if the extension has been requested at creation time.
         *
         *  Argument meaning is as per VK_AMD_draw_indirect_count specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_draw_indirect_count_AMD(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                            VkDeviceSize                   in_offset,
                                            std::shared_ptr<Anvil::Buffer> in_count_buffer_ptr,
                                            VkDeviceSize                   in_count_offset,
                                            uint32_t                       in_max_draw_count,
                                            uint32_t                       in_stride);

        /** Issues a vkCmdEndQuery() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_end_query(std::shared_ptr<Anvil::QueryPool> in_query_pool_ptr,
                              Anvil::QueryIndex                 in_entry);

        /** Issues a vkCmdFillBuffer() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_fill_buffer(std::shared_ptr<Anvil::Buffer> in_dst_buffer_ptr,
                                VkDeviceSize                   in_dst_offset,
                                VkDeviceSize                   in_size,
                                uint32_t                       in_data);

        /** Issues a vkCmdPipelineBarrier() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances, passed implicitly to this function, are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_pipeline_barrier(VkPipelineStageFlags       in_src_stage_mask,
                                     VkPipelineStageFlags       in_dst_stage_mask,
                                     VkBool32                   in_by_region,
                                     uint32_t                   in_memory_barrier_count,
                                     const MemoryBarrier* const in_memory_barriers_ptr,
                                     uint32_t                   in_buffer_memory_barrier_count,
                                     const BufferBarrier* const in_buffer_memory_barriers_ptr,
                                     uint32_t                   in_image_memory_barrier_count,
                                     const ImageBarrier*  const in_image_memory_barriers_ptr);

        /** Issues a vkCmdPushConstants() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_push_constants(std::shared_ptr<Anvil::PipelineLayout> in_layout_ptr,
                                   VkShaderStageFlags                     in_stage_flags,
                                   uint32_t                               in_offset,
                                   uint32_t                               in_size,
                                   const void*                            in_values);

        /** Issues a vkCmdResetEvent() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_reset_event(std::shared_ptr<Anvil::Event> in_event_ptr,
                                VkPipelineStageFlags          in_stage_mask);

        /** Issues a vkCmdResetQueryPool() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_reset_query_pool(std::shared_ptr<Anvil::QueryPool> in_query_pool_ptr,
                                     Anvil::QueryIndex                 in_start_query,
                                     uint32_t                          in_query_count);

        /** Issues a vkCmdResolveImage() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_resolve_image(std::shared_ptr<Anvil::Image> in_src_image_ptr,
                                  VkImageLayout                 in_src_image_layout,
                                  std::shared_ptr<Anvil::Image> in_dst_image_ptr,
                                  VkImageLayout                 in_dst_image_layout,
                                  uint32_t                      in_region_count,
                                  const VkImageResolve*         in_region_ptrs);

        /** Issues a vkCmdSetBlendConstants() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_set_blend_constants(const float in_blend_constants[4]);

        /** Issues a vkCmdSetDepthBias() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_set_depth_bias(float in_depth_bias_constant_factor,
                                   float in_depth_bias_clamp,
                                   float in_slope_scaled_depth_bias);

        /** Issues a vkCmdSetDepthBounds() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_set_depth_bounds(float in_min_depth_bounds,
                                     float in_max_depth_bounds);

        /** Issues a vkCmdSetEvent() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_set_event(std::shared_ptr<Anvil::Event> in_event_ptr,
                              VkPipelineStageFlags          in_stage_mask);

        /** Issues a vkCmdSetLineWidth() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_set_line_width(float in_line_width);

        /** Issues a vkCmdSetScissor() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_set_scissor(uint32_t        in_first_scissor,
                                uint32_t        in_scissor_count,
                                const VkRect2D* in_scissor_ptrs);

        /** Issues a vkCmdSetStencilCompareMask() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_set_stencil_compare_mask(VkStencilFaceFlags in_face_mask,
                                             uint32_t           in_stencil_compare_mask);

        /** Issues a vkCmdSetStencilReference() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_set_stencil_reference(VkStencilFaceFlags in_face_mask,
                                          uint32_t           in_stencil_reference);

        /** Issues a vkCmdSetStencilWriteMask() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_set_stencil_write_mask(VkStencilFaceFlags in_face_mask,
                                           uint32_t           in_stencil_write_mask);

        /** Issues a vkCmdSetViewport() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_set_viewport(uint32_t          in_first_viewport,
                                 uint32_t          in_viewport_count,
                                 const VkViewport* in_viewport_ptrs);

        /** Issues a vkCmdUpdateBuffer() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  It is also illegal to call this function when recording renderpass commands. Doing so
         *  will also result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_update_buffer(std::shared_ptr<Anvil::Buffer> in_dst_buffer_ptr,
                                  VkDeviceSize                   in_dst_offset,
                                  VkDeviceSize                   in_data_size,
                                  const uint32_t*                in_data_ptr);

        /** Issues a vkCmdWaitEvents() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_wait_events(uint32_t                       in_event_count,
                                std::shared_ptr<Anvil::Event>* in_event_ptrs,
                                VkPipelineStageFlags           in_src_stage_mask,
                                VkPipelineStageFlags           in_dst_stage_mask,
                                uint32_t                       in_memory_barrier_count,
                                const MemoryBarrier* const     in_memory_barriers_ptr,
                                uint32_t                       in_buffer_memory_barrier_count,
                                const BufferBarrier* const     in_buffer_memory_barriers_ptr,
                                uint32_t                       in_image_memory_barrier_count,
                                const ImageBarrier* const      in_image_memory_barriers_ptr);

        /** Issues a vkCmdWriteTimestamp() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_write_timestamp(VkPipelineStageFlagBits           in_pipeline_stage,
                                    std::shared_ptr<Anvil::QueryPool> in_query_pool_ptr,
                                    Anvil::QueryIndex                 in_entry);

        /** Resets the underlying Vulkan command buffer and clears the internally managed vector of
         *  recorded commands, if STORE_COMMAND_BUFFER_COMMANDS has been defined for the build.
         *
         *  @param in_should_release_resources true if the vkResetCommandBuffer() should be made with the
         *                                     VK_CMD_BUFFER_RESET_RELEASE_RESOURCES_BIT flag set.
         *
         *  @return true if the request was handled successfully, false otherwise.
         **/
        bool reset(bool in_should_release_resources);

        /** Stops an ongoing command recording process.
         *
         *  It is an error to invoke this function if the command buffer has not been put
         *  into the recording mode by calling start_recording().
         *
         *  @return true if successful, false otherwise.
         **/
        bool stop_recording();

    protected:
        /* Forward declarations */
        struct BeginQueryCommand;
        struct BindDescriptorSetsCommand;
        struct BindIndexBufferCommand;
        struct BindPipelineCommand;
        struct BindVertexBuffersCommand;
        struct BlitImageCommand;
        struct ClearAttachmentsCommand;
        struct ClearColorImageCommand;
        struct ClearDepthStencilImageCommand;
        struct CopyBufferCommand;
        struct CopyBufferToImageCommand;
        struct CopyImageCommand;
        struct CopyImageToBufferCommand;
        struct CopyQueryPoolResultsCommand;
        struct DebugMarkerBeginEXTCommand;
        struct DebugMarkerEndEXTCommand;
        struct DispatchCommand;
        struct DispatchIndirectCommand;
        struct DrawCommand;
        struct DrawIndexedCommand;
        struct DrawIndirectCommand;
        struct DrawIndexedIndirectCommand;
        struct EndQueryCommand;
        struct ExecuteCommandsCommand;
        struct FillBufferCommand;
        struct NextSubpassCommand;
        struct PushConstantsCommand;
        struct ResetEventCommand;
        struct ResetQueryPoolCommand;
        struct ResolveImageCommand;
        struct SetBlendConstantsCommand;
        struct SetDepthBiasCommand;
        struct SetDepthBoundsCommand;
        struct SetEventCommand;
        struct SetLineWidthCommand;
        struct SetScissorCommand;
        struct SetStencilCompareMaskCommand;
        struct SetStencilReferenceCommand;
        struct SetStencilWriteMaskCommand;
        struct SetViewportCommand;
        struct UpdateBufferCommand;
        struct WaitEventsCommand;
        struct WriteTimestampCommand;

        /* Protected type definitions */

        /** Holds all arguments passed to a vkCmdBeginQuery() command */
        typedef struct BeginQueryCommand : public Command
        {
            VkQueryControlFlagsVariable(flags);

            Anvil::QueryIndex                 entry;
            std::shared_ptr<Anvil::QueryPool> query_pool_ptr;

            /** Constructor. */
            explicit BeginQueryCommand(std::shared_ptr<Anvil::QueryPool> in_query_pool_ptr,
                                       Anvil::QueryIndex                 in_entry,
                                       VkQueryControlFlags               in_flags);

            /** Destructor. */
            virtual ~BeginQueryCommand()
            {
                /* Stub */
            }
        } BeginQueryCommand;

        /** Holds all arguments passed to a vkCmdBindDescriptorSets() command. */
        typedef struct BindDescriptorSetsCommand : public Command
        {
            std::vector<std::shared_ptr<Anvil::DescriptorSet> > descriptor_sets;
            std::vector<uint32_t>                               dynamic_offsets;
            uint32_t                                            first_set;
            std::shared_ptr<Anvil::PipelineLayout>              layout_ptr;
            VkPipelineBindPoint                                 pipeline_bind_point;

            /** Constructor. **/
            explicit BindDescriptorSetsCommand(VkPipelineBindPoint                    in_pipeline_bind_point,
                                               std::shared_ptr<Anvil::PipelineLayout> in_layout_ptr,
                                               uint32_t                               in_first_set,
                                               uint32_t                               in_set_count,
                                               std::shared_ptr<Anvil::DescriptorSet>* in_descriptor_set_ptrs,
                                               uint32_t                               in_dynamic_offset_count,
                                               const uint32_t*                        in_dynamic_offset_ptrs);

            /** Destructor. */
            virtual ~BindDescriptorSetsCommand()
            {
                /* Stub */
            }

        } BindDescriptorSetsCommand;


        /** Holds all arguments passed to a vkCmdBindIndexBuffer() command.
         *
         *  Raw Vulkan object handles have been replaced with pointers to wrapper objects.
         *  These objects are retained at construction time, and released at descriptor
         *  destruction time.
         */
        typedef struct BindIndexBufferCommand : public Command
        {
            VkBuffer                       buffer;
            std::shared_ptr<Anvil::Buffer> buffer_ptr;
            VkIndexType                    index_type;
            VkDeviceSize                   offset;

            /** Constructor. **/
            explicit BindIndexBufferCommand(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                            VkDeviceSize                   in_offset,
                                            VkIndexType                    in_index_type);

            /** Destructor. */
            virtual ~BindIndexBufferCommand()
            {
                /* Stub */
            }

        private:
            BindIndexBufferCommand& operator=(const BindIndexBufferCommand&);
        } BindIndexBufferCommand;


        /** Holds all arguments passed to a vkCmdBindPipeline() command. */
        typedef struct BindPipelineCommand : public Command
        {
            VkPipelineBindPoint pipeline_bind_point;
            Anvil::PipelineID   pipeline_id;

            /** Constructor.
             *
             *  @param in_pipeline_bind_point As per Vulkan API.
             *  @param in_pipeline_id         ID of the pipeline. Can either be a compute pipeline ID, coming from
             *                                the device-specific compute pipeline manager initialized by the library,
             *                                or a graphics pipeline ID, coming from the device-specific graphics pipeline
             *                                manager. The type of the pipeline is deduced from @param in_pipeline_bind_point.
             **/
            explicit BindPipelineCommand(VkPipelineBindPoint in_pipeline_bind_point,
                                         Anvil::PipelineID   in_pipeline_id);

            /* Destructor. */
            virtual ~BindPipelineCommand()
            {
                 /* Stub */
            }
        } BindPipelineCommand;


        /** Holds a single vertex buffer binding, as specified by "in_buffer_ptrs" and "in_offset_ptrs"
         *  argment arrays, passed to a vkCmdBindVertexBuffers() call.
         **/
        typedef struct BindVertexBuffersCommandBinding
        {
            VkBuffer                       buffer;
            std::shared_ptr<Anvil::Buffer> buffer_ptr;
            VkDeviceSize                   offset;

            /** Constructor. **/
            explicit BindVertexBuffersCommandBinding(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                                     VkDeviceSize                   in_offset);

            /** Destructor. */
            virtual ~BindVertexBuffersCommandBinding()
            {
                /* Stub */
            }

            BindVertexBuffersCommandBinding(const BindVertexBuffersCommandBinding& in);

        private:
            BindVertexBuffersCommandBinding& operator=(const BindVertexBuffersCommandBinding&);
        } BindVertexBuffersCommandBinding;

        /** Holds all arguments passed to a vkCmdBindVertexBuffers() command. */
        typedef struct BindVertexBuffersCommand : public Command
        {
            std::vector<BindVertexBuffersCommandBinding> bindings;
            uint32_t                                     start_binding;

            /** Constructor. **/
            explicit BindVertexBuffersCommand(uint32_t                        in_start_binding,
                                              uint32_t                        in_binding_count,
                                              std::shared_ptr<Anvil::Buffer>* in_buffer_ptrs,
                                              const VkDeviceSize*             in_offset_ptrs);

            /** Destructor. */
            virtual ~BindVertexBuffersCommand()
            {
                /* Stub */
            }
        } BindVertexBuffersCommand;

        /** Holds all arguments passed to a vkCmdBlitImage() command. */
        typedef struct BlitImageCommand : public Command
        {
            VkImage                       dst_image;
            VkImageLayout                 dst_image_layout;
            std::shared_ptr<Anvil::Image> dst_image_ptr;
            VkImage                       src_image;
            VkImageLayout                 src_image_layout;
            std::shared_ptr<Anvil::Image> src_image_ptr;

            VkFilter                 filter;
            std::vector<VkImageBlit> regions;

            /** Constructor. */
            explicit BlitImageCommand(std::shared_ptr<Anvil::Image> in_src_image_ptr,
                                      VkImageLayout                 in_src_image_layout,
                                      std::shared_ptr<Anvil::Image> in_dst_image_ptr,
                                      VkImageLayout                 in_dst_image_layout,
                                      uint32_t                      in_region_count,
                                      const VkImageBlit*            in_region_ptrs,
                                      VkFilter                      in_filter);

            /** Destructor. */
            virtual ~BlitImageCommand()
            {
                /* Stub */
            }

        private:
            BlitImageCommand& operator=(const BlitImageCommand&);
        } BlitImageCommand;


        /* Holds a single attachment definition, as used by ClearAttachmentsCommand descriptor */
        typedef struct ClearAttachmentsCommandAttachment
        {
            VkImageAspectFlagsVariable(aspect_mask);

            VkClearValue clear_value;
            uint32_t     color_attachment;

            /** Constructor. **/
            ClearAttachmentsCommandAttachment(VkImageAspectFlags in_aspect_mask,
                                              VkClearValue       in_clear_value,
                                              uint32_t           in_color_attachment)
            {
                aspect_mask      = in_aspect_mask;
                clear_value      = in_clear_value;
                color_attachment = in_color_attachment;
            }

            /** Destructor. */
            virtual ~ClearAttachmentsCommandAttachment()
            {
                /* Stub */
            }
        } ClearAttachmentsCommandAttachment;

        /** Holds all arguments passed to a vkCmdClearAttachments() command. */
        typedef struct ClearAttachmentsCommand : public Command
        {
            std::vector<ClearAttachmentsCommandAttachment> attachments;
            std::vector<VkClearRect>                       rects;

            /* Constructor. **/
            explicit ClearAttachmentsCommand(uint32_t                 in_n_attachments,
                                             const VkClearAttachment* in_attachments,
                                             uint32_t                 in_n_rects,
                                             const VkClearRect*       in_rect_ptrs);

            /** Destructor. */
            virtual ~ClearAttachmentsCommand()
            {
                /* Stub */
            }
        } ClearAttachmentsCommand;


        /** Holds all arguments passed to a vkCmdClearColorImage() command. */
        typedef struct ClearColorImageCommand : public Command
        {
            VkClearColorValue                    color;
            VkImage                              image;
            VkImageLayout                        image_layout;
            std::shared_ptr<Anvil::Image>        image_ptr;
            std::vector<VkImageSubresourceRange> ranges;

            /** Constructor. **/
            explicit ClearColorImageCommand(std::shared_ptr<Anvil::Image>  in_image_ptr,
                                            VkImageLayout                  in_image_layout,
                                            const VkClearColorValue*       in_color_ptr,
                                            uint32_t                       in_range_count,
                                            const VkImageSubresourceRange* in_range_ptrs);

            /** Destructor. */
            virtual ~ClearColorImageCommand()
            {
                /* Stub */
            }

        private:
            ClearColorImageCommand& operator=(const ClearColorImageCommand& in);
        } ClearColorImageCommand;


        /** Holds all arguments passed to a vkCmdClearDepthStencilImage() command. */
        typedef struct ClearDepthStencilImageCommand : public Command
        {
            VkClearDepthStencilValue             depth_stencil;
            VkImage                              image;
            VkImageLayout                        image_layout;
            std::shared_ptr<Anvil::Image>        image_ptr;
            std::vector<VkImageSubresourceRange> ranges;

            /** Constructor. **/
            explicit ClearDepthStencilImageCommand(std::shared_ptr<Anvil::Image>   in_image_ptr,
                                                   VkImageLayout                   in_image_layout,
                                                   const VkClearDepthStencilValue* in_depth_stencil_ptr,
                                                   uint32_t                        in_range_count,
                                                   const VkImageSubresourceRange*  in_range_ptrs);

            /** Destructor. */
            virtual ~ClearDepthStencilImageCommand()
            {
                /* Stub */
            }

        private:
            ClearDepthStencilImageCommand& operator=(const ClearDepthStencilImageCommand& in);
        } ClearDepthStencilImageCommand;

        /** Holds all arguments passed to a vkCmdCopyBuffer() command. */
        typedef struct CopyBufferCommand : public Command
        {
            VkBuffer                       dst_buffer;
            std::shared_ptr<Anvil::Buffer> dst_buffer_ptr;
            std::vector<VkBufferCopy>      regions;
            VkBuffer                       src_buffer;
            std::shared_ptr<Anvil::Buffer> src_buffer_ptr;

            /** Constructor. **/
            explicit CopyBufferCommand(std::shared_ptr<Anvil::Buffer> in_src_buffer_ptr,
                                       std::shared_ptr<Anvil::Buffer> in_dst_buffer_ptr,
                                       uint32_t                       in_region_count,
                                       const VkBufferCopy*            in_region_ptrs);

            /** Destructor. */
            virtual ~CopyBufferCommand()
            {
                /* Stub */
            }

        private:
            CopyBufferCommand& operator=(const CopyBufferCommand&);
        } CopyBufferCommand;


        /** Holds all arguments passed to a vkCmdCopyBufferToImage() command. */
        typedef struct CopyBufferToImageCommand : public Command
        {
            VkImage                        dst_image;
            VkImageLayout                  dst_image_layout;
            std::shared_ptr<Anvil::Image>  dst_image_ptr;
            std::vector<VkBufferImageCopy> regions;
            VkBuffer                       src_buffer;
            std::shared_ptr<Anvil::Buffer> src_buffer_ptr;

            /** Constructor. **/
            explicit CopyBufferToImageCommand(std::shared_ptr<Anvil::Buffer> in_src_buffer_ptr,
                                              std::shared_ptr<Anvil::Image>  in_dst_image_ptr,
                                              VkImageLayout                  in_dst_image_layout,
                                              uint32_t                       in_region_count,
                                              const VkBufferImageCopy*       in_region_ptrs);

            /** Destructor. */
            virtual ~CopyBufferToImageCommand()
            {
                /* Stub */
            }

        private:
            CopyBufferToImageCommand& operator=(const CopyBufferToImageCommand&);

        } CopyBufferToImageCommand;


        /** Holds all arguments passed to a vkCmdCopyImage() command. */
        typedef struct CopyImageCommand : public Command
        {
            VkImage                       dst_image;
            std::shared_ptr<Anvil::Image> dst_image_ptr;
            VkImageLayout                 dst_image_layout;
            std::vector<VkImageCopy>      regions;
            VkImage                       src_image;
            std::shared_ptr<Anvil::Image> src_image_ptr;
            VkImageLayout                 src_image_layout;

            /** Constructor. **/
            explicit CopyImageCommand(std::shared_ptr<Anvil::Image> in_src_image_ptr,
                                      VkImageLayout                 in_src_image_layout,
                                      std::shared_ptr<Anvil::Image> in_dst_image_ptr,
                                      VkImageLayout                 in_dst_image_layout,
                                      uint32_t                      in_region_count,
                                      const VkImageCopy*            in_region_ptrs);

            /** Destructor. */
            virtual ~CopyImageCommand()
            {
                /* Stub */
            }

        private:
            CopyImageCommand& operator=(const CopyImageCommand&);
        } CopyImageCommand;


        /** Holds all arguments passed to a vkCmdCopyImageToBuffer() command. */
        typedef struct CopyImageToBufferCommand : public Command
        {
            VkBuffer                       dst_buffer;
            std::shared_ptr<Anvil::Buffer> dst_buffer_ptr;
            std::vector<VkBufferImageCopy> regions;
            VkImage                        src_image;
            VkImageLayout                  src_image_layout;
            std::shared_ptr<Anvil::Image>  src_image_ptr;

            /** Constructor. **/
            explicit CopyImageToBufferCommand(std::shared_ptr<Anvil::Image>  in_src_image_ptr,
                                              VkImageLayout                  in_src_image_layout,
                                              std::shared_ptr<Anvil::Buffer> in_dst_buffer_ptr,
                                              uint32_t                       in_region_count,
                                              const VkBufferImageCopy*       in_region_ptrs);

            /** Destructor. */
            virtual ~CopyImageToBufferCommand()
            {
                /* Stub */
            }

        private:
            CopyImageToBufferCommand& operator=(const CopyImageToBufferCommand&);
        } CopyImageToBufferCommand;


        /** Holds all arguments passed to a vkCmdCopyQueryPoolResults() command. */
        typedef struct CopyQueryPoolResultsCommand : public Command
        {
            VkQueryResultFlagsVariable(flags);

            VkBuffer                          dst_buffer;
            std::shared_ptr<Anvil::Buffer>    dst_buffer_ptr;
            VkDeviceSize                      dst_offset;
            VkDeviceSize                      dst_stride;
            uint32_t                          query_count;
            std::shared_ptr<Anvil::QueryPool> query_pool_ptr;
            Anvil::QueryIndex                 start_query;

            /** Constructor. **/
            explicit CopyQueryPoolResultsCommand(std::shared_ptr<Anvil::QueryPool> in_query_pool_ptr,
                                                 Anvil::QueryIndex                 in_start_query,
                                                 uint32_t                          in_query_count,
                                                 std::shared_ptr<Anvil::Buffer>    in_dst_buffer_ptr,
                                                 VkDeviceSize                      in_dst_offset,
                                                 VkDeviceSize                      in_dst_stride,
                                                 VkQueryResultFlags                in_flags);

            /** Destructor. */
            virtual ~CopyQueryPoolResultsCommand()
            {
                /* Stub */
            }

        private:
            CopyQueryPoolResultsCommand& operator=(const CopyQueryPoolResultsCommand&);
        } CopyQueryPoolResultsCommand;


        /** Holds all arguments passed to a vkCmdDebugMarkerBeginEXT() command. */
        typedef struct DebugMarkerBeginEXTCommand : public Command
        {
            float       color[4];
            std::string marker_name;

            /** Constructor. */
            explicit DebugMarkerBeginEXTCommand(const std::string& in_marker_name,
                                                const float*       in_color);

            /* Destructor */
            virtual ~DebugMarkerBeginEXTCommand()
            {
                /* Stub */
            }
        } DebugMarkerBeginEXTCommand;

        /** Holds all arguments passed to a vkCmdDebugMarkerEndEXT() command. */
        typedef struct DebugMarkerEndEXTCommand : public Command
        {
            /* Constructor. */
            explicit DebugMarkerEndEXTCommand();

            /* Destructor. */
            virtual ~DebugMarkerEndEXTCommand()
            {
                /* Stub */
            }
        } DebugMarkerEndEXTCommand;

        /** Holds all arguments passed to a vkCmdDebugMarkerInsertEXT() command. */
        typedef struct DebugMarkerInsertEXTCommand : public Command
        {
            float       color[4];
            std::string marker_name;

            /** Constructor. */
            explicit DebugMarkerInsertEXTCommand(const std::string& in_marker_name,
                                                 const float*       in_color);

            /* Destructor. */
            virtual ~DebugMarkerInsertEXTCommand()
            {
            }
        } DebugMarkerInsertEXTCommand;

        /** Holds all arguments passed to a vkCmdDispatch() command. */
        typedef struct DispatchCommand : public Command
        {
            uint32_t x;
            uint32_t y;
            uint32_t z;

            /** Constructor. **/
            explicit DispatchCommand(uint32_t in_x,
                                     uint32_t in_y,
                                     uint32_t in_z);

            /** Destructor. */
            virtual ~DispatchCommand()
            {
                /* Stub */
            }
        } DispatchCommand;


        /** Holds all arguments passed to a vkCmdDispatchIndirect() command. */
        typedef struct DispatchIndirectCommand : public Command
        {
            VkBuffer                       buffer;
            std::shared_ptr<Anvil::Buffer> buffer_ptr;
            VkDeviceSize                   offset;

            /** Constructor. **/
            explicit DispatchIndirectCommand(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                             VkDeviceSize                   in_offset);

            /** Destructor. */
            virtual ~DispatchIndirectCommand()
            {
                /* Stub */
            }

        private:
            DispatchIndirectCommand& operator=(const DispatchIndirectCommand&);
        } DispatchIndirectCommand;


        /** Holds all arguments passed to a vkCmdDraw() command. */
        typedef struct DrawCommand : public Command
        {
            uint32_t first_instance;
            uint32_t first_vertex;
            uint32_t instance_count;
            uint32_t vertex_count;

            /** Constructor.
             *
             *  Arguments as per Vulkan API.
             **/
            explicit DrawCommand(uint32_t in_vertex_count,
                                 uint32_t in_instance_count,
                                 uint32_t in_first_vertex,
                                 uint32_t in_first_instance);

            /** Destructor. */
            virtual ~DrawCommand()
            {
                /* Stub */
            }
        } DrawCommand;


        /** Holds all arguments passed to a vkCmdDrawIndexed() command. */
        typedef struct DrawIndexedCommand : public Command
        {
            uint32_t first_index;
            uint32_t first_instance;
            uint32_t index_count;
            uint32_t instance_count;
            int32_t  vertex_offset;

            /** Constructor. **/
            explicit DrawIndexedCommand(uint32_t in_index_count,
                                        uint32_t in_instance_count,
                                        uint32_t in_first_index,
                                        int32_t  in_vertex_offset,
                                        uint32_t in_first_instance);

            /** Destructor. */
            virtual ~DrawIndexedCommand()
            {
                /* Stub */
            }
        } DrawIndexedCommand;


        /** Holds all arguments passed to a vkCmdDrawIndirect() command. */
        typedef struct DrawIndirectCommand : public Command
        {
            VkBuffer                       buffer;
            std::shared_ptr<Anvil::Buffer> buffer_ptr;
            uint32_t                       count;
            VkDeviceSize                   offset;
            uint32_t                       stride;

            /** Constructor. **/
            explicit DrawIndirectCommand(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                         VkDeviceSize                   in_offset,
                                         uint32_t                       in_count,
                                         uint32_t                       in_stride);

            /** Destructor. */
            virtual ~DrawIndirectCommand()
            {
                /* Stub */
            }

        private:
            DrawIndirectCommand& operator=(const DrawIndirectCommand&);
        } DrawIndirectCommand;

        /** Holds all arguments passed to a vkCmdDrawIndirectCountAMD() command. */
        typedef struct DrawIndirectCountAMDCommand : public Command
        {
            VkBuffer                                    buffer;
            std::shared_ptr<Anvil::Buffer>              buffer_ptr;
            VkBuffer                                    count_buffer;
            std::shared_ptr<Anvil::Buffer>              count_buffer_ptr;
            VkDeviceSize                                count_offset;
            uint32_t                                    max_draw_count;
            VkDeviceSize                                offset;
            uint32_t                                    stride;

            /** Constructor. **/
            explicit DrawIndirectCountAMDCommand(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                                 VkDeviceSize                   in_offset,
                                                 std::shared_ptr<Anvil::Buffer> in_count_buffer_ptr,
                                                 VkDeviceSize                   in_count_offset,
                                                 uint32_t                       in_max_draw_count,
                                                 uint32_t                       in_stride);

            /** Destructor */
            virtual ~DrawIndirectCountAMDCommand()
            {
                /* Stub */
            }

        private:
            DrawIndirectCountAMDCommand& operator=(const DrawIndirectCountAMDCommand&);
        } DrawIndirectCountAMDCommand;

        /** Holds all arguments passed to a vkCmdDrawIndexedIndirect() command. */
        typedef struct DrawIndexedIndirectCommand : public Command
        {
            VkBuffer                       buffer;
            std::shared_ptr<Anvil::Buffer> buffer_ptr;
            uint32_t                      draw_count;
            VkDeviceSize                  offset;
            uint32_t                      stride;

            /** Constructor. **/
            explicit DrawIndexedIndirectCommand(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                                VkDeviceSize                   in_offset,
                                                uint32_t                       in_draw_count,
                                                uint32_t                       in_stride);

            /** Destructor. */
            virtual ~DrawIndexedIndirectCommand()
            {
                /* Stub */
            }

        private:
            DrawIndexedIndirectCommand& operator=(const DrawIndexedIndirectCommand&);
        } DrawIndexedIndirectCommand;

        /** Holds all arguments passed to a vkCmdDrawIndexedIndirectCountAMD() command. */
        typedef struct DrawIndexedIndirectCountAMDCommand : public Command
        {
            VkBuffer                                    buffer;
            std::shared_ptr<Anvil::Buffer>              buffer_ptr;
            VkBuffer                                    count_buffer;
            std::shared_ptr<Anvil::Buffer>              count_buffer_ptr;
            VkDeviceSize                                count_offset;
            uint32_t                                    max_draw_count;
            VkDeviceSize                                offset;
            uint32_t                                    stride;

            /** Constructor. **/
            explicit DrawIndexedIndirectCountAMDCommand(std::shared_ptr<Anvil::Buffer> in_buffer_ptr,
                                                        VkDeviceSize                   in_offset,
                                                        std::shared_ptr<Anvil::Buffer> in_count_buffer_ptr,
                                                        VkDeviceSize                   in_count_offset,
                                                        uint32_t                       in_max_draw_count,
                                                        uint32_t                       in_stride);

            /** Destructor */
            virtual ~DrawIndexedIndirectCountAMDCommand()
            {
                /* Stub */
            }

        private:
            DrawIndexedIndirectCountAMDCommand& operator=(const DrawIndexedIndirectCountAMDCommand&);
        } DrawIndexedIndirectCountAMDCommand;

        /** Holds all arguments passed to a vkCmdEndQuery() command. */
        typedef struct EndQueryCommand : public Command
        {
            Anvil::QueryIndex                 entry;
            std::shared_ptr<Anvil::QueryPool> query_pool_ptr;

            /** Constructor. **/
            explicit EndQueryCommand(std::shared_ptr<Anvil::QueryPool> in_query_pool_ptr,
                                     Anvil::QueryIndex                 in_entry);

            /** Destructor. */
            virtual ~EndQueryCommand()
            {
                /* Stub */
            }

        } EndQueryCommand;

        /** Holds all arguments passed to a vkCmdExecuteCommands() command. */
        typedef struct ExecuteCommandsCommand : public Command
        {
            std::vector<std::shared_ptr<Anvil::SecondaryCommandBuffer> > command_buffer_ptrs;
            std::vector<VkCommandBuffer>                                 command_buffers;

            /** Constructor. **/
            explicit ExecuteCommandsCommand(uint32_t                                        in_cmd_buffers_count,
                                            std::shared_ptr<Anvil::SecondaryCommandBuffer>* in_cmd_buffer_ptrs);

            /** Destructor. */
            virtual ~ExecuteCommandsCommand()
            {
                /* Stub */
            }

        private:
            ExecuteCommandsCommand& operator=(const ExecuteCommandsCommand&);
        } ExecuteCommandsCommand;


        /** Holds all arguments passed to a vkCmdFillBuffer() command. */
        typedef struct FillBufferCommand : public Command
        {
            uint32_t                       data;
            VkBuffer                       dst_buffer;
            std::shared_ptr<Anvil::Buffer> dst_buffer_ptr;
            VkDeviceSize                   dst_offset;
            VkDeviceSize                   size;

            /** Constructor. **/
            explicit FillBufferCommand(std::shared_ptr<Anvil::Buffer> in_dst_buffer_ptr,
                                       VkDeviceSize                   in_dst_offset,
                                       VkDeviceSize                   in_size,
                                       uint32_t                       in_data);

            /** Destructor. */
            virtual ~FillBufferCommand()
            {
                /* Stub */
            }

        private:
            FillBufferCommand& operator=(const FillBufferCommand&);
        } FillBufferCommand;


        /** Holds all arguments passed to a vkCmdNextSubpass() command. */
        typedef struct NextSubpassCommand : public Command
        {
            VkSubpassContents contents;

            /** Constructor. **/
            explicit NextSubpassCommand(VkSubpassContents in_contents);

            /** Destructor. */
            virtual ~NextSubpassCommand()
            {
                /* Stub */
            }
        } NextSubpassCommand;


        /** Holds all arguments passed to a vkCmdPushConstants() command. */
        typedef struct PushConstantsCommand : public Command
        {
            VkShaderStageFlagsVariable(stage_flags);

            std::shared_ptr<Anvil::PipelineLayout> layout_ptr;
            uint32_t                               offset;
            uint32_t                               size;
            const void*                            values;

            /** Constructor. **/
            explicit PushConstantsCommand(std::shared_ptr<Anvil::PipelineLayout> in_layout_ptr,
                                          VkShaderStageFlags                     in_stage_flags,
                                          uint32_t                               in_offset,
                                          uint32_t                               in_size,
                                          const void*                            in_values);

            /** Destructor. */
            virtual ~PushConstantsCommand()
            {
                /* Stub */
            }
        } PushConstantsCommand;


        /** Holds all arguments passed to a vkCmdResetEvent() command. **/
        typedef struct ResetEventCommand : public Command
        {
            VkPipelineStageFlagsVariable(stage_mask);

            VkEvent                       event;
            std::shared_ptr<Anvil::Event> event_ptr;

            /** Constructor. **/
            explicit ResetEventCommand(std::shared_ptr<Anvil::Event> in_event_ptr,
                                       VkPipelineStageFlags          in_stage_mask);

            /** Destructor. */
            virtual ~ResetEventCommand()
            {
                /* Stub */
            }

        private:
            ResetEventCommand& operator=(const ResetEventCommand&);
        } ResetEventCommand;


        /** Holds all arguments passed to a vkCmdResetQueryPoolCommand() command. **/
        typedef struct ResetQueryPoolCommand : public Command
        {
            uint32_t                          query_count;
            std::shared_ptr<Anvil::QueryPool> query_pool_ptr;
            Anvil::QueryIndex                 start_query;

            /** Constructor. **/
            explicit ResetQueryPoolCommand(std::shared_ptr<Anvil::QueryPool> in_query_pool_ptr,
                                           Anvil::QueryIndex                 in_start_query,
                                           uint32_t                          in_query_count);

            /** Destructor. */
            virtual ~ResetQueryPoolCommand()
            {
                /* Stub */
            }
        } ResetQueryPoolCommand;


        /** Holds all arguments passed to a vkCmdResolveImage() command. **/
        typedef struct ResolveImageCommand : public Command
        {
            VkImage                       dst_image;
            std::shared_ptr<Anvil::Image> dst_image_ptr;
            VkImageLayout                 dst_image_layout;
            std::vector<VkImageResolve>   regions;
            VkImage                       src_image;
            std::shared_ptr<Anvil::Image> src_image_ptr;
            VkImageLayout                 src_image_layout;

            /** Constructor. **/
            explicit ResolveImageCommand(std::shared_ptr<Anvil::Image> in_src_image_ptr,
                                         VkImageLayout                 in_src_image_layout,
                                         std::shared_ptr<Anvil::Image> in_dst_image_ptr,
                                         VkImageLayout                 in_dst_image_layout,
                                         uint32_t                      in_region_count,
                                         const VkImageResolve*         in_region_ptrs);

            /** Destructor. */
            virtual ~ResolveImageCommand()
            {
                /* Stub */
            }

        private:
            ResolveImageCommand& operator=(const ResolveImageCommand&);
        } ResolveImageCommand;


        /** Holds all arguments passed to a vkCmdSetBlendConstants() command. **/
        typedef struct SetBlendConstantsCommand : public Command
        {
            float blend_constants[4];

            /** Constructor. **/
            explicit SetBlendConstantsCommand(const float in_blend_constants[4]);

            /** Destructor. */
            virtual ~SetBlendConstantsCommand()
            {
                /* Stub */
            }
        } SetBlendConstantsCommand;


        /** Holds all arguments passed to a vkCmdSetDepthBias() command. **/
        typedef struct SetDepthBiasCommand : public Command
        {
            float depth_bias_clamp;
            float depth_bias_constant_factor;
            float slope_scaled_depth_bias;

            /** Constructor. **/
            explicit SetDepthBiasCommand(float in_depth_bias_constant_factor,
                                         float in_depth_bias_clamp,
                                         float in_slope_scaled_depth_bias);

            /** Destructor. */
            virtual ~SetDepthBiasCommand()
            {
                /* Stub */
            }
        } SetDepthBiasCommand;


        /** Holds all arguments passed to a vkCmdSetDepthBounds() command. **/
        typedef struct SetDepthBoundsCommand : public Command
        {
            float max_depth_bounds;
            float min_depth_bounds;

            /** Constructor. **/
            explicit SetDepthBoundsCommand(float in_min_depth_bounds,
                                           float in_max_depth_bounds);

            /** Destructor. */
            virtual ~SetDepthBoundsCommand()
            {
                /* Stub */
            }
        } SetDepthBoundsCommand;

        /** Holds all arguments passed to a vkCmdSetEvent() command. **/
        typedef struct SetEventCommand : public Command
        {
            VkEvent                       event;
            std::shared_ptr<Anvil::Event> event_ptr;

            VkPipelineStageFlagsVariable(stage_mask);

            /** Constructor. **/
            explicit SetEventCommand(std::shared_ptr<Anvil::Event> in_event_ptr,
                                     VkPipelineStageFlags          in_stage_mask);

            /** Destructor. */
            virtual ~SetEventCommand()
            {
                /* Stub */
            }

        private:
            SetEventCommand& operator=(const SetEventCommand&);
        } SetEventCommand;


        /** Holds all arguments passed to a vkCmdSetLineWidth() command. **/
        typedef struct SetLineWidthCommand : public Command
        {
            float line_width;

            /** Constructor. **/
            explicit SetLineWidthCommand(float in_line_width);

            /** Destructor. */
            virtual ~SetLineWidthCommand()
            {
                /* Stub */
            }
        } SetLineWidthCommand;


        /** Holds all arguments passed to a vkCmdSetScissor() command. **/
        typedef struct SetScissorCommand : public Command
        {
            uint32_t              first_scissor;
            std::vector<VkRect2D> scissors;

            /** Constructor. **/
            explicit SetScissorCommand(uint32_t        in_first_scissor,
                                       uint32_t        in_scissor_count,
                                       const VkRect2D* in_scissor_ptrs);

            /** Destructor. */
            virtual ~SetScissorCommand()
            {
                /* Stub */
            }
        } SetScissorCommand;


        /** Holds all arguments passed to a vkCmdSetStencilCompareMask() command. **/
        typedef struct SetStencilCompareMaskCommand : public Command
        {
            VkStencilFaceFlagsVariable(face_mask);

            uint32_t stencil_compare_mask;

            /** Constructor. **/
            explicit SetStencilCompareMaskCommand(VkStencilFaceFlags in_face_mask,
                                                  uint32_t           in_stencil_compare_mask);

            /** Destructor. */
            virtual ~SetStencilCompareMaskCommand()
            {
                /* Stub */
            }
        } SetStencilCompareMaskCommand;


        /** Holds all arguments passed to a vkCmdSetStencilReference() command. **/
        typedef struct SetStencilReferenceCommand : public Command
        {
            VkStencilFaceFlagsVariable(face_mask);

            uint32_t stencil_reference;

            /** Constructor. **/
            explicit SetStencilReferenceCommand(VkStencilFaceFlags in_face_mask,
                                                uint32_t           in_stencil_reference);

            /** Destructor. */
            virtual ~SetStencilReferenceCommand()
            {
                /* Stub */
            }
        } SetStencilReferenceCommand;


        /** Holds all arguments passed to a vkCmdSetStencilWriteMask() command. **/
        typedef struct SetStencilWriteMaskCommand : public Command
        {
            VkStencilFaceFlagsVariable(face_mask);

            uint32_t stencil_write_mask;

            /** Constructor. **/
            explicit SetStencilWriteMaskCommand(VkStencilFaceFlags in_face_mask,
                                                uint32_t           in_stencil_write_mask);

            /** Destructor. */
            virtual ~SetStencilWriteMaskCommand()
            {
                /* Stub */
            }
        } SetStencilWriteMaskCommand;

        /** Holds all arguments passed to a vkCmdSetViewport() command. **/
        typedef struct SetViewportCommand : public Command
        {
            uint32_t                first_viewport;
            std::vector<VkViewport> viewports;

            /** Constructor. **/
            explicit SetViewportCommand(uint32_t          in_first_viewport,
                                        uint32_t          in_viewport_count,
                                        const VkViewport* in_viewport_ptrs);

            /** Destructor. */
            virtual ~SetViewportCommand()
            {
                /* Stub */
            }
        } SetViewportCommand;


        /** Holds all arguments passed to a vkCmdUpdateBuffer() command. **/
        typedef struct UpdateBufferCommand : public Command
        {
            const uint32_t*                data_ptr;
            VkDeviceSize                   data_size;
            VkBuffer                       dst_buffer;
            std::shared_ptr<Anvil::Buffer> dst_buffer_ptr;
            VkDeviceSize                   dst_offset;

            /** Constructor **/
            explicit UpdateBufferCommand(std::shared_ptr<Anvil::Buffer> in_dst_buffer_ptr,
                                         VkDeviceSize                   in_dst_offset,
                                         VkDeviceSize                   in_data_size,
                                         const uint32_t*                in_data_ptr);

            /** Destructor. */
            virtual ~UpdateBufferCommand()
            {
                /* Stub */
            }

        private:
            UpdateBufferCommand& operator=(const UpdateBufferCommand&);
        } UpdateBufferCommand;


        /** Holds all arguments passed to a vkCmdWaitEvents() command. **/
        typedef struct WaitEventsCommand : public Command
        {
            VkPipelineStageFlagsVariable(dst_stage_mask);
            VkPipelineStageFlagsVariable(src_stage_mask);

            std::vector<BufferBarrier>  buffer_barriers;
            std::vector<ImageBarrier>   image_barriers;
            std::vector<MemoryBarrier>  memory_barriers;

            std::vector<VkEvent>                        events;
            std::vector<std::shared_ptr<Anvil::Event> > event_ptrs;

            /** Constructor **/
            explicit WaitEventsCommand(uint32_t                       in_event_count,
                                       std::shared_ptr<Anvil::Event>* in_event_ptrs,
                                       VkPipelineStageFlags           in_src_stage_mask,
                                       VkPipelineStageFlags           in_dst_stage_mask,
                                       uint32_t                       in_memory_barrier_count,
                                       const MemoryBarrier* const     in_memory_barrier_ptr_ptr,
                                       uint32_t                       in_buffer_memory_barrier_count,
                                       const BufferBarrier* const     in_buffer_memory_barrier_ptr_ptr,
                                       uint32_t                       in_image_memory_barrier_count,
                                       const ImageBarrier* const      in_image_memory_barrier_ptr_ptr);

            /** Destructor. */
            virtual ~WaitEventsCommand()
            {
                /* Stub */
            }

        private:
            WaitEventsCommand& operator=(const WaitEventsCommand&);
        } WaitEventsCommand;


        /** Holds all arguments passed to a vkCmdWriteTimestamp() command. **/
        typedef struct WriteTimestampCommand : public Command
        {
            VkPipelineStageFlagsVariable(pipeline_stage);

            Anvil::QueryIndex                 entry;
            std::shared_ptr<Anvil::QueryPool> query_pool_ptr;

            /** Constructor. **/
            explicit WriteTimestampCommand(VkPipelineStageFlagBits           in_pipeline_stage,
                                           std::shared_ptr<Anvil::QueryPool> in_query_pool_ptr,
                                           Anvil::QueryIndex                 in_entry);

            /** Destructor. */
            virtual ~WriteTimestampCommand()
            {
                /* Stub */
            }

        private:
            WriteTimestampCommand& operator=(const WriteTimestampCommand&);
        } WriteTimestampCommand;


        typedef std::vector<Command> Commands;

        /* Protected functions */
        explicit CommandBufferBase(std::weak_ptr<Anvil::BaseDevice>    in_device_ptr,
                                   std::shared_ptr<Anvil::CommandPool> in_parent_command_pool_ptr,
                                   CommandBufferType                   in_type);

        virtual ~CommandBufferBase();

        void cache_referenced_buffer        (std::shared_ptr<Anvil::Buffer>            in_buffer_ptr);
        void cache_referenced_command_buffer(std::shared_ptr<Anvil::CommandBufferBase> in_cmd_buffer_ptr);
        void cache_referenced_descriptor_set(std::shared_ptr<Anvil::DescriptorSet>     in_ds_ptr);
        void cache_referenced_event         (std::shared_ptr<Anvil::Event>             in_event_ptr);
        void cache_referenced_framebuffer   (std::shared_ptr<Anvil::Framebuffer>       in_fb_ptr);
        void cache_referenced_image         (std::shared_ptr<Anvil::Image>             in_image_ptr);
        void cache_referenced_query_pool    (std::shared_ptr<Anvil::QueryPool>         in_query_pool_ptr);
        void cache_referenced_renderpass    (std::shared_ptr<Anvil::RenderPass>        in_renderpass_ptr);

        #ifdef STORE_COMMAND_BUFFER_COMMANDS
            void clear_commands();
        #endif

        void clear_referenced_objects();

        /* Protected variables */
        #ifdef STORE_COMMAND_BUFFER_COMMANDS
            Commands m_commands;
        #endif

        VkCommandBuffer                   m_command_buffer;
        std::weak_ptr<Anvil::BaseDevice>  m_device_ptr;
        bool                              m_is_renderpass_active;
        std::weak_ptr<Anvil::CommandPool> m_parent_command_pool_ptr;
        bool                              m_recording_in_progress;
        CommandBufferType                 m_type;

        static bool m_command_stashing_disabled;

    private:
        /* Private type definitions */

        /* Private functions */
        CommandBufferBase           (const CommandBufferBase&);
        CommandBufferBase& operator=(const CommandBufferBase&);

        /* Private variables */
        std::vector<std::shared_ptr<Anvil::Buffer> >            m_referenced_buffers;
        std::vector<std::shared_ptr<Anvil::CommandBufferBase> > m_referenced_command_buffers;
        std::vector<std::shared_ptr<Anvil::DescriptorSet> >     m_referenced_descriptor_sets;
        std::vector<std::shared_ptr<Anvil::Event> >             m_referenced_events;
        std::vector<std::shared_ptr<Anvil::Framebuffer> >       m_referenced_framebuffers;
        std::vector<std::shared_ptr<Anvil::Image> >             m_referenced_images;
        std::vector<std::shared_ptr<Anvil::QueryPool> >         m_referenced_query_pools;
        std::vector<std::shared_ptr<Anvil::RenderPass> >        m_referenced_renderpasses;

        friend class Anvil::CommandPool;
    };

    /** Wrapper class for primary command buffers. */
    class PrimaryCommandBuffer : public CommandBufferBase
    {
    public:
        /* Public functions */

        /* Destructor */
        virtual ~PrimaryCommandBuffer()
         {
            /* Stub */
        }

        /** Issues a vkCmdBeginRenderPass() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_begin_render_pass(uint32_t                            in_n_clear_values,
                                      const VkClearValue*                 in_clear_value_ptrs,
                                      std::shared_ptr<Anvil::Framebuffer> in_fbo_ptr,
                                      VkRect2D                            in_render_area,
                                      std::shared_ptr<Anvil::RenderPass>  in_render_pass_ptr,
                                      VkSubpassContents                   in_contents);

        /** Issues a vkCmdEndRenderPass() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_end_render_pass();

        /** Issues a vkCmdExecuteCommands() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Any Vulkan object wrapper instances passed to this function are going to be retained,
         *  and will be released when the command buffer is released or resetted.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_execute_commands(uint32_t                                        in_cmd_buffers_count,
                                     std::shared_ptr<Anvil::SecondaryCommandBuffer>* in_cmd_buffers);

        /** Issues a vkCmdNextSubpass() call and appends it to the internal vector of commands
         *  recorded for the specified command buffer (for builds with STORE_COMMAND_BUFFER_COMMANDS
         *  #define enabled).
         *
         *  Calling this function for a command buffer which has not been put into a recording mode
         *  (by issuing a start_recording() call earlier) will result in an assertion failure.
         *
         *  Argument meaning is as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool record_next_subpass(VkSubpassContents in_contents);

        /** Issues a vkBeginCommandBufer() call and clears the internally managed vector of recorded
         *  commands, if STORE_COMMAND_BUFFER_COMMANDS has been defined for the build.
         *
         *  It is an error to invoke this function if recording is already in progress.
         *
         *  @param in_one_time_submit          true if the VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT flag should
         *                                     be used for the Vulkan API call.
         *  @param in_simultaneous_use_allowed true if the VK_CMD_BUFFER_OPTIMIZE_NO_SIMULTANEOUS_USE_BIT flag should
         *                                     be used for the Vulkan API call.
         *
         *  @return true if successful, false otherwise.
         **/
        bool start_recording(bool in_one_time_submit,
                             bool in_simultaneous_use_allowed);

    protected:
        /** Constructor. Should be used to instantiate primary-level command buffers.
         *
         *  NOTE: In order to create a command buffer, please call relevant alloc() functions
         *        from Anvil::CommandPool().
         *
         *  @param in_device_ptr              Device to use.
         *  @param in_parent_command_pool_ptr Command pool to use as a parent. Must not be nullptr.
         *
         **/
        PrimaryCommandBuffer(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                             std::shared_ptr<CommandPool>     in_parent_command_pool_ptr);

    private:
        friend class Anvil::CommandPool;

        /* Private functions */
        PrimaryCommandBuffer           (const PrimaryCommandBuffer&);
        PrimaryCommandBuffer& operator=(const PrimaryCommandBuffer&);
    };

    /** Wrapper class for secondary command buffers. */
    class SecondaryCommandBuffer : public CommandBufferBase
    {
    public:
        /* Public functions */

        /** Issues a vkBeginCommandBufer() call and clears the internally managed vector of recorded
         *  commands, if STORE_COMMAND_BUFFER_COMMANDS has been defined for the build.
         *
         *  The difference between this function and ::start_recording() is that this entrypoint should be
         *  used to start recording a secondary-level command buffer which will live within the specified
         *  subpass and will only render to a renderpass compatible with @param in_render_pass_ptr.
         *
         *  It is an error to invoke this function if recording is already in progress.
         *
         *  @param in_one_time_submit                        true if the VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT flag should
         *                                                   be used for the Vulkan API call.
         *  @param in_simultaneous_use_allowed               true if the VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT flag should
         *                                                   be used for the Vulkan API call.
         *  @param in_renderpass_usage_only                  true if the VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT flag should
         *                                                   be used for the Vulkan API call.
         *  @param in_framebuffer_ptr                        Meaning as per Vulkan API specification.
         *  @param in_render_pass_ptr                        Meaning as per Vulkan API specification.
         *  @param in_subpass_id                             Meaning as per Vulkan API specification.
         *  @param in_required_occlusion_query_support_scope Meaning as per OcclusionQuerySupportScope documentation.
         *  @param in_required_pipeline_statistics_scope     Meaning as per Vulkan API specification.
         *
         *  @return true if successful, false otherwise.
         **/
        bool start_recording(bool                          in_one_time_submit,
                             bool                          in_simultaneous_use_allowed,
                             bool                          in_renderpass_usage_only,
                             std::shared_ptr<Framebuffer>  in_framebuffer_ptr,
                             std::shared_ptr<RenderPass>   in_render_pass_ptr,
                             SubPassID                     in_subpass_id,
                             OcclusionQuerySupportScope    in_required_occlusion_query_support_scope,
                             bool                          in_occlusion_query_used_by_primary_command_buffer,
                             VkQueryPipelineStatisticFlags in_required_pipeline_statistics_scope);

        /* Destructor */
        virtual ~SecondaryCommandBuffer()
        {
            /* Stub */
        }
    protected:
        /** Constructor. Should be used to instantiate secondary-level command buffers.
         *
         *  @param in_device_ptr              Device to use.
         *  @param in_parent_command_pool_ptr Command pool to use as a parent. Must not be nullptr.
         *
         **/
        SecondaryCommandBuffer(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                               std::shared_ptr<CommandPool>     in_parent_command_pool_ptr); 

    private:
        friend class Anvil::CommandPool;

        /* Private functions */
        SecondaryCommandBuffer           (const SecondaryCommandBuffer&);
        SecondaryCommandBuffer& operator=(const SecondaryCommandBuffer&);
    };
}; /* namespace Anvil */

#endif /* WRAPPERS_COMMAND_BUFFER_H */
