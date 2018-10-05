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
    
/** Defines & implements a simple callback manager which lets:
 *
 *  - clients sign up to any of the exposed callback slots at any time.
 *  - the deriving class specify the range of callback IDs clients can sign up to at creation time.
 *  - the deriving class call back all subscribers signed up to the specified callback slot at any
 *    time. In this case, subscribers are called one after another from the same thread, from which
 *    the notification is coming from.
 **/
#ifndef MISC_CALLBACKS_H
#define MISC_CALLBACKS_H

#include "misc/debug.h"
#include "misc/io.h"
#include "misc/types.h"

#ifndef _WIN32
    #include <unistd.h>
#endif

#include <algorithm>

namespace Anvil
{
    /* Helper forward declarations .. */
    struct PipelineBarrierCommand;


    /** Prototype of a call-back handler.
     *
     *  @param in_callback_arg Call-back specific argument.
     **/
    typedef std::function< void(CallbackArgument* in_callback_arg) > CallbackFunction;

    /* Defines the callback ID type.
     *
     * Each class which inherits from CallbacksSupportProvider uses its own range of callback IDs.
     * Please consult the documentation for more details.
     **/
    typedef int CallbackID;

    /* Base call-back argument structure. All call-back arguments are required to derive off this class. */
    struct CallbackArgument
    {
        virtual ~CallbackArgument()
        {
            /* Stub */
        }
    };


    typedef struct IsBufferMemoryAllocPendingQueryCallbackArgument : public Anvil::CallbackArgument
    {
        const Anvil::Buffer* buffer_ptr;
        bool                 result;

        explicit IsBufferMemoryAllocPendingQueryCallbackArgument(const Anvil::Buffer* in_buffer_ptr)
            :buffer_ptr(in_buffer_ptr),
             result    (false)
        {
            /* Stub */
        }

        IsBufferMemoryAllocPendingQueryCallbackArgument& operator=(const IsBufferMemoryAllocPendingQueryCallbackArgument&) = delete;
    } IsBufferMemoryAllocPendingQueryCallbackArgument;

    typedef struct IsImageMemoryAllocPendingQueryCallbackArgument : Anvil::CallbackArgument
    {
        explicit IsImageMemoryAllocPendingQueryCallbackArgument(const Anvil::Image* in_image_ptr)
            :image_ptr(in_image_ptr),
             result   (false)
        {
            /* Stub */
        }

        IsImageMemoryAllocPendingQueryCallbackArgument& operator=(const IsImageMemoryAllocPendingQueryCallbackArgument&) = delete;

        const Anvil::Image* image_ptr;
        bool                result;
    } IsImageMemoryAllocPendingQueryCallbackArgument;

    typedef struct OnDescriptorPoolResetCallbackArgument : public Anvil::CallbackArgument
    {
        const DescriptorPool* descriptor_pool_ptr;

        explicit OnDescriptorPoolResetCallbackArgument(DescriptorPool* in_descriptor_pool_ptr)
        {
            descriptor_pool_ptr = in_descriptor_pool_ptr;
        }
    } OnDescriptorPoolResetCallbackArgument;

    typedef struct OnGLSLToSPIRVConversionAboutToBeStartedCallbackArgument : public Anvil::CallbackArgument
    {
        const GLSLShaderToSPIRVGenerator* generator_ptr;

        explicit OnGLSLToSPIRVConversionAboutToBeStartedCallbackArgument(const GLSLShaderToSPIRVGenerator* in_generator_ptr)
        {
            generator_ptr = in_generator_ptr;
        }
    } OnGLSLToSPIRVConversionAboutToBeStartedCallbackArgument;

    typedef OnGLSLToSPIRVConversionAboutToBeStartedCallbackArgument OnGLSLToSPIRVConversionFinishedCallbackArgument;

