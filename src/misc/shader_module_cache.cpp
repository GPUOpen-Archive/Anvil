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
#include "misc/debug.h"
#include "misc/object_tracker.h"
#include "misc/shader_module_cache.h"
#include "wrappers/shader_module.h"

/** Please see header for documentation */
Anvil::ShaderModuleCache::ShaderModuleCache()
{
    /* Stub */
}

/** Please see header for documentation */
Anvil::ShaderModuleCache::~ShaderModuleCache()
{
    std::unique_lock<std::mutex> lock(m_cs);
    {
        auto object_tracker_ptr = Anvil::ObjectTracker::get();

        object_tracker_ptr->unregister_from_callbacks(OBJECT_TRACKER_CALLBACK_ID_ON_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                                                      on_object_about_to_be_released,
                                                      this);
        object_tracker_ptr->unregister_from_callbacks(OBJECT_TRACKER_CALLBACK_ID_ON_OBJECT_REGISTERED,
                                                      on_object_registered,
                                                      this);
    }
}

/** TODO */
void Anvil::ShaderModuleCache::cache(std::shared_ptr<Anvil::ShaderModule> in_shader_module_ptr)
{
    const auto shader_module_cs_entrypoint_name = in_shader_module_ptr->get_cs_entrypoint_name();
    const auto shader_module_device_ptr         = in_shader_module_ptr->get_parent_device     ();
    const auto shader_module_fs_entrypoint_name = in_shader_module_ptr->get_fs_entrypoint_name();
    const auto shader_module_gs_entrypoint_name = in_shader_module_ptr->get_gs_entrypoint_name();
    const auto shader_module_spirv_blob         = in_shader_module_ptr->get_spirv_blob();
    const auto shader_module_tc_entrypoint_name = in_shader_module_ptr->get_tc_entrypoint_name();
    const auto shader_module_te_entrypoint_name = in_shader_module_ptr->get_te_entrypoint_name();
    const auto shader_module_vs_entrypoint_name = in_shader_module_ptr->get_vs_entrypoint_name();

    const size_t hash(get_hash(reinterpret_cast<const char*>(&shader_module_spirv_blob.at(0) ),
                               static_cast<uint32_t>(shader_module_spirv_blob.size() * sizeof(shader_module_spirv_blob.at(0) )),
                               shader_module_cs_entrypoint_name,
                               shader_module_fs_entrypoint_name,
                               shader_module_gs_entrypoint_name,
                               shader_module_tc_entrypoint_name,
                               shader_module_te_entrypoint_name,
                               shader_module_vs_entrypoint_name) );

    anvil_assert(in_shader_module_ptr != nullptr);

    {
        std::unique_lock<std::mutex> lock(m_cs);

        auto items_map_iterator    = m_items.find(hash);
        bool should_store_new_item = false;

        if (items_map_iterator == m_items.end() )
        {
            /* No collision case */
            should_store_new_item = true;
        }
        else
        {
            /* The item we are being asked to cache might be already there. Make sure this is not the case
             * before stashing the new structure.
             */
            bool  item_found = false;
            auto& item_list  = items_map_iterator->second;

            for (const auto& current_item : item_list)
            {
                if (current_item.matches(shader_module_device_ptr,
                                         reinterpret_cast<const char*>(&shader_module_spirv_blob.at(0) ),
                                         static_cast<uint32_t>(shader_module_spirv_blob.size() * sizeof(shader_module_spirv_blob.at(0) )),
                                         shader_module_cs_entrypoint_name,
                                         shader_module_fs_entrypoint_name,
                                         shader_module_gs_entrypoint_name,
                                         shader_module_tc_entrypoint_name,
                                         shader_module_te_entrypoint_name,
                                         shader_module_vs_entrypoint_name) )
                {
                    /* This assertion check should never explode */
                    anvil_assert(current_item.shader_module_ptr == in_shader_module_ptr);

                    item_found = true;
                    break;
                }
            }

            should_store_new_item = !item_found;
        }

        if (should_store_new_item)
        {
            auto new_item = HashMapItem(shader_module_device_ptr,
                                        shader_module_spirv_blob,
                                        shader_module_cs_entrypoint_name,
                                        shader_module_fs_entrypoint_name,
                                        shader_module_gs_entrypoint_name,
                                        shader_module_tc_entrypoint_name,
                                        shader_module_te_entrypoint_name,
                                        shader_module_vs_entrypoint_name,
                                        in_shader_module_ptr);

            m_items[hash].push_front(new_item);
        }
    }
}

std::shared_ptr<Anvil::ShaderModuleCache> Anvil::ShaderModuleCache::create()
{
    auto                                      object_tracker_ptr = Anvil::ObjectTracker::get();
    std::shared_ptr<Anvil::ShaderModuleCache> result_ptr;

    result_ptr.reset(
        new Anvil::ShaderModuleCache()
    );

    /* Sign up for object creation / release notifications. We're going to need this in order to cache shader module instances
     * which are created by the app. */
    object_tracker_ptr->register_for_callbacks(OBJECT_TRACKER_CALLBACK_ID_ON_OBJECT_ABOUT_TO_BE_UNREGISTERED,
                                               on_object_about_to_be_released,
                                               result_ptr.get() );
    object_tracker_ptr->register_for_callbacks(OBJECT_TRACKER_CALLBACK_ID_ON_OBJECT_REGISTERED,
                                               on_object_registered,
                                               result_ptr.get() );

    return result_ptr;
}

