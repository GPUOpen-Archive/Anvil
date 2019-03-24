//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef MISC_MEMORY_BLOCK_CREATE_INFO_H
#define MISC_MEMORY_BLOCK_CREATE_INFO_H

#include "misc/types.h"


namespace Anvil
{
    class MemoryBlockCreateInfo
    {
    public:
        /* Public functions */

        /** Spawns a create info instance which can be used to instantiate a memory block whose storage space is maintained
         *  by another MemoryBlock instance.
         *
         *  @param in_parent_memory_block_ptr MemoryBlock instance to use as a parent. Must not be nullptr.
         *                                    Parent memory block must not be mapped.
         *  @param in_start_offset            Start offset of the storage maintained by the specified parent memory block,
         *                                    from which the new MemoryBlock instance's storage should start.
         *                                    Must not be equal to or larger than parent object's storage size.
         *                                    When added to @param in_size, the result value must not be be larger than the remaining
         *                                    storage size.
         *  @param in_size                    Region size to use for the derived memory block.
         **/
        static MemoryBlockCreateInfoUniquePtr create_derived(MemoryBlock* in_parent_memory_block_ptr,
                                                             VkDeviceSize in_start_offset,
                                                             VkDeviceSize in_size);

        /** Spawns a create info instance which can be used to instantiate a memory block whose lifetime is maintained by a separate
         *  entity. While the memory block remains reference-counted as usual, the destruction process is carried out by an external
         *  party via the specified call-back.
         *
         *  This implements a special case required for support of Vulkan Memory Allocator memory allocator backend. Applications
         *  are very unlikely to ever need to use this create() function.
         *
         *  NOTE: The following parameters take the following default values:
         *
         *  - Exportable external memory handle types: Anvil::ExternalMemoryHandleTypeFlagBits::NONE
         *  - Importable external memory handle type:  Anvil::ExternalMemoryHandleTypeFlagBits::NONE
         *  - Use a dedicated allocation?:             No.
         *
         *  These can be further adjusted by callingt corresponding set_..() functions prior to passing the CreateInfo instance
         *  to MemoryBlock::create().
         *
         *  TODO
         *
         *  @param in_allowed_memory_bits A bitfield whose indices correspond to memory type indices which can be used for the allocation.
         *                                
         */
        static MemoryBlockCreateInfoUniquePtr create_derived_with_custom_delete_proc(const Anvil::BaseDevice*             in_device_ptr,
                                                                                     VkDeviceMemory                       in_memory,
                                                                                     uint32_t                             in_allowed_memory_bits,
                                                                                     Anvil::MemoryFeatureFlags            in_memory_features,
                                                                                     uint32_t                             in_memory_type_index,
                                                                                     VkDeviceSize                         in_size,
                                                                                     VkDeviceSize                         in_start_offset,
                                                                                     OnMemoryBlockReleaseCallbackFunction in_on_release_callback_function);

        /** Spawns a create info instance which can be used to instantiate a new memory block.
         *
         *  This function can be used for both single- and multi-GPU device instances. For the latter case,
         *  the default behavior is to allocate a single instance of memory (deviceMask = 1) for memory heaps
         *  that do NOT have the VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR bit on, or allocate as many instances
         *  of memory as there are physical devices assigned to the logical device. This can be adjusted
         *  by calling corresponding set_..() functions.
         *
         *  NOTE: The following parameters take the following defautl values:
         *
         *  - Exportable external memory handle types: Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE
         *  - Importable external memory handle type:  Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE
         *  - MT safety:                               Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE.
         *  - Use a dedicated allocation?:             No.
         *
         *  These can be further adjusted by callingt corresponding set_..() functions prior to passing the CreateInfo instance
         *  to MemoryBlock::create().
         *
         *  @param in_device_ptr          Device to use.
         *  @param in_allowed_memory_bits Memory type bits which meet the allocation requirements.
         *  @param in_size                Required allocation size.
         *  @param in_memory_features     Required memory features.
         **/
        static MemoryBlockCreateInfoUniquePtr create_regular(const Anvil::BaseDevice*  in_device_ptr,
                                                             uint32_t                  in_allowed_memory_bits,
                                                             VkDeviceSize              in_size,
                                                             Anvil::MemoryFeatureFlags in_memory_features);

