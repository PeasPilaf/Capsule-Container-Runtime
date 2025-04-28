# Linux Control Groups (cgroups)

Control Groups (cgroups) are a Linux kernel feature that allows you to allocate resources — such as CPU time, system memory, network bandwidth, or combinations of these resources — among user-defined groups of tasks (processes) running on a system.

## Key Concepts

*   **Resource Limiting:** Cgroups can be used to limit the amount of resources a group of processes can consume.
*   **Prioritization:** Assign different priorities to different groups for resource allocation.
*   **Accounting:** Measure the resource usage of different groups.
*   **Control:** Freeze groups of processes, checkpoint them, and restart them.

Cgroups are organized hierarchically, allowing for fine-grained control over resource distribution. Different resource types (CPU, memory, I/O, etc.) are managed by specific *controllers* or *subsystems*.

Common controllers include:
*   `cpu`: Limits CPU usage.
*   `memory`: Limits memory usage.
*   `blkio`: Limits block I/O.
*   `pids`: Limits the number of processes.

Cgroups are a fundamental building block for containerization technologies like Docker and Kubernetes, enabling resource isolation and limits for containers.

## Example: Limiting Memory Usage to 10MB

Here's how you can manually create a cgroup (using cgroup v1 syntax) to limit a process's memory usage to 10MB:

1.  **Identify the memory cgroup mount point:**
    ```bash
    mount | grep cgroup | grep memory
    # Example output: cgroup on /sys/fs/cgroup/memory type cgroup (rw,nosuid,nodev,noexec,relatime,memory)
    ```
    Let's assume it's `/sys/fs/cgroup/memory`.

2.  **Create a new cgroup:**
    ```bash
    sudo mkdir /sys/fs/cgroup/memory/my-limited-group
    ```

3.  **Set the memory limit:** 10MB = 10 * 1024 * 1024 = 10485760 bytes.
    ```bash
    sudo sh -c 'echo 10485760 > /sys/fs/cgroup/memory/my-limited-group/memory.limit_in_bytes'
    ```
    *Note: Also consider setting `memory.memsw.limit_in_bytes` if you want to limit memory+swap.*

4.  **Run a process within this cgroup:**
    You can add an existing process PID to the `tasks` file:
    ```bash
    # Get PID of the process you want to limit
    # sudo sh -c 'echo <PID> > /sys/fs/cgroup/memory/my-limited-group/tasks'
    ```
    Or, execute a new command directly into the cgroup (requires `cgexec` from `libcgroup-tools` or similar):
    ```bash
    # Example using cgexec (install libcgroup-tools if needed)
    # sudo cgexec -g memory:my-limited-group your_command
    ```
    Alternatively, manually add the current shell's PID to the tasks file and then run the command from that shell:
    ```bash
    # Add current shell to the cgroup
    sudo sh -c 'echo $$ > /sys/fs/cgroup/memory/my-limited-group/tasks'
    # Now run your command in this shell
    # ./my_memory_intensive_app
    ```

This process will now be constrained by the 10MB memory limit set by the cgroup. If it tries to exceed this limit, it might be slowed down or killed by the kernel's Out-Of-Memory (OOM) killer.