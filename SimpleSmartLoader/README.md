# SimpleSmartLoader

# Overview
The loader.c program loads and executes an ELF (Executable and Linkable Format) binary file. It maps the necessary sections into memory on-demand when a segmentation fault occurs due to page access, dynamically handling memory allocation, data loading, and protection changes. Additionally, it tracks and reports page faults, memory allocations, and internal fragmentation.

# Program Flow

main: The entry point of the program. It checks if the provided ELF file path is valid and accessible, then calls load_and_run_elf to start loading and running the ELF executable.

load_and_run_elf: It opens the ELF file and loads the ELF header and program headers into memory and registers the segFaultHandler function to handle SIGSEGV (segmentation fault) signals for on-demand page loading,sets the program’s entry point and begins execution. After execution is complete, prints statistics on page faults, memory allocations, and internal fragmentation.

segFaultHandler:It is triggered when a segmentation fault occurs when an unmapped page is accessed. It Calculates the fault address, identifies the relevant program segment, and maps a memory page and loads any required data from the ELF file into memory and tracks internal fragmentation for partially filled pages. Finally, it sets the appropriate memory protections (e.g., read, write, execute) for the page.

loader_cleanup:It Frees the memory allocated for ELF header and program header and closes the file descriptor while checking for errors.

# Contributions:

We collaborated to design and develop the scheduler, with each member contributing equally to coding, debugging, and testing.