        /** Spawns a create info instance which can be used to instantiate a new memory block.
         *
         *  WARNING: in_memory_type_index has to meet all memory requirements, it is user's responsibility to adhere!
         *           If it is possible please use create_regular function instead of this function.
         *
         *  This function can be used for both single- and multi-GPU device instances. For the latter case,
         *  the default behavior is to allocate a single instance of memory (deviceMask = 1) for memory heaps
         *  that do NOT have the VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHR bit on, or allocate as many instances
         *  of memory as there are physical devices assigned to the logical device. This can be adjusted
         *  by calling corresponding set_..() functions.
         *
         *  NOTE: The following parameters take the following defautl values:
         *
         *  - Exportable external memory handle types: Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE
         *  - Importable external memory handle type:  Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE
         *  - MT safety:                               Anvil::MTSafety::INHERIT_FROM_PARENT_DEVICE.
         *  - Use a dedicated allocation?:             No.
         *
         *  These can be further adjusted by callingt corresponding set_..() functions prior to passing the CreateInfo instance
         *  to MemoryBlock::create().
         *
         *  @param in_device_ptr          Device to use.
         *  @param in_allowed_memory_bits Memory type bits which meet the allocation requirements.
         *  @param in_size                Required allocation size.
         *  @param in_memory_features     Required memory features.
         *  @param in_memory_type_index   Required memory type index
         **/
        static MemoryBlockCreateInfoUniquePtr create_with_memory_type(const Anvil::BaseDevice*  in_device_ptr,
                                                                      uint32_t                  in_allowed_memory_bits,
                                                                      VkDeviceSize              in_size,
                                                                      Anvil::MemoryFeatureFlags in_memory_features,
                                                                      uint32_t                  in_memory_type_index);

        const uint32_t& get_allowed_memory_bits() const
        {
            return m_allowed_memory_bits;
        }

        void get_dedicated_allocation_properties(bool*           out_opt_enabled_ptr,
                                                 Anvil::Buffer** out_opt_buffer_ptr_ptr,
                                                 Anvil::Image**  out_opt_image_ptr_ptr) const
        {
            if (out_opt_enabled_ptr != nullptr)
            {
                *out_opt_enabled_ptr = m_use_dedicated_allocation;
            }

            if (out_opt_buffer_ptr_ptr != nullptr)
            {
                *out_opt_buffer_ptr_ptr = m_dedicated_allocation_buffer_ptr;
            }

            if (out_opt_image_ptr_ptr != nullptr)
            {
                *out_opt_image_ptr_ptr = m_dedicated_allocation_image_ptr;
            }
        }

        float get_memory_priority() const
        {
            return m_memory_priority;
        }

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        const uint32_t& get_device_mask() const
        {
            return m_device_mask;
        }

        const Anvil::ExternalMemoryHandleTypeFlags& get_exportable_external_memory_handle_types() const
        {
            return m_exportable_external_memory_handle_types;
        }

        /* Returns true if set_external_handle_import_info() has been called prior to this call.
         * Otherwise returns false.
         *
         * If the func returns true, *out_result_ptr is set to the queried data.
         */
        bool get_external_handle_import_info(const ExternalMemoryHandleImportInfo** out_result_ptr_ptr) const
        {
            bool result = false;

            if (m_external_handle_import_info_specified)
            {
                *out_result_ptr_ptr = &m_external_handle_import_info;
                result              = true;
            }

            return result;
        }

        #if defined(_WIN32)
            /* Returns true if set_exportable_nt_handle_info() has been called prior to this call.
             * Otherwise returns false.
             *
             * If the func returns true, *out_result_ptr is set to the queried data.
             */
            bool get_exportable_nt_handle_info(const ExternalNTHandleInfo** out_result_ptr_ptr) const
            {
                bool result = false;

                if (m_exportable_nt_handle_info_specified)
                {
                    *out_result_ptr_ptr = &m_exportable_nt_handle_info;
                    result              = true;
                }

                return result;
            }
        #endif

        const Anvil::ExternalMemoryHandleTypeFlagBits& get_imported_external_memory_handle_type() const
        {
            return m_imported_external_memory_handle_type;
        }

        VkDeviceMemory get_memory() const
        {
            anvil_assert(m_type == Anvil::MemoryBlockType::DERIVED_WITH_CUSTOM_DELETE_PROC);

            return m_memory;
        }

