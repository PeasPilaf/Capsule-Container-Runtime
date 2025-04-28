# Building a Simple Container Manager in C

This repository contains resources and code examples for understanding Linux containers. The content is structured to accompany a YouTube video series titled "Understanding Linux Containers."

## Repository Structure

- **`/`**: 
  - **`control_group.md`**: Documentation on Linux Control Groups (cgroups).
  - **`namespaces.md`**: Documentation on Linux namespaces.
  - **`README.md`**: This file.
  
- **`/src`**: 
  - **`headers.h`**: Header file with necessary includes for container-related C programs.
  - **`capsule.c`**: A simple C program for container management.
  - **`unshare_example.sh`**: Shell script demonstrating the use of the `unshare` command to create namespaces, and manually set up a "container-ised" process

## How to Use

1. **Run the C Programs**:
   - Compile the C files using `gcc` or a similar compiler.
   - Example: `gcc -o capsule capsule.c -lpthread`
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