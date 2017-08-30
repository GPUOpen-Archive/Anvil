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
#include <stdint.h>
#include <vector>
#ifndef _WIN32
    #include <unistd.h>
#endif

#include "misc/types.h"
#include <algorithm>

namespace Anvil
{
    /* Defines the callback ID type.
     *
     * Each class which inherits from CallbacksSupportProvider uses its own range of callback IDs.
     * Please consult the documentation for more details.
     **/
    typedef int CallbackID;

    /** Prototype of a call-back handler.
     *
     *  @param in_callback_arg Call-back specific argument.
     *  @param in_user_arg     Argument, specified by the subscriber at sign-up time.
     **/
    typedef void (*PFNCALLBACKPROC)(void* in_callback_arg,
                                    void* in_user_arg);

    /** Interface which provides entrypoints that let class users sign up and sign out of
     *  notifications.
     **/
    class ICallbacksSupportClient
    {
        virtual bool is_callback_registered   (CallbackID      in_callback_id,
                                               PFNCALLBACKPROC in_pfn_callback_proc,
                                               void*           in_user_arg) const    = 0;
        virtual void register_for_callbacks   (CallbackID      in_callback_id,
                                               PFNCALLBACKPROC in_pfn_callback_proc,
                                               void*           in_user_arg)          = 0;
        virtual void unregister_from_callbacks(CallbackID      in_callback_id,
                                               PFNCALLBACKPROC in_pfn_callback_proc,
                                               void*           in_user_arg)          = 0;
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
            if (m_callbacks != nullptr)
            {
                delete [] m_callbacks;

                m_callbacks = nullptr;
            }
        }

        /* ICallbacksSupportClient interface implementation */

        /* Tells whether a given callback has already been registered
         *
         * @param in_callback_id       ID of the call-back slot. Must not exceed the maximum callback ID
         *                             allowed by the inheriting class.
         * @param in_pfn_callback_proc Callback handler.
         * @param in_user_arg          Optional argument to be passed with the callback.
         *
         * @return true if a callback with user-specified parameters has already been registered,
         *         false otherwise.
         */
        bool is_callback_registered(CallbackID      in_callback_id,
                                    PFNCALLBACKPROC in_pfn_callback_proc,
                                    void*           in_user_arg) const
        {
            Callback callback = Callback(in_pfn_callback_proc,
                                         in_user_arg);

            anvil_assert(in_callback_id < m_callback_id_count);

            return std::find(m_callbacks[in_callback_id].begin(),
                             m_callbacks[in_callback_id].end(),
                             callback) != m_callbacks[in_callback_id].end();
        }

        /** Registers a new call-back client.
         *
         *  Note that the function does NOT check if the specified callback func ptr + user argument
         *  has not already been registered.
         *
         *  @param in_callback_id       ID of the call-back slot the caller intends to sign up to. The
         *                              value must not exceed the maximum callback ID allowed by the
         *                              inheriting class.
         *  @param in_pfn_callback_proc Call-back handler. Must not be nullptr.
         *  @param in_user_arg          Optional argument to be passed with the call-back. May be nullptr.
         *
         **/
        void register_for_callbacks(CallbackID      in_callback_id,
                                    PFNCALLBACKPROC in_pfn_callback_proc,
                                    void*           in_user_arg)
        {
            Callback new_callback = Callback(in_pfn_callback_proc,
                                             in_user_arg);

            anvil_assert(in_callback_id       <  m_callback_id_count);
            anvil_assert(in_pfn_callback_proc != nullptr);
            anvil_assert(!m_callbacks_locked);

            anvil_assert(std::find(m_callbacks[in_callback_id].begin(),
                                   m_callbacks[in_callback_id].end(),
                                   new_callback) == m_callbacks[in_callback_id].end() );

            m_callbacks[in_callback_id].push_back(new_callback);
        }