    typedef struct OnKeypressReleasedCallbackArgument : public Anvil::CallbackArgument
    {
        KeyID         released_key_id;
        const Window* window_ptr;

        /** Constructor.
         *
         *  @param in_command_buffer_ptr  Command buffer instance the command is being recorded for.
         *  @param in_command_details_ptr Structure holding all arguments to be passed to the vkCmdBeginRenderPass() call.
         **/
        explicit OnKeypressReleasedCallbackArgument(Window* in_window_ptr,
                                                    KeyID   in_released_key_id)
            :released_key_id(in_released_key_id),
             window_ptr     (in_window_ptr)
        {
            /* Stub */
        }
    } OnKeypressReleasedCallbackArgument;

    typedef struct OnMemoryBlockNeededForBufferCallbackArgument : public Anvil::CallbackArgument
    {
        explicit OnMemoryBlockNeededForBufferCallbackArgument(const Anvil::Buffer* in_buffer_ptr)
            :buffer_ptr(in_buffer_ptr)
        {
            /* Stub */
        }

        OnMemoryBlockNeededForBufferCallbackArgument& operator=(const OnMemoryBlockNeededForBufferCallbackArgument&) = delete;

        const Anvil::Buffer* buffer_ptr;
    } OnMemoryBlockNeededForBufferCallbackArgument;

    typedef struct OnMemoryBlockNeededForImageCallbackArgument : public Anvil::CallbackArgument
    {
        explicit OnMemoryBlockNeededForImageCallbackArgument(const Anvil::Image* in_image_ptr)
            :image_ptr(in_image_ptr)
        {
            /* Stub */
        }

        OnMemoryBlockNeededForImageCallbackArgument& operator=(const OnMemoryBlockNeededForImageCallbackArgument&) = delete;

        const Anvil::Image* image_ptr;
    } OnMemoryBlockNeededForImageCallbackArgument;

    typedef struct OnNewBindingAddedToDescriptorSetLayoutCallbackArgument : public Anvil::CallbackArgument
    {
        const DescriptorSetLayout* descriptor_set_layout_ptr;

        explicit OnNewBindingAddedToDescriptorSetLayoutCallbackArgument(const DescriptorSetLayout* in_descriptor_set_layout_ptr)
        {
            descriptor_set_layout_ptr = in_descriptor_set_layout_ptr;
        }
    } OnNewBindingAddedToDescriptorSetLayoutCallbackArgument;

    typedef struct OnNewPipelineCreatedCallbackData : public Anvil::CallbackArgument
    {
        PipelineID new_pipeline_id;

        explicit OnNewPipelineCreatedCallbackData(PipelineID in_new_pipeline_id)
        {
            new_pipeline_id = in_new_pipeline_id;
        }
    } OnNewPipelineCreatedCallbackData;

    typedef struct OnObjectRegisteredCallbackArgument : Anvil::CallbackArgument
    {
        void*      object_raw_ptr;
        ObjectType object_type;

        explicit OnObjectRegisteredCallbackArgument(const ObjectType& in_object_type,
                                                    void*             in_object_raw_ptr)
        {
            anvil_assert(in_object_raw_ptr != nullptr);

            object_raw_ptr = in_object_raw_ptr;
            object_type    = in_object_type;
        }
    } OnObjectRegisteredCallbackArgument;

    typedef OnObjectRegisteredCallbackArgument OnObjectAboutToBeUnregisteredCallbackArgument;

    typedef struct OnPipelineBarrierCommandRecordedCallbackData : public Anvil::CallbackArgument
    {
        CommandBufferBase*            command_buffer_ptr;
        const PipelineBarrierCommand* command_details_ptr;

        /** Constructor.
         *
         *  @param in_command_buffer_ptr  Command buffer instance the command is being recorded for.
         *  @param in_command_details_ptr Structure holding all arguments to be passed to the vkCmdPipelineBarrier() call.
         **/
        explicit OnPipelineBarrierCommandRecordedCallbackData(CommandBufferBase*            in_command_buffer_ptr,
                                                              const PipelineBarrierCommand* in_command_details_ptr)
            :command_buffer_ptr (in_command_buffer_ptr),
             command_details_ptr(in_command_details_ptr) 
        {
            /* Stub */
        }
    } OnPipelineBarrierCommandRecordedCallbackData;

