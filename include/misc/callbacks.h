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
     *  @param callback_arg Call-back specific argument.
     *  @param user_arg     Argument, specified by the subscriber at sign-up time.
     **/
    typedef void (*PFNCALLBACKPROC)(void* callback_arg,
                                    void* user_arg);

    /** Interface which provides entrypoints that let class users sign up and sign out of
     *  notifications.
     **/
    class ICallbacksSupportClient
    {
        virtual void register_for_callbacks   (CallbackID      callback_id,
                                               PFNCALLBACKPROC pfn_callback_proc,
                                               void*           user_arg)          = 0;
        virtual void unregister_from_callbacks(CallbackID      callback_id,
                                               PFNCALLBACKPROC pfn_callback_proc,
                                               void*           user_arg)          = 0;
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
         *  @param callback_id_count Defines the number of callback slots to allocate. Valid callback ID
         *                           pool ranges from 0 to (@param callback_id_count - 1).
         *                           Must be at least 1.
         **/
        explicit CallbacksSupportProvider(CallbackID callback_id_count)
        {
            anvil_assert(callback_id_count > 0);

            m_callback_id_count = callback_id_count;
            m_callbacks         = new Callbacks[callback_id_count];
        }

        /** Destructor.
         *
         *  Throws an assertion failure if there are dangling call-back subscribers at the time this destructor
         *  is called.
         **/
        ~CallbacksSupportProvider()
        {
            if (m_callbacks != nullptr)
            {
                delete [] m_callbacks;

                m_callbacks = nullptr;
            }
        }

        /* ICallbacksSupportClient interface implementation */

        /** Registers a new call-back client.
         *
         *  Note that the function does NOT check if the specified callback func ptr + user argument
         *  has not already been registered.
         *
         *  @param callback_id       ID of the call-back slot the caller intends to sign up to. The
         *                           value must not exceed the maximum callback ID allowed by the
         *                           inheriting class.
         *  @param pfn_callback_proc Call-back handler. Must not be nullptr.
         *  @param user_arg          Optional argument to be passed with the call-back. May be nullptr.
         *
         **/
        void register_for_callbacks(CallbackID      callback_id,
                                    PFNCALLBACKPROC pfn_callback_proc,
                                    void*           user_arg)
        {
            anvil_assert(callback_id       <  m_callback_id_count);
            anvil_assert(pfn_callback_proc != nullptr);

            m_callbacks[callback_id].push_back(Callback(pfn_callback_proc,
                                                        user_arg) );
        }

        /** Unregisters the client from the specified call-back slot.
         *
         *  It is an error to try to unregister from a callback that has not been configured by
         *  a preceding register_for_callbacks() call, or which has already been unregistered.
         *  Doing so will result in an assertion failure.
         *
         *  @param callback_id       ID of the call-back slot the caller wants to sign out from.
         *                           The value must not exceed the maximum callback ID allowed by
         *                           the inheriting class.
         *  @param pfn_callback_proc Call-back handler. Must not be nullptr.
         *  @param user_arg          User argument specified for the call-back.
         *
         **/
        void unregister_from_callbacks(CallbackID      callback_id,
                                       PFNCALLBACKPROC pfn_callback_proc,
                                       void*           user_arg)
        {
            bool has_found = false;

            anvil_assert(callback_id       <  m_callback_id_count);
            anvil_assert(pfn_callback_proc != nullptr);

            for (auto callback_iterator  = m_callbacks[callback_id].begin();
                      callback_iterator != m_callbacks[callback_id].end();
                    ++callback_iterator)
            {
                if (callback_iterator->pfn_callback_proc == pfn_callback_proc &&
                    callback_iterator->user_arg          == user_arg)
                {
                    m_callbacks[callback_id].erase(callback_iterator);

                    has_found = true;
                    break;
                }
            }

            anvil_assert(has_found);
        }

    protected:
        /* Protected functions */

        /** Calls back all subscribers which have signed up for the specified callback slot.
         *
         *  The clients are called one after another from the thread, in which the call has
         *  been invoked.
         *
         *  @param callback_id  ID of the call-back slot to use.
         *  @param callback_arg Call-back argument to use.
         **/
        void callback(CallbackID callback_id,
                      void*      callback_arg)
        {
            anvil_assert(callback_id < m_callback_id_count);

            const uint32_t n_callbacks = (uint32_t) m_callbacks[callback_id].size();

            for (auto callback_iterator  = m_callbacks[callback_id].begin();
                      callback_iterator != m_callbacks[callback_id].end();
                    ++callback_iterator)
            {
                const Callback& current_callback = *callback_iterator;

                current_callback.pfn_callback_proc(callback_arg,
                                                   current_callback.user_arg);
            }
        }

        /** Tells how many subscribers have registered for the specified callback */
        uint32_t get_n_of_callback_subscribers(CallbackID callback_id) const
        {
            uint32_t result = 0;

            if (callback_id < m_callback_id_count)
            {
                result = static_cast<uint32_t>(m_callbacks[callback_id].size() );
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
        } Callback;

        typedef std::vector<Callback> Callbacks;

        /* Private variables */
        CallbackID m_callback_id_count;
        Callbacks* m_callbacks;
    };
} /* namespace Anvil */

#endif /* MISC_CALLBACKS_H */