/** Please see header for documentation */
std::shared_ptr<Anvil::ShaderModule> Anvil::ShaderModuleCache::get_cached_shader_module(std::weak_ptr<Anvil::BaseDevice> in_device_ptr,
                                                                                        const char*                      in_spirv_blob,
                                                                                        uint32_t                         in_n_spirv_blob_bytes,
                                                                                        const std::string&               in_cs_entrypoint_name,
                                                                                        const std::string&               in_fs_entrypoint_name,
                                                                                        const std::string&               in_gs_entrypoint_name,
                                                                                        const std::string&               in_tc_entrypoint_name,
                                                                                        const std::string&               in_te_entrypoint_name,
                                                                                        const std::string&               in_vs_entrypoint_name)
{
    std::shared_ptr<Anvil::ShaderModule> result_ptr;

    {
        std::unique_lock<std::mutex> lock(m_cs);

        const auto hash              (get_hash(in_spirv_blob,
                                               in_n_spirv_blob_bytes,
                                               in_cs_entrypoint_name,
                                               in_fs_entrypoint_name,
                                               in_gs_entrypoint_name,
                                               in_tc_entrypoint_name,
                                               in_te_entrypoint_name,
                                               in_vs_entrypoint_name) );
        auto       items_map_iterator(m_items.find(hash) );

        if (items_map_iterator != m_items.end() )
        {
            const auto& items = items_map_iterator->second;

            for (const auto& current_item : items)
            {
                if (current_item.matches(in_device_ptr,
                                         in_spirv_blob,
                                         in_n_spirv_blob_bytes,
                                         in_cs_entrypoint_name,
                                         in_fs_entrypoint_name,
                                         in_gs_entrypoint_name,
                                         in_tc_entrypoint_name,
                                         in_te_entrypoint_name,
                                         in_vs_entrypoint_name) )
                {
                    result_ptr = current_item.shader_module_ptr;
                    break;
                }
            }
        }
    }

    return result_ptr;
}

/** TODO */
size_t Anvil::ShaderModuleCache::get_hash(const char*        in_spirv_blob,
                                          uint32_t           in_n_spirv_blob_bytes,
                                          const std::string& in_cs_entrypoint_name,
                                          const std::string& in_fs_entrypoint_name,
                                          const std::string& in_gs_entrypoint_name,
                                          const std::string& in_tc_entrypoint_name,
                                          const std::string& in_te_entrypoint_name,
                                          const std::string& in_vs_entrypoint_name) const
{
    std::hash<std::string> hash_string;
    std::hash<uint32_t>    hash_uint32;
    const uint32_t         n_spirv_blob_ops      = in_n_spirv_blob_bytes / sizeof(uint32_t);
    size_t                 result_hash           = 0;
    const uint32_t*        spirv_blob_uint32_ptr = reinterpret_cast<const uint32_t*>(in_spirv_blob);

    anvil_assert((in_n_spirv_blob_bytes % sizeof(uint32_t)) == 0);

    for (uint32_t n_spirv_blob_op = 0;
                  n_spirv_blob_op < n_spirv_blob_ops;
                ++n_spirv_blob_op)
    {
        result_hash ^= hash_uint32(spirv_blob_uint32_ptr[n_spirv_blob_op]);
    }

    result_hash ^= hash_string(in_cs_entrypoint_name);
    result_hash ^= hash_string(in_fs_entrypoint_name);
    result_hash ^= hash_string(in_gs_entrypoint_name);
    result_hash ^= hash_string(in_tc_entrypoint_name);
    result_hash ^= hash_string(in_te_entrypoint_name);
    result_hash ^= hash_string(in_vs_entrypoint_name);

    return result_hash;
}

/** TODO */
void Anvil::ShaderModuleCache::on_object_about_to_be_released(void* in_callback_arg,
                                                              void* in_shader_module_cache_raw_ptr)
{
    const Anvil::ObjectTrackerOnObjectAboutToBeUnregisteredCallbackArg* callback_arg_ptr        = static_cast<Anvil::ObjectTrackerOnObjectAboutToBeUnregisteredCallbackArg*>(in_callback_arg);
    Anvil::ShaderModuleCache*                                           shader_module_cache_ptr = static_cast<Anvil::ShaderModuleCache*>                                    (in_shader_module_cache_raw_ptr);

    anvil_assert            (callback_arg_ptr        != nullptr);
    anvil_assert            (shader_module_cache_ptr != nullptr);
    ANVIL_REDUNDANT_VARIABLE(shader_module_cache_ptr);

    if (callback_arg_ptr->object_type == OBJECT_TYPE_SHADER_MODULE)
    {
        /* Technically we should never reach this place, as shader module cache does not implement
         * any sort of GC mechanism.
         *
         * Please investigate if this assertion check fires.
         */
        anvil_assert_fail();
    }
}

/** TODO */
void Anvil::ShaderModuleCache::on_object_registered(void* in_callback_arg,
                                                    void* in_shader_module_cache_raw_ptr)
{
    const Anvil::ObjectTrackerOnObjectRegisteredCallbackArg* callback_arg_ptr        = static_cast<Anvil::ObjectTrackerOnObjectRegisteredCallbackArg*>(in_callback_arg);
    Anvil::ShaderModuleCache*                                shader_module_cache_ptr = static_cast<Anvil::ShaderModuleCache*>                         (in_shader_module_cache_raw_ptr);

    anvil_assert(callback_arg_ptr        != nullptr);
    anvil_assert(shader_module_cache_ptr != nullptr);

    if (callback_arg_ptr->object_type == OBJECT_TYPE_SHADER_MODULE)
    {
        auto shader_module_ptr = static_cast<Anvil::ShaderModule*>(callback_arg_ptr->object_raw_ptr);

        shader_module_cache_ptr->cache(shader_module_ptr->shared_from_this() );
    }
}