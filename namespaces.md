# Unix/Linux Namespaces

Namespaces are a fundamental aspect of virtualization and containerization on Linux systems.

They provide process isolation by partitioning kernel resources.

Each process belongs to one namespace of each type.

This ensures that a process can only see resources within its own namespaces.

## Purpose

The primary goal of namespaces is to limit what a process can see and interact with.

This creates isolated environments, similar to virtual machines, but with much lower overhead.

They are a key technology behind containers like Docker and LXC.

## Types of Namespaces

Linux provides several types of namespaces:

1.  **PID (Process ID):** Isolates the process ID number space. Processes in different PID namespaces can have the same PID. The first process in a new PID namespace gets PID 1 and acts like `init`.

2.  **Net (Network):** Isolates network interfaces, IP addresses, routing tables, port numbers, etc. Each network namespace has its own private network stack.

3.  **Mnt (Mount):** Isolates filesystem mount points. Processes can have a different view of the filesystem hierarchy.

4.  **UTS (Unix Timesharing System):** Isolates hostname and NIS domain name. Allows containers to have their own hostname.

5.  **IPC (Inter-Process Communication):** Isolates System V IPC objects and POSIX message queues. Prevents processes in different namespaces from interfering via shared memory, semaphores, etc.

6.  **User:** Isolates user IDs (UID) and group IDs (GID). Allows a process to have root privileges within its namespace without having them on the host system.

7.  **Cgroup:** Isolates the Cgroup hierarchy, providing a virtualized view of control groups.

Namespaces allow multiple containers to run on a single host securely and efficiently.