    typedef struct OnPresentRequestIssuedCallbackArgument : public Anvil::CallbackArgument
    {
        const Swapchain* swapchain_ptr;

        explicit OnPresentRequestIssuedCallbackArgument(const Swapchain* in_swapchain_ptr)
        {
            swapchain_ptr = in_swapchain_ptr;
        }
    } OnPresentRequestIssuedCallbackArgument;

    typedef struct OnRenderPassBakeNeededCallbackArgument : public Anvil::CallbackArgument
    {
        const RenderPass* renderpass_ptr;

        explicit OnRenderPassBakeNeededCallbackArgument(const RenderPass* in_renderpass_ptr)
        {
            renderpass_ptr = in_renderpass_ptr;
        }
    } OnRenderPassBakeNeededCallbackArgument;

    typedef struct OnWindowAboutToCloseCallbackArgument : public Anvil::CallbackArgument
    {
        const Window* window_ptr;

        explicit OnWindowAboutToCloseCallbackArgument(const Window* in_window_ptr)
        {
            window_ptr = in_window_ptr;
        }
    } OnWindowAboutToCloseCallbackArgument;


    /** Interface which provides entrypoints that let class users sign up and sign out of
     *  notifications.
     **/
    class ICallbacksSupportClient
    {
        virtual bool is_callback_registered   (CallbackID       in_callback_id,
                                               CallbackFunction in_callback_function,
                                               void*            in_callback_function_owner_ptr) const = 0;
        virtual void register_for_callbacks   (CallbackID       in_callback_id,
                                               CallbackFunction in_callback_function,
                                               void*            in_callback_function_owner_ptr)       = 0;
        virtual void unregister_from_callbacks(CallbackID       in_callback_id,
                                               CallbackFunction in_callback_function,
                                               void*            in_callback_function_owner_ptr)       = 0;
    };


    /** Provides call-back support for inheriting classes. Please see wrappers/callbacks.h header for
     *  more details.
     **/
    class CallbacksSupportProvider : public ICallbacksSupportClient
    {
    public:
        /* Public functions */

        /** Constructor.
         *
         *  @param in_callback_id_count Defines the number of callback slots to allocate. Valid callback ID
         *                              pool ranges from 0 to (@param in_callback_id_count - 1).
         *                              Must be at least 1.
         **/
        explicit CallbacksSupportProvider(CallbackID in_callback_id_count)
        {
            anvil_assert(in_callback_id_count > 0);

            m_callback_id_count = in_callback_id_count;
            m_callbacks         = new Callbacks[static_cast<uintptr_t>(in_callback_id_count)];
            m_callbacks_locked  = false;
        }

        /** Destructor.
         *
         *  Throws an assertion failure if there are dangling call-back subscribers at the time this destructor
         *  is called.
         **/
        virtual ~CallbacksSupportProvider()
        {
            delete [] m_callbacks;

            m_callbacks = nullptr;
        }

        /* ICallbacksSupportClient interface implementation */

        /* Tells whether a given callback has already been registered
         *
         * @param in_callback_id                 ID of the call-back slot. Must not exceed the maximum callback ID
         *                                       allowed by the inheriting class.
         * @param in_callback_function           Callback handler.
         * @param in_callback_function_owner_ptr Callback owner, as specified at registration time.
         *
         * @return true if a callback with user-specified parameters has already been registered,
         *         false otherwise.
         */
        bool is_callback_registered(CallbackID       in_callback_id,
                                    CallbackFunction in_callback_function,
                                    void*            in_callback_function_owner_ptr) const
        {
            std::unique_lock<std::recursive_mutex> mutex_lock(m_mutex);

            anvil_assert(in_callback_id < m_callback_id_count);

            return std::find(m_callbacks[in_callback_id].begin(),
                             m_callbacks[in_callback_id].end(),
                             Callback(in_callback_function,
                                      in_callback_function_owner_ptr) ) != m_callbacks[in_callback_id].end();
        }

