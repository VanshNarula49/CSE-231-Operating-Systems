#include "loader.h"
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd = -1;
int pageFaults = 0;
int memAllocationNumber = 0;
int internalFragmentation = 0;

#define PAGE_SIZE 4096

void loader_cleanup(){
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

void segFaultHandler(int signum, siginfo_t *info, void *context) {
    if (signum == SIGSEGV) {
        if(info==NULL){
            printf("SegFaultHandler received NULL value for info");
            loader_cleanup();
            exit(EXIT_FAILURE);
        }

        if(context==NULL){
            printf("SegFaultHandler received NULL value for context");
            loader_cleanup();
            exit(EXIT_FAILURE);
        }

        pageFaults++;
        uintptr_t segFaultAddress = (uintptr_t)info->si_addr;

        if(ehdr==NULL){
            printf("NULL value received for EHDR");
            loader_cleanup();
            exit(EXIT_FAILURE);
        }
        
        if(phdr==NULL){
            printf("NULL value received for PHDR");
            loader_cleanup();
            exit(EXIT_FAILURE);
        }
        
        for (int i = 0; i < ehdr->e_phnum; i++) {
            if (phdr[i].p_type != PT_LOAD) {
                continue;
            }
            
            if (segFaultAddress >= phdr[i].p_vaddr && segFaultAddress < phdr[i].p_vaddr + phdr[i].p_memsz) {
                uintptr_t pageStart = segFaultAddress & ~(PAGE_SIZE - 1);
                int segmentOffset = pageStart - phdr[i].p_vaddr;
                int data = PAGE_SIZE;
                if (segmentOffset + data > phdr[i].p_memsz) {
                    data = phdr[i].p_memsz - segmentOffset;
                }

                void *virtual_mem = mmap((void *)pageStart, PAGE_SIZE,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,-1, 0);

                if (virtual_mem == MAP_FAILED) {
                    perror("Error mapping memory");
                    loader_cleanup();
                    exit(EXIT_FAILURE);
                }

                if (segmentOffset < phdr[i].p_filesz){
                    int dataToLoad = data;
                    if (segmentOffset + dataToLoad > phdr[i].p_filesz) {
                        dataToLoad = phdr[i].p_filesz - segmentOffset;
                    }

                    if (dataToLoad > 0) {
                        if (lseek(fd, phdr[i].p_offset + segmentOffset, SEEK_SET) == -1) {
                            perror("lseek failed");
                            loader_cleanup();
                            exit(EXIT_FAILURE);
                        }

                        if (read(fd, virtual_mem, dataToLoad) != dataToLoad) {
                            perror("Failed to read data into memory");
                            munmap(virtual_mem, PAGE_SIZE);
                            loader_cleanup();
                            exit(EXIT_FAILURE);
                        }
                    }

                    if (dataToLoad < data) {
                        memset((char *)virtual_mem + dataToLoad, 0, data - dataToLoad);
                    }
                    internalFragmentation += PAGE_SIZE - dataToLoad;
                    
                }
                else{
                    internalFragmentation += PAGE_SIZE - data;
                }

                // Set final protection flags
                int prot = PROT_READ;
                if (phdr[i].p_flags & PF_W) prot |= PROT_WRITE;
                if (phdr[i].p_flags & PF_X) prot |= PROT_EXEC;

                if (mprotect(virtual_mem, PAGE_SIZE, prot) == -1) {
                    perror("Error adjusting memory protection");
                    munmap(virtual_mem, PAGE_SIZE);
                    loader_cleanup();
                    exit(EXIT_FAILURE);
                }

                memAllocationNumber+=1;
                return;
            }
        }
        printf("Segmentation Fault could not be handled for address %p\n", info->si_addr);
        loader_cleanup();
        exit(EXIT_FAILURE);
    }
}

void load_and_run_elf(char** exe) {
    fd = open(exe[0], O_RDONLY);
    if (fd < 0) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    ehdr = malloc(sizeof(Elf32_Ehdr));
    if (!ehdr) {
        perror("Failed to allocate memory for ELF header");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        perror("Failed to read ELF header");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    phdr = malloc(sizeof(Elf32_Phdr) * ehdr->e_phnum);
    if (!phdr) {
        perror("Failed to allocate memory for program headers");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    if (lseek(fd, ehdr->e_phoff, SEEK_SET) == -1) {
        perror("Failed to seek to program headers offset");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    if (read(fd, phdr, sizeof(Elf32_Phdr) * ehdr->e_phnum) != 
        sizeof(Elf32_Phdr) * ehdr->e_phnum) {
        perror("Failed to read program headers");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    struct sigaction signalCatcher;
    signalCatcher.sa_flags = SA_SIGINFO;
    signalCatcher.sa_sigaction = segFaultHandler;

    if (sigaction(SIGSEGV, &signalCatcher, NULL) == -1) {
        perror("Error while registering signal handler");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }

    int (*start)(void) = (int (*)(void))(uintptr_t)(ehdr->e_entry);
    int result = start();
    printf("User start return value = %d\n", result);

    printf("Total number of Page Faults: %d\n", pageFaults);
    printf("Total number of Page Memory Allocations: %d\n", memAllocationNumber);
    printf("Total amount of Internal Fragmentation: %d\n", internalFragmentation);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    if (access(argv[1], F_OK) == -1) {
        printf("ELF File does not exist\n");
        exit(1);
    }

    if (access(argv[1], R_OK) == -1) {
        printf("Need permission for reading ELF\n");
        exit(1);
    }

    load_and_run_elf(&argv[1]);
    loader_cleanup();
    printf("Loader completed successfully\n");
    return 0;
}