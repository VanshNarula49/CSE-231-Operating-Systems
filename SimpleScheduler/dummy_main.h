#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

int dummy_main(int argc, char **argv);

int main(int argc, char **argv) {
    volatile sig_atomic_t running = 0;

    void handle_start(int sig){
        running=1;
    }

    void handle_stop(int sig){
        running=0;
    }

    signal(SIGSTOP, handle_stop);

    signal(SIGCONT, handle_start);	

 
    printf("Process %d is ready and waiting for scheduling.\n", getpid());

    while (!running) {
        pause();
    }

    int ret = dummy_main(argc, argv);

    printf("Process %d has completed execution.\n", getpid());
    return ret;
}

#define main dummy_main