        /** Registers a new call-back client.
         *
         *  Note that the function does NOT check if the specified callback func ptr + user argument
         *  has not already been registered.
         *
         *  @param in_callback_id        ID of the call-back slot the caller intends to sign up to. The
         *                               value must not exceed the maximum callback ID allowed by the
         *                               inheriting class.
         *  @param in_callback_function  Call-back handler. Must not be nullptr.
         *  @param in_callback_owner_ptr Pointer to the object which is going to own the callback. This
         *                               is required for correct identification of the callback at is_registered()
         *                               or unregister() call time. Must not be null
         *
         **/
        void register_for_callbacks(CallbackID       in_callback_id,
                                    CallbackFunction in_callback_function,
                                    void*            in_callback_owner_ptr)
        {
            std::unique_lock<std::recursive_mutex> mutex_lock(m_mutex);

            anvil_assert(in_callback_id        <  m_callback_id_count);
            anvil_assert(in_callback_function  != nullptr);
            anvil_assert(in_callback_owner_ptr != nullptr);
            anvil_assert(!m_callbacks_locked);

            #ifdef _DEBUG
            {
                anvil_assert(!is_callback_registered(in_callback_id,
                                                     in_callback_function,
                                                     in_callback_owner_ptr) );
            }
            #endif

            m_callbacks[in_callback_id].push_back(
                Callback(in_callback_function,
                         in_callback_owner_ptr)
            );
        }

        /** Unregisters the client from the specified call-back slot.
         *
         *  It is an error to try to unregister from a callback that has not been configured by
         *  a preceding register_for_callbacks() call, or which has already been unregistered.
         *  Doing so will result in an assertion failure.
         *
         *  @param in_callback_id                 ID of the call-back slot the caller wants to sign out from.
         *                                        The value must not exceed the maximum callback ID allowed by
         *                                        the inheriting class.
         *  @param in_callback_function           Call-back handler. Must not be nullptr.
         *  @param in_callback_function_owner_ptr Call-back owner, as specified at registration time.
         *
         **/
        void unregister_from_callbacks(CallbackID       in_callback_id,
                                       CallbackFunction in_callback_function,
                                       void*            in_callback_function_owner_ptr)
        {
            std::unique_lock<std::recursive_mutex> mutex_lock(m_mutex);

            anvil_assert(in_callback_id       <  m_callback_id_count);
            anvil_assert(in_callback_function != nullptr);
            anvil_assert(!m_callbacks_locked);

            auto callback_iterator = std::find(m_callbacks[in_callback_id].begin(),
                                               m_callbacks[in_callback_id].end(),
                                               Callback(in_callback_function,
                                                        in_callback_function_owner_ptr) );

            anvil_assert(callback_iterator != m_callbacks[in_callback_id].end() );
            if (callback_iterator != m_callbacks[in_callback_id].end() )
            {
                m_callbacks[in_callback_id].erase(callback_iterator);
            }
        }

    protected:
        /* Protected functions */

        /** Calls back all subscribers which have signed up for the specified callback slot.
         *
         *  The clients are called one after another from the thread, in which the call has
         *  been invoked.
         *
         *  This implementation assumes that the invoked functions will NOT alter the
         *  callback array. If that is the case, use (slower) callback_safe() instead.
         *
         *  @param in_callback_id      ID of the call-back slot to use.
         *  @param in_callback_arg_ptr Call-back argument to use.
         **/
        void callback(CallbackID        in_callback_id,
                      CallbackArgument* in_callback_arg_ptr) const
        {
            std::unique_lock<std::recursive_mutex> mutex_lock(m_mutex);

            anvil_assert(in_callback_id < m_callback_id_count);
            anvil_assert(!m_callbacks_locked);

            m_callbacks_locked = true;
            {
                for (auto callback_iterator  = m_callbacks[in_callback_id].begin();
                          callback_iterator != m_callbacks[in_callback_id].end();
                        ++callback_iterator)
                {
                    const auto& current_callback = *callback_iterator;

                    current_callback.function(in_callback_arg_ptr);
                }
            }
            m_callbacks_locked = false;
        }

