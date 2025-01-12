#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

int pid_list[100];
int start_time_list[100];
clock_t start_time;
clock_t end_time;
int total_time_list[100];
int no_of_commands = 0;
char cmd_list[100][100];
volatile sig_atomic_t signum = 0;
int scheduler_started = 0;
int ncpu;
int tslice;
char* submit_cmd_list[100];
pid_t submit_id_list[100];
int submit_execution_times_list[100];
clock_t submit_wait_time_list[100];
int submit_times_executed[100];
clock_t submit_start_time_list[100];
clock_t submit_wait_start_time_list[100];
int pos;

struct process_node {
    char *file;
    struct process_node *next;
    pid_t id;
    int executed;
    int priority;
    clock_t start;
    clock_t end;
    clock_t total;
};

struct process_queue {
    struct process_node *front;
    struct process_node *rear;
    int size;
};

typedef struct shm_t {
    struct process_queue *pcb;
    sem_t mutex;
} shm_t;

shm_t *shared_mem;
size_t shared_size = sizeof(shm_t);
int fd_shared_mem;

struct process_queue *create_pcb() {
    struct process_queue *pcb = (struct process_queue *)malloc(sizeof(struct process_queue));
    if (pcb != NULL) {
        pcb->front = NULL;
        pcb->rear = NULL;
        pcb->size = 0;
    }
    return pcb;
}

void add_process(struct process_queue *pcb, const char *file_name, int priority) {
    struct process_node *new_process = (struct process_node *)malloc(sizeof(struct process_node));
    new_process->file = strdup(file_name);
    new_process->next = NULL;
    new_process->id = 0;
    new_process->executed = 0;
    new_process->priority = priority;

    if (pcb->front == NULL) {
        pcb->front = new_process;
        pcb->rear = new_process;
    } else {
        pcb->rear->next = new_process;
        pcb->rear = new_process;
    }
}

void init_submit_list() {
    for (int i = 0; i < 100; i++) {
        submit_cmd_list[i] = NULL;
        submit_id_list[i] = 0;
        submit_execution_times_list[i] = 0;
        submit_wait_time_list[i] = 0;
    }
}

void print_history() {
    printf("\nComplete History\n");
    printf("No.  PID    Start Time  Total Time  Command\n");
    for (int l = 0; l < no_of_commands; l++) {
        printf("%-4d %-6d %-12d %-12d %s\n", l + 1, pid_list[l], start_time_list[l], total_time_list[l], cmd_list[l]);
    }
    exit(0);
}

int exec_process(char *cmd, char *args[], int num_pipes) {
    if (num_pipes > 0) return -3;
    
    pid_t process_id = fork();
    pid_list[no_of_commands] = process_id;
    
    if (process_id < 0) return -1;
    
    if (process_id == 0) {
        if (execvp(cmd, args) == -1) return -2;
    }
    
    int status;
    waitpid(process_id, &status, 0);
    return process_id;
}

int exec_process_scheduler(char *cmd, char *args[], int num_pipes) {
    if (num_pipes > 0) return -3;
    
    pid_t process_id = fork();
    pid_list[no_of_commands] = process_id;
    
    if (process_id < 0) return -1;
    
    if (process_id == 0) {
        if (execvp(cmd, args) == -1) return -2;
    }
    
    int status;
    waitpid(process_id, &status, WNOHANG);
    return process_id;
}

int find_process_index(char *args[]) {
    int i = 0;
    while (submit_cmd_list[i] != NULL) {
        if (strcmp(submit_cmd_list[i], args[0]) == 0) {
            return i;
        }
        i++;
    }
    return -1;
}

int add_to_submit_list(char *args[]) {
    int i = 0;
    while(submit_cmd_list[i] != NULL) i++;
    submit_cmd_list[i] = strdup(args[1]);
    submit_id_list[i] = getpid();
    submit_execution_times_list[i] = 0;
    submit_wait_time_list[i] = 0;
    return i;
}

void scheduler_handler(int scheduler_sig) {
    if (scheduler_sig == SIGUSR1) {
        struct process_queue *sch_pcb = shared_mem->pcb;
        
        while (shared_mem->pcb->front != NULL) {
            for (int i = 0; i < ncpu; i++) {
                if (sch_pcb->front == NULL) break;
                
                struct process_node *highest_priority = sch_pcb->front;
                struct process_node *current = sch_pcb->front;
                
                while (current != NULL) {
                    if (current->priority < highest_priority->priority) {
                        highest_priority = current;
                    }
                    current = current->next;
                }
                
                char *args[] = {highest_priority->file, NULL};
                int proc_idx = find_process_index(args);
                
                if (highest_priority->id == 0) {
                    pid_t process_id = fork();
                    if (process_id < 0) break;
                    
                    if (process_id == 0) {
                        submit_start_time_list[proc_idx] = clock();
                        submit_id_list[proc_idx] = getpid();
                        exec_process_scheduler(highest_priority->file, args, 0);
                        exit(0);
                    } else {
                        highest_priority->id = process_id;
                        int status;
                        waitpid(process_id, &status, WNOHANG);
                    }
                } else if (highest_priority->executed == 0) {
                    highest_priority->executed++;
                    submit_start_time_list[proc_idx] = clock();
                    submit_wait_start_time_list[proc_idx] = clock();
                    
                    kill(highest_priority->id, SIGCONT);
                    highest_priority->start = clock();
                    usleep(1000 * tslice);
                    clock_t end_time = clock();
                    clock_t elapsed_time = end_time - highest_priority->start;
                    
                    if (elapsed_time > tslice * CLOCKS_PER_SEC / 1000) {
                        kill(highest_priority->id, SIGSTOP);
                        sem_wait(&shared_mem->mutex);
                        highest_priority->executed++;
                        if (highest_priority == sch_pcb->front) {
                            sch_pcb->front = highest_priority->next;
                        } else {
                            current = sch_pcb->front;
                            while (current->next != highest_priority) {
                                current = current->next;
                            }
                            current->next = highest_priority->next;
                        }
                        sem_post(&shared_mem->mutex);
                    }
                } else {
                    clock_t wait_start_time = submit_wait_start_time_list[proc_idx];
                    clock_t end_time = clock();
                    submit_wait_time_list[proc_idx] += end_time - wait_start_time;
                    highest_priority->total = highest_priority->executed;
                    highest_priority->end = clock();
                    submit_execution_times_list[proc_idx] += (int)(highest_priority->total);
                    
                    sem_wait(&shared_mem->mutex);
                    highest_priority->executed++;
                    if (highest_priority == sch_pcb->front) {
                        sch_pcb->front = highest_priority->next;
                    } else {
                        current = sch_pcb->front;
                        while (current->next != highest_priority) {
                            current = current->next;
                        }
                        current->next = highest_priority->next;
                    }
                    sem_post(&shared_mem->mutex);
                }
            }
        }
    }
}