        /** Returns memory features of the underlying memory region */
        Anvil::MemoryFeatureFlags get_memory_features() const;

        /** Returns the memory type index the memory block was allocated from */
        uint32_t get_memory_type_index() const;

        const Anvil::MTSafety& get_mt_safety() const
        {
            return m_mt_safety;
        }

        const Anvil::OnMemoryBlockReleaseCallbackFunction& get_on_release_callback_function() const
        {
            return m_on_release_callback_function;
        }

        /* Returns parent memory block, if one has been defined for this instance */
        Anvil::MemoryBlock* get_parent_memory_block() const
        {
            return m_parent_memory_block_ptr;
        }

        /* Returns the size of the memory block. */
        VkDeviceSize get_size() const
        {
            return m_size;
        }

        /* Returns the start offset of the memory block.
         *
         * If the memory block has a parent, the returned start offset is NOT relative to the parent memory block's
         * start offset (in other words: the returned value is an absolute offset which can be directly used against
         * the memory block instance)
         */
        VkDeviceSize get_start_offset() const
        {
            return m_start_offset;
        }

        const Anvil::MemoryBlockType& get_type() const
        {
            return m_type;
        }

        /* Requires VK_KHR_device_group */
        void set_device_mask(const uint32_t& in_device_mask)
        {
            m_device_mask = in_device_mask;
        }

        /* Requires VK_KHR_external_memory */
        void set_exportable_external_memory_handle_types(const Anvil::ExternalMemoryHandleTypeFlags& in_external_memory_handle_types)
        {
            m_exportable_external_memory_handle_types = in_external_memory_handle_types;
        }

        /* Lets the app specify imported handle details.
         *
         * Under Windows, if @param in_name is zero-sized, <name> member of the VkImportMemoryWin32HandleInfoKHR struct, as chained to the VkMemoryAllocateInfo struct chain,
         * will be set to nullptr.
         *
         * NOTE: This function MUST NOT be used for importing host pointers. Please use the other set_external_handle_import_info() function instead.
         * NOTE: For NT handles, you also need to call set_external_nt_handle_import_info().
         *
         * Requires VK_KHR_external_memory_fd    under Windows.
         * Requires VK_KHR_external_memory_win32 under Windows.
         */
        #if defined(_WIN32)
            void set_external_handle_import_info(ExternalHandleType  in_handle,
                                                 const std::wstring& in_name)
        #else
            void set_external_handle_import_info(ExternalHandleType in_handle)
        #endif
        {
            anvil_assert(!m_external_handle_import_info_specified);

            m_external_handle_import_info.handle = in_handle;

            #if defined(_WIN32)
            {
                m_external_handle_import_info.name = in_name;
            }
            #endif

            m_external_handle_import_info_specified = true;
        }

        /* Lets the app specify imported handle details.
         *
         * NOTE: This function MUST NOT be used for importing handles other than host pointers. Please use the other set_external_handle_import_info() function instead.
         *
         * Requires VK_EXT_external_memory_host.
         */
        void set_external_handle_import_info(void* in_host_pointer)
        {
            anvil_assert(!m_external_handle_import_info_specified);
            anvil_assert( in_host_pointer                         != nullptr);

            m_external_handle_import_info.host_ptr  = in_host_pointer;
            m_external_handle_import_info_specified = true;
        }

        #if defined(_WIN32)
            /* Lets the app specify additional details for exportable NT handles.
             *
             * If @param in_name is zero-sized, <name> member of the VkExportMemoryWin32HandleInfoKHR struct, as chained to the VkMemoryAllocateInfo struct chain,
             * will be set to nullptr.
             *
             * Requires VK_KHR_external_memory_win32
             */
            void set_exportable_nt_handle_info(const SECURITY_ATTRIBUTES* in_opt_attributes_ptr,
                                               const DWORD&               in_access,
                                               const std::wstring&        in_name)
            {
                anvil_assert(!m_exportable_nt_handle_info_specified);

                m_exportable_nt_handle_info.access         = in_access;
                m_exportable_nt_handle_info.attributes_ptr = in_opt_attributes_ptr;
                m_exportable_nt_handle_info.name           = in_name;

                m_exportable_nt_handle_info_specified      = true;
            }
        #endif

