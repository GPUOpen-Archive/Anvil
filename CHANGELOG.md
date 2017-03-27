# Anvil Changelog

### v1.3.1
- Fixed an issue where builds with disabled ANVIL_LINK_WITH_GLSLANG option would not build correctly.
- Fixed various compilation issues (community contribution)

### v1.3
- Added four new example applications:
  * DynamicBuffers
  * MultiViewport
  * OcclusionQuery
  * PushConstants
- Added new functions useful for performing FP16<->FP32 value conversion.
- Added support for FP16 values.
- Added support for partially-resident images.
- Added support for retrieving disassembly of the generated SPIR-V blob.
- Added support for sparse bindings.
- Added support for VK_KHR_get_physical_device_properties2.
- Anvil::Device interface was refactored to provide space for adding mGPU support in the future.
- Fixed an issue where linear image mipmap data would not be correctly updated in certain scenarios.
- Fixed gcc-specific compilation warnings.
- Improved memory allocator to support allocating from more than one memory heap during life-time.
- Improved memory allocator to support memory requirements on a per-allocation basis.
- Increased warning level to 4; warnings are now treated as errors.
- Linear images now cache & expose subresource layout info for all supported aspects.
- Various bug-fixes and improvements.

### v1.2
- Added support for VK_AMD_draw_indirect_count
- All wrappers now use smart pointers for simplified memory management model.
- Unified instantiation across all wrappers. All of them now expose one or more static create() method(s). All constructors have been made private.
- Updated glslang to the latest version.
- Various bug-fixes and improvements of minor importance.

### v1.1
- Added a Window factory to let applications determine which system interface should be used to create windows.
- Adjusted validation support to follow the new validation layer naming.
- Anvil builds which are statically linked with glslang will now pass physical device-specific limits to the library.
- Buffer object wrapper will now automatically request for TRANSFER_DST usage at creation time, if it is requested to allocate memory from a non-mappable memory heap.
- Buffer object wrapper will now maintain a staging buffer for read & write operations, if the underlying memory storage is non-mappable.
- Buffer object wrapper will now report the same size as specified at creation time, even if the underlying memory is rounded up as per running platform's requirements.
- DescriptorSet now correctly recognizes the VK_WHOLE_SIZE enum.
- Fixed a bug where ranged binding updates would not work correctly.
- Fixed a bug where Swapchain wrapper would pass a signalled fence at presentable image acquisition time.
- Fixed a bug where subsequent DescriptorSet::set_binding_item() calls would be ignored.
- Fixed a copy-paste bug in Anvil::Device::get_transfer_queue().
- Fixed an invalid assertion check which assumed VkSurfaceCapabilitiesKHR::maxImageCount can never be set to 0.
- Linux version of the library no longer depends on XCB presence.
- Memory mapped into process space can now be optionally retrieved by MemoryBlock users.
- Swapchain images are no longer transitioned to a different image layout after having been created.
- Other minor fixes.
