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
         *  - Exportable external memory handle types: Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE
         *  - Importable external memory handle type:  Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE
         *
         *  These can be further adjusted by callingt corresponding set_..() functions prior to passing the CreateInfo instance
         *  to MemoryBlock::create().
         *
         *  TODO
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
         *  NOTE: The following parameters take the following defautl values:
         *
         *  - Exportable external memory handle types: Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE
         *  - Importable external memory handle type:  Anvil::EXTERNAL_MEMORY_HANDLE_TYPE_NONE
         *  - MT safety:                               MT_SAFETY_INHERIT_FROM_PARENT_DEVICE.
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

        const uint32_t& get_allowed_memory_bits() const
        {
            return m_allowed_memory_bits;
        }

        const Anvil::BaseDevice* get_device() const
        {
            return m_device_ptr;
        }

        const Anvil::ExternalMemoryHandleTypeBits& get_exportable_external_memory_handle_types() const
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
            /* Returns true if set_external_nt_handle_import_info() has been called prior to this call.
             * Otherwise returns false.
             *
             * If the func returns true, *out_result_ptr is set to the queried data.
             */
            bool get_external_nt_handle_import_info(const ExternalNTHandleInfo** out_result_ptr_ptr) const
            {
                bool result = false;

                if (m_external_nt_handle_import_info_specified)
                {
                    *out_result_ptr_ptr = &m_external_nt_handle_import_info;
                    result              = true;
                }

                return result;
            }
        #endif

        const Anvil::ExternalMemoryHandleTypeBit& get_imported_external_memory_handle_type() const
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

        /* Requires VK_KHR_external_memory */
        void set_exportable_external_memory_handle_types(const Anvil::ExternalMemoryHandleTypeBits& in_external_memory_handle_types)
        {
            m_exportable_external_memory_handle_types = in_external_memory_handle_types;
        }

        /* Lets the app specify imported handle details.
         *
         * Under Windows, if @param in_name is zero-sized, <name> member of the VkImportMemoryWin32HandleInfoKHR struct, as chained to the VkMemoryAllocateInfo struct chain,
         * will be set to nullptr.
         *
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

        #if defined(_WIN32)
            /* Lets the app specify additional details for exportable NT handles.
             *
             * If @param in_name is zero-sized, <name> member of the VkExportMemoryWin32HandleInfoKHR struct, as chained to the VkMemoryAllocateInfo struct chain,
             * will be set to nullptr.
             *
             * Requires VK_KHR_external_memory_win32
             */
            void set_external_nt_handle_import_info(const SECURITY_ATTRIBUTES* in_opt_attributes_ptr,
                                                    const DWORD&               in_access,
                                                    const std::wstring&        in_name)
            {
                anvil_assert(!m_external_nt_handle_import_info_specified);

                m_external_nt_handle_import_info.access         = in_access;
                m_external_nt_handle_import_info.attributes_ptr = in_opt_attributes_ptr;
                m_external_nt_handle_import_info.name           = in_name;

                m_external_nt_handle_import_info_specified      = true;
            }
        #endif

        void set_imported_external_memory_handle_type(const Anvil::ExternalMemoryHandleTypeBit& in_memory_handle_type)
        {
            m_imported_external_memory_handle_type = in_memory_handle_type;
        }

        void set_mt_safety(const Anvil::MTSafety& in_mt_safety)
        {
            m_mt_safety = in_mt_safety;
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

        uint32_t                                    m_allowed_memory_bits;
        const Anvil::BaseDevice*                    m_device_ptr;
        Anvil::ExternalMemoryHandleTypeBits         m_exportable_external_memory_handle_types;
        Anvil::ExternalMemoryHandleTypeBit          m_imported_external_memory_handle_type;
        VkDeviceMemory                              m_memory;
        Anvil::MemoryFeatureFlags                   m_memory_features;
        uint32_t                                    m_memory_type_index;
        Anvil::MTSafety                             m_mt_safety;
        Anvil::OnMemoryBlockReleaseCallbackFunction m_on_release_callback_function;
        Anvil::MemoryBlock*                         m_parent_memory_block_ptr;
        std::vector<const Anvil::PhysicalDevice*>   m_physical_devices;
        VkDeviceSize                                m_size;
        VkDeviceSize                                m_start_offset;
        const Anvil::MemoryBlockType                m_type;

        #ifdef _WIN32
            ExternalNTHandleInfo m_external_nt_handle_import_info;
            bool                 m_external_nt_handle_import_info_specified;
        #endif

        ExternalMemoryHandleImportInfo m_external_handle_import_info;
        bool                           m_external_handle_import_info_specified;

        ANVIL_DISABLE_ASSIGNMENT_OPERATOR(MemoryBlockCreateInfo);
        ANVIL_DISABLE_COPY_CONSTRUCTOR(MemoryBlockCreateInfo);
    };
} /* namespace Anvil */

#endif /* MISC_MEMORY_BLOCK_CREATE_INFO_H */