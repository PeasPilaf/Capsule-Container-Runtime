# Building a Simple Container Manager in C

   Use the `pivot_root` syscall to mount a new root directory for our container, isolating the host file system from the process
   Add a readonly sys mount and some necessary defaults for dev (devtmpfs)

## Repository Structure

- **`/`**: 
  - **`control_group.md`**: Documentation on Linux Control Groups (cgroups).
  - **`namespaces.md`**: Documentation on Linux namespaces.
  - **`README.md`**: This file.
  
- **`/src`**: 
  - **`libcapsule`**: Folder containing `libcapsule.h`, the interface & it's implementation `libcapsule.c`.
  - **`main.c`**: A simple C program for container management, using `libcapsule.h`
  - **`unshare_example.sh`**: Shell script demonstrating the use of the `unshare` command to create namespaces, and manually set up a "container-ised" process

## How to Use

1. **Run the C Programs**:
   - Compile the C files using `gcc` or a similar compiler.
   - Example: `gcc -Isrc -Isrc/libcapsule src/main.c src/libcapsule/libcapsule.c -o capsule`
   - Run the compiled binary: `./capsule cmd ...args`

2. **Explore Shell Scripts**:
   - Use `src/unshare_example.sh` to experiment with namespaces.
   - Example: `sudo  ./unshare_example.sh`

3. **Learn from Documentation**:
   - Read `namespaces.md` and `control_group.md` for in-depth explanations of namespaces and cgroups.

4. **Follow Along with the Video**:
   - Link coming soon.

## Prerequisites

- A Linux system with kernel version 4.0 or higher.
- Basic knowledge of Linux commands and C programming.
- Root privileges for running certain scripts and programs.

##
## License

This branch of the repository is licensed under the MIT License. Feel free to use and modify the content as needed.