        void set_imported_external_memory_handle_type(const Anvil::ExternalMemoryHandleTypeFlagBits& in_memory_handle_type)
        {
            m_imported_external_memory_handle_type = in_memory_handle_type;
        }

        void set_mt_safety(const Anvil::MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
        }

        /* Call to request a dedicated allocation for the memory block. Requirements are:
         *
         * 1) Device must support VK_KHR_dedicated_allocation.
         * 2) Either in_opt_buffer_ptr or in_opt_image_ptr must not be null. Cases where both
         *    are nullptr or != nullptr are not allowed.
         *
         * May only be called once.
         * The specified object must remain alive until the actual memory allocation takes place.
         *
         */
        void use_dedicated_allocation(Anvil::Buffer* in_opt_buffer_ptr,
                                      Anvil::Image*  in_opt_image_ptr)
        {
            anvil_assert(m_dedicated_allocation_buffer_ptr == nullptr);
            anvil_assert(m_dedicated_allocation_image_ptr  == nullptr);
            anvil_assert(!m_use_dedicated_allocation);

            anvil_assert((in_opt_buffer_ptr == nullptr && in_opt_image_ptr != nullptr) ||
                         (in_opt_buffer_ptr != nullptr && in_opt_image_ptr == nullptr) );

            m_dedicated_allocation_buffer_ptr = in_opt_buffer_ptr;
            m_dedicated_allocation_image_ptr  = in_opt_image_ptr;
            m_use_dedicated_allocation        = true;
        }

        void set_memory_priority(const float& in_priority)
        {
            m_memory_priority = in_priority;
        }

    private:
        MemoryBlockCreateInfo(const Anvil::MemoryBlockType&                      in_type,
                              const uint32_t&                                    in_allowed_memory_bits,
                              const Anvil::BaseDevice*                           in_device_ptr,
                              VkDeviceMemory                                     in_memory,
                              const Anvil::MemoryFeatureFlags&                   in_memory_features,
                              const uint32_t&                                    in_memory_type_index,
                              const uint32_t&                                    in_n_physical_devices,
                              const Anvil::OnMemoryBlockReleaseCallbackFunction& in_on_release_callback_function,
                              const Anvil::PhysicalDevice* const*                in_opt_physical_device_ptr_ptr,
                              Anvil::MemoryBlock*                                in_parent_memory_block_ptr,
                              const VkDeviceSize&                                in_size,
                              const VkDeviceSize&                                in_start_offset);

        /* NOTE: Only to be used by Anvil::MemoryBlock! */
        void set_memory_type_index(const uint32_t& in_new_index)
        {
            m_memory_type_index = in_new_index;
        }

        uint32_t                                    m_allowed_memory_bits;
        uint32_t                                    m_device_mask;
        const Anvil::BaseDevice*                    m_device_ptr;
        Anvil::ExternalMemoryHandleTypeFlags        m_exportable_external_memory_handle_types;
        Anvil::ExternalMemoryHandleTypeFlagBits     m_imported_external_memory_handle_type;
        VkDeviceMemory                              m_memory;
        Anvil::MemoryFeatureFlags                   m_memory_features;
        float                                       m_memory_priority;
        uint32_t                                    m_memory_type_index;
        Anvil::MTSafety                             m_mt_safety;
        Anvil::OnMemoryBlockReleaseCallbackFunction m_on_release_callback_function;
        Anvil::MemoryBlock*                         m_parent_memory_block_ptr;
        std::vector<const Anvil::PhysicalDevice*>   m_physical_devices;
        VkDeviceSize                                m_size;
        VkDeviceSize                                m_start_offset;
        const Anvil::MemoryBlockType                m_type;

        Anvil::Buffer* m_dedicated_allocation_buffer_ptr;
        Anvil::Image*  m_dedicated_allocation_image_ptr;
        bool           m_use_dedicated_allocation;

        #ifdef _WIN32
            ExternalNTHandleInfo m_exportable_nt_handle_info;
            bool                 m_exportable_nt_handle_info_specified;
        #endif

        ExternalMemoryHandleImportInfo m_external_handle_import_info;
        bool                           m_external_handle_import_info_specified;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(MemoryBlockCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(MemoryBlockCreateInfo);

        friend class MemoryBlock;
    };
} /* namespace Anvil */

#endif /* MISC_MEMORY_BLOCK_CREATE_INFO_H */