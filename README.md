# Anvil

This is a README file for Anvil, a framework for Vulkan&trade;.
The README is organized as a FAQ.

|Toolchain|Status|
|:--------|-----:|
|**Linux (clang/gcc) build status**|[![Build Status](https://travis-ci.org/GPUOpen-LibrariesAndSDKs/Anvil.svg?branch=master)](https://travis-ci.org/GPUOpen-LibrariesAndSDKs/Anvil)|
|**Windows (VS15/VS17) build status:**|[![Build status](https://ci.appveyor.com/api/projects/status/e395xs8mvq92fa5t?svg=true)](https://ci.appveyor.com/project/DominikWitczakAMD/anvil)|


What license governs Anvil usage?
------
MIT. See `LICENSE.txt`.

What is this?
------
Anvil is a framework for Vulkan v1.0 and v1.1, which we have been using internally for
quite some time now, in order to develop various Vulkan applications. This guarantees
that vast majority of the functionality exposed by the library is regularly tested.

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
- Provide a simple cross-platform implementation for areas unrelated to Vulkan
  (eg. window management)
- Provide a simple way (with optional flexibility) to manage memory allocations
  and memory bindings.

Anvil is **not** the right choice for developers who do not have a reasonable
understanding of how Vulkan works. Its goal is not to provide a glBegin/glEnd-like
level of abstraction, but rather to give a sensible environment,
in which you can rapidly prototype Vulkan applications.

What platforms and hardware does it work on?
------
Currently, Anvil has been confirmed to build and work correctly under:
- 32- and 64-bit Linux   (Ubuntu)
- 32- and 64-bit Windows (7, 8.1, 10)

What are Anvil's requirements?
------
In order to build Anvil, you will need the following software:
- C++11 compiler.
- CMake
- Vulkan SDK (the latest available version is highly recommended)

To build Anvil on Linux, additional packages should be installed:
- libxcb-keysyms (For ubuntu, use "apt-get install libxcb-keysyms1-dev")

Does Anvil work only with AMD driver?
------
Anvil has *not* been designed with AMD hardware architecture in mind, but it
is going to provide interface-level support for any extensions we may decide
to release in the future.

As such, it should (and has been verified to) work on any Windows Vulkan
implementation and on AMDGPU-PRO Vulkan implementation on Linux.

What can it do?
------
Anvil provides full support for functionality exposed in Vulkan 1.0 and Vulkan 1.1.
We also try to do our best to keep it up to date with any extensions our Vulkan implementations
expose.

We are planning to keep adding new features in the future.

Are there any Anvil examples available which would present how to use the framework?
------
Anvil comes with several example applications, including OutOfOrderRasterization,
which renders 10k teapots on screen. It uses various Anvil wrappers, so you can
use it to get a better understanding of how various parts of the library can be
used.

...and its Vulkan-related code only takes ~45kbytes!

OutOfOrderRasterization and the other example applications are located in the
`examples` directory.

What are the known issues?
------
Please observe the "Issues" tab for more details.

Who made it?
------
Various developers within AMD.

What if I have some feed-back?
------
Please feel free to open an issue in Anvil's GitHub project.

Attribution
-----------

* AMD, the AMD Arrow logo, Radeon, and combinations thereof are either registered trademarks or trademarks of Advanced Micro Devices, Inc. in the United States and/or other countries.
* Microsoft, Visual Studio, and Windows are either registered trademarks or trademarks of Microsoft Corporation in the United States and/or other countries.
* Linux is the registered trademark of Linus Torvalds in the U.S. and other countries.
* Vulkan and the Vulkan logo are trademarks of the Khronos Group, Inc.
