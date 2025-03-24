# NowRaspOS

An operating system targeted at Rasberry Pi 3b+

I am aiming to build a fully premptive multi-threaded micro-kernel architecture with basic terminal support.

## Subsystem support

This will be an additive list of the features this kernel supports that I will update as I make progress
- Early boot C environment with basic memory utilites and memory manager
- Buddy allocator page memory manager
- Heap memory manager
- Paging and MMU support

# Dependencies

A cross-compiler is required if on non arm64 machines, the guide to do this is at https://wiki.osdev.org/GCC_Cross-Compiler
with $TARGET=aarch64-elf

To run the kernel in a virtual machine you will also need `qemu-system-aarch64` which can be optained from most package distributors.

To debug you will need to build `aarch64-elf-gdb`, with information to build it in the cross-compiler guide above.

# Compiling

Running `make` should compile all src files and create a `NowRaspOS.elf` and ` NowRaspOS.img` in the `./build` directory.

# Running
Right now only supports running with `qemu-system-aarch64` as we do not have any boot loaders at the moment to load on real hardware.

To run the kernel `make run` will run the kernel in qemu.

# Debugging

Run `make debugrun` will start the kernel and wait for the gdb client to connect.

Then in a seperate process run `make debug` will start `aarch64-elf-gdb` and connect to the running instance.

You can then type `c` to tell gdb to continue to the first breakpoint which is in the beggining of kernel boot at `start.s`

# Contact

If you have any questions you can reach me at <lukenowakow@proton.me>
