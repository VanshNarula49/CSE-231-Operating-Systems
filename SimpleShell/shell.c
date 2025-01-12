#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

#define BUFFER_SIZE 2048
#define MAX_ARGS 200
#define MAX_PIPES 20

typedef struct {
    char* args[MAX_ARGS];
    int arg_count;
    bool bgProc;
} Command;

typedef struct {
    Command commands[MAX_PIPES];
    int command_count;
    bool bgProc;
} Pipeline;

static volatile int exitFlag = 0;

void parse_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), file)) {
        printf("%s", buffer);
    }
    fclose(file);
}

void interrupt_routine(int signo) {
    if (signo == SIGINT) {
        printf("\nInterrupted\n");
        parse_file("procLog.txt");
        exitFlag = 1;
    }
}

void execute_pipeline(Pipeline* piped_commands) {
    int pipe_fds[MAX_PIPES - 1][2];
    int cmd_idx = 0;

    while (cmd_idx < piped_commands->command_count - 1) {
        if (pipe(pipe_fds[cmd_idx]) == -1) {
            perror("Pipe creation failed");
            return;
        }
        cmd_idx++;
    }

    cmd_idx = 0;

    while (cmd_idx < piped_commands->command_count) {
        int input_source = (cmd_idx == 0) ? STDIN_FILENO : pipe_fds[cmd_idx - 1][0];
        int output_dest = (cmd_idx == piped_commands->command_count - 1) ? STDOUT_FILENO : pipe_fds[cmd_idx][1];

        struct timeval start_time;
        gettimeofday(&start_time, NULL);
        pid_t process_id = fork();

        if (process_id < 0) {
            perror("Failed to fork process");
            return;
        }

        if (process_id == 0) {
            if (input_source != STDIN_FILENO) {
                if (dup2(input_source, STDIN_FILENO) == -1) {
                    perror("Input redirection failed");
                    exit(EXIT_FAILURE);
                }
                close(input_source);
            }

            if (output_dest != STDOUT_FILENO) {
                if (dup2(output_dest, STDOUT_FILENO) == -1) {
                    perror("Output redirection failed");
                    exit(EXIT_FAILURE);
                }
                close(output_dest);
            }

            if (execvp(piped_commands->commands[cmd_idx].args[0], piped_commands->commands[cmd_idx].args) == -1) {
                perror("Command execution failed");
                exit(EXIT_FAILURE);
            }
        }

        struct timeval end_time;
        gettimeofday(&end_time, NULL);

        double duration = (end_time.tv_sec - start_time.tv_sec) + 
                          (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

        char full_cmd[BUFFER_SIZE] = {0};
        for (int arg_idx = 0; arg_idx < piped_commands->commands[cmd_idx].arg_count; arg_idx++) {
            strcat(full_cmd, piped_commands->commands[cmd_idx].args[arg_idx]);
            strcat(full_cmd, " ");
        }

        FILE* log_file = fopen("procLog.txt", "a");
        if (!log_file) {
            perror("Error opening log file");
            return;
        }
        fprintf(log_file, "PID: %d  | Start: %.2f | Duration: %.6f\n",
                (int)process_id,
                (double)start_time.tv_sec + (double)start_time.tv_usec / 1000000.0,
                duration);
        fclose(log_file);

        if (cmd_idx > 0) {
            close(pipe_fds[cmd_idx - 1][0]);
        }
        if (cmd_idx < piped_commands->command_count - 1) {
            close(pipe_fds[cmd_idx][1]);
        }

        cmd_idx++;
    }

    if (!piped_commands->bgProc) {
        for (cmd_idx = 0; cmd_idx < piped_commands->command_count; cmd_idx++) {
            wait(NULL);
        }
    }
}

int main() {
    signal(SIGINT, interrupt_routine);
    signal(SIGCHLD, SIG_IGN);

    char input[BUFFER_SIZE];
    int cmd_count = 0;

    while (!exitFlag) {
        printf("SimpleShell: ");
        if (!fgets(input, sizeof(input), stdin)) {
            perror("Error reading input");
            break;
        }

        input[strcspn(input, "\n")] = 0;
        FILE* history = fopen("cmdLog.txt", "a");
        if (history) {
            fprintf(history, "%d: %s\n", ++cmd_count, input);
            fclose(history);
        } else {
            perror("Error opening history file");
        }

        if (strcmp(input, "exit") == 0) {
            printf("Exiting...\n");
            parse_file("procLog.txt");
            break;
        } else if (strcmp(input, "history") == 0) {
            parse_file("cmdLog.txt");
        } else {
            Pipeline pipeline = {0};
            int command_index = 0;
            char* pipe_split = strtok(input, "|");

            while (pipe_split != NULL && command_index < MAX_PIPES) {
                Command new_command = {0};
                int arg_index = 0;

                char* arg_split = strtok(pipe_split, " \t\n");
                while (arg_split != NULL && arg_index < MAX_ARGS - 1) {
                    new_command.args[arg_index++] = arg_split;
                    arg_split = strtok(NULL, " \t\n");
                }

                if (strcmp(new_command.args[arg_index - 1], "&") == 0) {
                    new_command.args[arg_index - 1] = NULL;
                    new_command.bgProc = true;
                    pipeline.bgProc = true;
                }

                new_command.args[arg_index] = NULL;
                pipeline.commands[command_index++] = new_command;
                pipe_split = strtok(NULL, "|");
            }

            pipeline.command_count = command_index;

            execute_pipeline(&pipeline);
        }
    }

    return 0;
}
