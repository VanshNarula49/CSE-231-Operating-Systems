# CSE-231-Operating-Systems
This repository contains the implementations of five projects developed in C. Each project explores different aspects of system-level programming, from ELF loading and execution to a priority-based process scheduler, multithreading, and more.

## Projects Overview

### 1. SimpleLoader

**Overview**  
SimpleLoader is a C program that implements a simple ELF loader. It loads and executes ELF executables by reading ELF and program headers, mapping necessary segments into memory, and adjusting memory protections as required.

**Key Functions**  
- `loader_cleanup()`: Frees allocated memory and closes the file descriptor to prevent accidental reuse.
- `load_and_run_elf()`: Loads the ELF file, allocates memory, reads headers, maps memory segments, and executes the entry point.


---

### 2. SimpleShell

**Overview**  
SimpleShell is a C program that provides a basic shell interface for executing commands, supporting piping, background processes, and logging command history.

**Key Features**  
- Command execution with support for pipelines and background processes.
- Command history logging and signal handling for interruption.

**Limitations**  
Does not support built-in commands like `cd` or advanced redirection.


---

### 3. SimpleScheduler

**Overview**  
SimpleScheduler is a priority-based process scheduler written in C, handling task scheduling, signal-based task management, and command logging.

**Key Features**  
- Priority-based task scheduling with time slicing.
- Signal handling for process management and logging.


---

### 4. SimpleSmartLoader

**Overview**  
SimpleSmartLoader is an advanced ELF loader that dynamically maps program sections into memory on-demand when a segmentation fault occurs. It manages memory allocation, tracks page faults, and reports internal fragmentation.

**Program Flow**  
- `main()`: Validates ELF file path and initiates loading.
- `load_and_run_elf()`: Loads headers, registers the signal handler, and starts execution.
- `segFaultHandler()`: Handles segmentation faults by mapping pages and loading data on-demand.
- `loader_cleanup()`: Cleans up allocated resources and handles file descriptors.


---

### 5. SimpleMultithreader

**Overview**  
Multithreader is a C++ program that demonstrates multithreading by creating and managing multiple threads to perform concurrent tasks. It utilizes synchronization primitives to manage thread interactions safely.

**Key Features**  
- Creation and management of multiple threads.
- Use of mutexes and condition variables for thread synchronization.
- Efficient handling of shared resources to prevent race conditions.


---

Each project in this repository demonstrates various system programming techniques and collaborative development. The contributions were made equally by all team members, ensuring a comprehensive understanding and implementation of each project.