        /** Unregisters the client from the specified call-back slot.
         *
         *  It is an error to try to unregister from a callback that has not been configured by
         *  a preceding register_for_callbacks() call, or which has already been unregistered.
         *  Doing so will result in an assertion failure.
         *
         *  @param in_callback_id       ID of the call-back slot the caller wants to sign out from.
         *                              The value must not exceed the maximum callback ID allowed by
         *                              the inheriting class.
         *  @param in_pfn_callback_proc Call-back handler. Must not be nullptr.
         *  @param in_user_arg          User argument specified for the call-back.
         *
         **/
        void unregister_from_callbacks(CallbackID      in_callback_id,
                                       PFNCALLBACKPROC in_pfn_callback_proc,
                                       void*           in_user_arg)
        {
            bool has_found = false;

            anvil_assert(in_callback_id       <  m_callback_id_count);
            anvil_assert(in_pfn_callback_proc != nullptr);
            anvil_assert(!m_callbacks_locked);

            for (auto callback_iterator  = m_callbacks[in_callback_id].begin();
                      callback_iterator != m_callbacks[in_callback_id].end();
                    ++callback_iterator)
            {
                if (callback_iterator->pfn_callback_proc == in_pfn_callback_proc &&
                    callback_iterator->user_arg          == in_user_arg)
                {
                    m_callbacks[in_callback_id].erase(callback_iterator);

                    has_found = true;
                    break;
                }
            }

            anvil_assert            (has_found);
            ANVIL_REDUNDANT_VARIABLE(has_found);
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
         *  @param in_callback_id  ID of the call-back slot to use.
         *  @param in_callback_arg Call-back argument to use.
         **/
        void callback(CallbackID in_callback_id,
                      void*      in_callback_arg)
        {
            anvil_assert(in_callback_id < m_callback_id_count);
            anvil_assert(!m_callbacks_locked);

            m_callbacks_locked = true;
            {
                for (auto callback_iterator  = m_callbacks[in_callback_id].begin();
                          callback_iterator != m_callbacks[in_callback_id].end();
                        ++callback_iterator)
                {
                    const Callback& current_callback = *callback_iterator;

                    current_callback.pfn_callback_proc(in_callback_arg,
                                                       current_callback.user_arg);
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
         *  @param in_callback_id  ID of the call-back slot to use.
         *  @param in_callback_arg Call-back argument to use.
         **/
        void callback_safe(CallbackID in_callback_id,
                           void*      in_callback_arg)
        {
            anvil_assert(in_callback_id < m_callback_id_count);
            anvil_assert(!m_callbacks_locked);

            bool                  another_iteration_needed = true;
            std::vector<Callback> invoked_callbacks;

            while (another_iteration_needed)
            {
                const std::vector<Callback> cached_callbacks = m_callbacks[in_callback_id];

                another_iteration_needed = false;

                for (uint32_t n_current_callback = 0;
                              n_current_callback < static_cast<uint32_t>(cached_callbacks.size() );
                            ++n_current_callback)
                {
                    const Callback& current_callback = cached_callbacks[n_current_callback];

                    if (std::find(invoked_callbacks.begin(),
                                  invoked_callbacks.end(),
                                  current_callback) == invoked_callbacks.end() )
                    {
                        current_callback.pfn_callback_proc(in_callback_arg,
                                                           current_callback.user_arg);

                        invoked_callbacks.push_back(current_callback);
                    }

                    if (cached_callbacks != m_callbacks[in_callback_id])
                    {
                        another_iteration_needed = true;
                        break;
                    }
                }

                if (!another_iteration_needed)
                {
                    break;
                }
            }
        }

        /** Tells how many subscribers have registered for the specified callback */
        uint32_t get_n_of_callback_subscribers(CallbackID in_callback_id) const
        {
            uint32_t result = 0;

            if (in_callback_id < m_callback_id_count)
            {
                result = static_cast<uint32_t>(m_callbacks[in_callback_id].size() );
            }

            return result;
        }

    private:
        /* Private type definitions */

        /** Describes an individual callback registration */
        typedef struct Callback
        {
            PFNCALLBACKPROC pfn_callback_proc;
            void*           user_arg;

            /* Constructor. Should only be use by STL. **/
            Callback()
            {
                pfn_callback_proc = nullptr;
                user_arg          = nullptr;
            }

            /** Constructor.
             *
             *  @param in_pfn_callback_proc Function pointer to the call-back handler.
             *  @param in_user_arg          User argument to use for the call-back.
             **/
            Callback(PFNCALLBACKPROC in_pfn_callback_proc,
                     void*           in_user_arg)
            {
                pfn_callback_proc = in_pfn_callback_proc;
                user_arg          = in_user_arg;
            }

            bool operator==(const Callback& in) const
            {
                return (in.pfn_callback_proc == pfn_callback_proc &&
                        in.user_arg          == user_arg);
            }
        } Callback;

        typedef std::vector<Callback> Callbacks;

        /* Private variables */
        CallbackID m_callback_id_count;
        Callbacks* m_callbacks;
        bool       m_callbacks_locked;
    };
} /* namespace Anvil */

#endif /* MISC_CALLBACKS_H */