int main() {
    signal(SIGUSR1, scheduler_handler);
    signal(SIGINT, print_history);
    init_submit_list();
    
    fd_shared_mem = shm_open("/shared_mem", O_CREAT | O_RDWR, 0666);
    if (fd_shared_mem == -1) {
        printf("shm_open failed");
        exit(0);
    }
    
    if (ftruncate(fd_shared_mem, shared_size) == -1) {
        printf("ftruncate failed");
        exit(0);
    }
    
    shared_mem = (shm_t *)mmap(NULL, sizeof(shm_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, fd_shared_mem, 0);
    if (ftruncate(fd_shared_mem, shared_size) == -1) {
        printf("ftruncate failed");
        exit(0);
    }
    
    shared_mem->pcb = create_pcb();
    sem_init(&shared_mem->mutex, 1, 1);
    
    printf("Enter total number of CPU resources: ");
    scanf("%d", &ncpu);
    printf("Enter time quantum (in milliseconds): ");
    scanf("%d", &tslice);
    
    char input_str[100];
    char *arguments[100];
    
    while (1) {
        printf("SimpleShell~$: ");
        fgets(input_str, sizeof(input_str), stdin);
        start_time = clock();
        
        if (strcmp(input_str, "\n") == 0) continue;
        
        start_time_list[no_of_commands] = start_time;
        input_str[strlen(input_str) - 1] = '\0';
        strcpy(cmd_list[no_of_commands], input_str);
        
        if (strcmp(input_str, "exit") == 0) {
            if (shared_mem->pcb->front == NULL) {
                printf("Program Exited\n");
                end_time = clock();
                total_time_list[no_of_commands] = (int)(end_time - start_time);
                no_of_commands++;
                
                int all_finished = 1;
                while (1) {
                    all_finished = 1;
                    for (int i = 0; i < no_of_commands; i++) {
                        if (pid_list[i] > 0) {
                            int status;
                            if (waitpid(pid_list[i], &status, WNOHANG) == 0) {
                                all_finished = 0;
                                usleep(1000);
                            }
                        }
                    }
                    if (all_finished) break;
                }
                break;
            } else {
                printf("Processes yet to be completed\n");
                fflush(stdout);
            }
        } else if (strcmp(input_str, "history") == 0) {
            printf("History of commands:\n");
            for (int k = 0; k < no_of_commands; k++) {
                printf("%d  %s\n", k + 1, cmd_list[k]);
            }
            end_time = clock();
            total_time_list[no_of_commands] = (int)(end_time - start_time);
            no_of_commands++;
        } else {
            char *command = strtok(input_str, " ");
            int arg_length = 0;
            int pipes = 0;
            
            while (command != NULL) {
                arguments[arg_length] = command;
                if (strcmp(command, "|") == 0) pipes++;
                command = strtok(NULL, " \n");
                arg_length++;
            }
            arguments[arg_length] = NULL;
            
            if (strcmp(arguments[0], "submit") == 0) {
                sem_wait(&shared_mem->mutex);
                add_process(shared_mem->pcb, arguments[1], arguments[2] ? atoi(arguments[2]) : 1);
                pos = add_to_submit_list(arguments);
                sem_post(&shared_mem->mutex);
            } else if (strcmp(arguments[0], "start") == 0) {
                kill(0, SIGUSR1);
            } else {
                int state = exec_process(arguments[0], arguments, pipes);
                if (state == -1) {
                    printf("Error occurred while forking\n");
                    break;
                } else if (state == -2) {
                    printf("Error occurred while executing the command\n");
                    break;
                } else if (state == -3) {
                    printf("External Error\n");
                    break;
                }
            }
            end_time = clock();
            total_time_list[no_of_commands] = (int)(end_time - start_time);
            no_of_commands++;
        }
    }
    
    printf("\nComplete History\n");
    printf("PID   Start Time  Total Time  Command\n");
    for (int l = 0; l < no_of_commands; l++) {
        printf("%-6d %-12d %-12d %s\n", pid_list[l], start_time_list[l], total_time_list[l], cmd_list[l]);
    }

    
    munmap(shared_mem, sizeof(shm_t));
    close(fd_shared_mem);
    shm_unlink("/shared_mem");
    sem_destroy(&shared_mem->mutex);
    return 0;
}