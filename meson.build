project('Anvil', 'c', 'cpp', default_options: ['c_std=c11', 'cpp_std=c++11'])

anvil_config = configuration_data()
anvil_config.set('ANVIL_INCLUDE_XCB_WINDOW_SYSTEM_SUPPORT', true)
configure_file(output: 'config.h', configuration: anvil_config)
anvil_config_include_dir = include_directories('.')

anvil_include_dirs = [
    anvil_config_include_dir,
    include_directories('include'),
    include_directories('deps')
]

anvil_dependencies = [
    dependency('vulkan'),
    dependency('threads')
]

anvil_sources = [
    'src/misc/memalloc_backends/backend_oneshot.cpp',
    'src/misc/memalloc_backends/backend_vma.cpp',
    'src/misc/base_pipeline_create_info.cpp',
    'src/misc/base_pipeline_manager.cpp',
    'src/misc/buffer_create_info.cpp',
    'src/misc/buffer_view_create_info.cpp',
    'src/misc/compute_pipeline_create_info.cpp',
    'src/misc/debug.cpp',
    'src/misc/debug_marker.cpp',
    'src/misc/descriptor_set_create_info.cpp',
    'src/misc/dummy_window.cpp',
    'src/misc/external_handle.cpp',
    'src/misc/event_create_info.cpp',
    'src/misc/fence_create_info.cpp',
    'src/misc/formats.cpp',
    'src/misc/fp16.cpp',
    'src/misc/framebuffer_create_info.cpp',
    'src/misc/graphics_pipeline_create_info.cpp',
    'src/misc/image_create_info.cpp',
    'src/misc/image_view_create_info.cpp',
    'src/misc/io.cpp',
    'src/misc/memory_allocator.cpp',
    'src/misc/memory_block_create_info.cpp',
    'src/misc/object_tracker.cpp',
    'src/misc/page_tracker.cpp',
    'src/misc/pools.cpp',
    'src/misc/render_pass_create_info.cpp',
    'src/misc/sampler_create_info.cpp',
    'src/misc/semaphore_create_info.cpp',
    'src/misc/shader_module_cache.cpp',
    'src/misc/swapchain_create_info.cpp',
    'src/misc/time.cpp',
    'src/misc/types.cpp',
    'src/misc/types_classes.cpp',
    'src/misc/types_struct.cpp',
    'src/misc/types_utils.cpp',
    'src/misc/window.cpp',
    'src/misc/window_factory.cpp',
    'src/wrappers/buffer.cpp',
    'src/wrappers/buffer_view.cpp',
    'src/wrappers/command_buffer.cpp',
    'src/wrappers/command_pool.cpp',
    'src/wrappers/compute_pipeline_manager.cpp',
    'src/wrappers/descriptor_pool.cpp',
    'src/wrappers/descriptor_set.cpp',
    'src/wrappers/descriptor_set_group.cpp',
    'src/wrappers/descriptor_set_layout.cpp',
    'src/wrappers/descriptor_set_layout_manager.cpp',
    'src/wrappers/descriptor_update_template.cpp',
    'src/wrappers/device.cpp',
    'src/wrappers/event.cpp',
    'src/wrappers/fence.cpp',
    'src/wrappers/framebuffer.cpp',
    'src/wrappers/graphics_pipeline_manager.cpp',
    'src/wrappers/image.cpp',
    'src/wrappers/image_view.cpp',
    'src/wrappers/instance.cpp',
    'src/wrappers/memory_block.cpp',
    'src/wrappers/physical_device.cpp',
    'src/wrappers/pipeline_cache.cpp',
    'src/wrappers/pipeline_layout.cpp',
    'src/wrappers/pipeline_layout_manager.cpp',
    'src/wrappers/query_pool.cpp',
    'src/wrappers/queue.cpp',
    'src/wrappers/render_pass.cpp',
    'src/wrappers/rendering_surface.cpp',
    'src/wrappers/sampler.cpp',
    'src/wrappers/semaphore.cpp',
    'src/wrappers/shader_module.cpp',
    'src/wrappers/swapchain.cpp',
    'src/misc/xcb_loader.cpp',
    'src/misc/window_xcb.cpp'
]

anvil = static_library('Anvil', 
    anvil_sources, 
    include_directories: anvil_include_dirs, 
    dependencies: anvil_dependencies,
    cpp_args: '-fpermissive',
    link_args: '-Wall, -Werror',
    install: true)

install_subdir('include/misc', install_dir: 'include/Anvil')
install_subdir('include/wrappers', install_dir: 'include/Anvil')
install_subdir('deps/VulkanMemoryAllocator', install_dir: 'include/Anvil')