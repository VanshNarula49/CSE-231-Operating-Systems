#include "loader.h"

Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd = -1;

void loader_cleanup()
{
    if (phdr)
    {
        free(phdr);
        phdr = NULL;
    }

    if (ehdr)
    {
        free(ehdr);
        ehdr = NULL;
    }

    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
}

void load_and_run_elf(char** exe)
{
    // Open the ELF file using the path provided in exe[0]
    fd = open(exe[0], O_RDONLY);
    if (fd < 0)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the ELF header
    ehdr = malloc(sizeof(Elf32_Ehdr));
    if (!ehdr)
    {
        perror("Failed to allocate memory for ELF header");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    // Read the ELF header from the file
    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr))
    {
        perror("Failed to read ELF header");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the program headers
    phdr = malloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
    if (!phdr)
    {
        perror("Failed to allocate memory for program headers");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    // Move the file pointer to the program headers offset
    if (lseek(fd, ehdr->e_phoff, SEEK_SET) == -1)
    {
        perror("Failed to seek to program headers offset");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    // Read the program headers from the file
    if (read(fd, phdr, sizeof(Elf32_Phdr) * ehdr->e_phnum) != sizeof(Elf32_Phdr) * ehdr->e_phnum)
    {
        perror("Failed to read program headers");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    // Loop through the program headers to find PT_LOAD segments
    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            // Map memory for the PT_LOAD segment
            void *virtual_mem = mmap((void *)phdr[i].p_vaddr, phdr[i].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (virtual_mem == MAP_FAILED)
            {
                perror("Error mapping memory");
                loader_cleanup();
                exit(EXIT_FAILURE);
            }

            // Move the file pointer to the segment offset
            if (lseek(fd, phdr[i].p_offset, SEEK_SET) == -1)
            {
                perror("lseek failed");
                loader_cleanup();
                exit(EXIT_FAILURE);
            }

            // Read the segment data into the mapped memory
            if (read(fd, virtual_mem, phdr[i].p_filesz) != phdr[i].p_filesz)
            {
                perror("Failed to read data into memory");
                munmap(virtual_mem, phdr[i].p_memsz);
                loader_cleanup();
                exit(EXIT_FAILURE);
            }

            // Adjust memory protection to read and execute only
            if (mprotect(virtual_mem, phdr[i].p_memsz, PROT_READ | PROT_EXEC) == -1)
            {
                perror("Error adjusting memory protection");
                munmap(virtual_mem, phdr[i].p_memsz);
                loader_cleanup();
                exit(EXIT_FAILURE);
            }
        }
    }

    // Cast the entry point address to a function pointer and call it
    int (*start)(void) = (int (*)(void))(ehdr->e_entry);
    int result = start();
    printf("User start return value = %d\n", result);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(&argv[1]);
    loader_cleanup();
    return 0;
}