        /** Calls back all subscribers which have signed up for the specified callback slot.
         *
         *  The clients are called one after another from the thread, in which the call has
         *  been invoked.
         *
         *  This implementation assumes that the invoked functions MAY alter the
         *  callback array. It will go the extra mile to ensure the cached callbacks are not
         *  called more than once, and will take note of any new callback subscriptions that
         *  may have been added by the called back functions.
         *
         *  This implementation is NOT MT-safe.
         *
         *  This function can potentially take a long time to execute.
         *
         *
         *  @param in_callback_id  ID of the call-back slot to use.
         *  @param in_callback_arg Call-back argument to use.
         **/
        void callback_safe(CallbackID        in_callback_id,
                           CallbackArgument* in_callback_arg_ptr)
        {
            std::unique_lock<std::recursive_mutex> mutex_lock(m_mutex);

            anvil_assert(in_callback_id < m_callback_id_count);
            anvil_assert(!m_callbacks_locked);

            bool                  another_iteration_needed = true;
            bool                  first_iteration          = true;
            std::vector<Callback> invoked_callbacks;

            while (another_iteration_needed                &&
                   m_callbacks[in_callback_id].size() > 0)
            {
                const std::vector<Callback> cached_callbacks = m_callbacks[in_callback_id];

                another_iteration_needed = false;

                for (uint32_t n_current_callback = 0;
                              n_current_callback < static_cast<uint32_t>(cached_callbacks.size() );
                            ++n_current_callback)
                {
                    const auto& current_callback = cached_callbacks[n_current_callback];

                    if (first_iteration                                        ||
                        std::find(invoked_callbacks.begin(),
                                  invoked_callbacks.end(),
                                  current_callback) == invoked_callbacks.end() )
                    {
                        current_callback.function(in_callback_arg_ptr);

                        invoked_callbacks.push_back(current_callback);
                    }
                }

                /* Has m_callbacks[in_callback_id] changed as a result of the callback above? */
                if (!(m_callbacks[in_callback_id] == invoked_callbacks) )
                {
                    another_iteration_needed = true;
                }

                first_iteration = false;
            }
        }

        /** Tells how many subscribers have registered for the specified callback */
        uint32_t get_n_of_callback_subscribers(CallbackID in_callback_id) const
        {
            uint32_t result = 0;

            if (in_callback_id < m_callback_id_count)
            {
                std::unique_lock<std::recursive_mutex> mutex_lock(m_mutex);

                result = static_cast<uint32_t>(m_callbacks[in_callback_id].size() );
            }

            return result;
        }

    private:
        /* Private type definitions */
        typedef struct Callback
        {
            CallbackFunction function;
            void*            magic;

            explicit Callback(CallbackFunction in_function,
                              void*            in_magic)
            {
                function = in_function;
                magic    = in_magic;
            }

            bool operator==(const Callback& in_callback) const
            {
                const auto& this_target_type = function.target_type();
                const auto& this_target      = function.target<void(*)(void*)> ();

                const auto& in_target_type   = in_callback.function.target_type();
                const auto& in_target        = in_callback.function.target<void(*)(void*)> ();

                return (this_target_type == in_target_type       &&
                        this_target      == in_target            &&
                        magic            == in_callback.magic);
            }
        } Callback;

        typedef std::vector<Callback> Callbacks;

        /* Private variables */
        CallbackID                   m_callback_id_count;
        Callbacks*                   m_callbacks;
        mutable volatile bool        m_callbacks_locked;
        mutable std::recursive_mutex m_mutex;
    };
} /* namespace Anvil */

#endif /* MISC_CALLBACKS_H */
