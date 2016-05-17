# Anvil

This is a README file for Anvil v1.0, a framework for Vulkan&trade;.
The README is organized as a FAQ.

What license governs Anvil usage?
------
MIT. See See `LICENSE.txt`.

What is this?
------
Anvil is a framework for Vulkan v1.0, which we have been using internally for
quite some time now, in order to develop various Vulkan applications.

The general idea we started from was to have a cross-platform tool, which would
reduce the amount of time required to write portable Vulkan-based apps from
scratch. We would then find ourselves adding new features & extending the
existing codebase with new wrappers every now and then.
  
This eventually led to the library we have decided to release to the public.

Why? Do we really need another wrapper library for Vulkan?
------
Anvil was designed with the following goals in mind:

- Provide object-oriented Vulkan solution.
- Reduce the amount of code the developer needs to write in order to start
  using Vulkan, without hiding the API behind thick abstraction layers.
- Simplify validation layer usage. All you have to do is specify which function
  you would like to be called if a debug call-back is made, and that's it.
- Provide a reasonable level of deferred approach to baking objects, in order
  to emulate mutability of certain Vulkan objects to a certain degree. Note
  that this is an optional feature - you are free to request baking of any
  object at any time, in ordre to ensure it will never happen at draw time.
- Provide a simple cross-platform implementation for areas unrelated to Vulkan
  (eg. window management)

Anvil can be thought of as a toolbox. You are free to use any of its parts you
like,  and write your own code for everything else. Any Anvil wrapper for an
actual Vulkan object can be queried to retrieve the raw Vulkan handle.

Anvil is **not** the right choice for developers who do not have a reasonable
understanding of how Vulkan works. Its goal is not to provide a glBegin/glEnd-like
level of abstraction, but rather to give a sensible environment,
in which you can rapidly prototype Vulkan applications.
  
What platforms and hardware does it work on?
------
Currently, Anvil has been confirmed to build and work correctly under:
- 32- and 64-bit Linux   (Ubuntu)
- 32- and 64-bit Windows (7, 8, 10)

What are Anvil's requirements?
------
In order to build Anvil, you will need the following software:
- C++11 compiler.
- CMake
- Vulkan SDK

To build the Anvil on Linux, additional packages should be installed:
- libxcb-keysyms (For ubuntu, use "apt-get install libxcb-keysyms1-dev")

Does Anvil work only with AMD driver?
------
Anvil has *not* been designed with AMD hardware architecture in mind, but it
is going to provide interface-level support for any extensions we may decide
to release in the future.

As such, it should (and has been verified to) work on any Linux/Windows Vulkan
implementation.

What can it do?
------
This version of Anvil provides:

*  Low-level ref-counted wrappers for the following Vulkan v1.0 features:
   - Buffer objects
   - Buffer views
   - Command buffers
   - Command pools
   - Descriptor pools
   - Descriptor sets
   - Descriptor set layouts
   - Devices
   - Events
   - Fences
   - Framebuffers
   - Image objects
   - Image views
   - Instances
   - Physical devices
   - Pipeline cache
   - Pipeline layouts
   - Query pools
   - Queues
   - Renderpasses
   - Rendering surfaces
   - Samplers
   - Semaphores
   - Shader modules
   - Swapchains

* More complicated wrappers:
   - Compute / graphics pipeline managers: provide a way to create regular/derivative pipelines with automatic pipeline layout re-use.
   - Descriptor set groups:                simplify descriptor set configuration, management and updates.
   - Memory allocator:                     implements a very simple memory allocator which assigns properly-aligned memory blocks to Vulkan buffer & image objects.
   - Pipeline layout manager:              caches all created pipeline layouts to avoid instantiating duplicate layout instances.

* Miscellaneous functionality:
   - Format info reflection:               provides information about format properties.
   - GLSL -> SPIR-V conversion:            provides glslang-based GLSL->SPIR-V conversion. The conversion is performed in run-time.
   - IO:                                   provides a number of functions to simplify directory- and file-related operations.
   - Object tracker:                       tracks object allocations and helps detect object leakage.
   - Time:                                 provides a way to query current time using high-performance system queries.

We are planning to keep adding new features in the future.

Are there any Anvil examples available which would present how to use the framework?
------
Anvil currently comes with a single example application, which renders 10k
teapots on screen. It uses various Anvil wrappers, so you can use it to get
a better understanding of how various parts of the library can be used.

...and its Vulkan-related code only takes ~45kbytes!

What are the known issues?
------
Just a handful:
* All command queues are currently assigned a priority of 1.0.
* Buffer object wrapper does not support staging buffer-based writing.
* DescriptorSetGroup wrapper does not leverage the <releaseable_sets> flag at
  baking time.
* Per-attachment blending may not work correctly.
* Reference-counting mechanism is non-atomic, and needs to be replaced with
  automated pointers.

Some of these will definitely be addressed in the next Anvil release.

Who made it?
------
Various developers within AMD.

What if I have some feed-back?
------
For now, please post it under the GPUOpen article informing about the library release.

Attribution
-----------

* AMD, the AMD Arrow logo, Radeon, and combinations thereof are either registered trademarks or trademarks of Advanced Micro Devices, Inc. in the United States and/or other countries.
* Microsoft, Visual Studio, and Windows are either registered trademarks or trademarks of Microsoft Corporation in the United States and/or other countries.
* Linux is the registered trademark of Linus Torvalds in the U.S. and other countries.
* Vulkan and the Vulkan logo are trademarks of the Khronos Group, Inc.
