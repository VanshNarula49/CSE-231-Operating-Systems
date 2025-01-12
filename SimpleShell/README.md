We have written a C program that implements a simple shell that allows the execution of commands, supports piping between multiple commands, handles background processes, and logs command history and execution details. The shell reads input from the user, processes it, and executes commands either sequentially or in parallel using pipes. It also handles the "history" and "exit" commands and also maintains logs of executed commands and their runtimes.

We have first defined data-structures to handle commands and pipelines. Then we have written several functions to perform operations involved in a simple shell. 

The function parse_file() opens the specified file while checking if the file cant be opened in which case, it throws an error message. Inside the loop, the function reads a line of the file into a buffer until a newline is encountered or it reaches the end of the file and this line is printed.  It's used to display the command history and process log files.

The function interrput_routine() handles the SIGINT signal (triggered by Ctrl+C). When the signal is received, it prints a message, logs process information by calling parse_file(), and sets a flag (exitFlag) to exit the program. 

The function execute_pipeline()  manages the execution of a series of piped commands. It creates pipes, forks processes to run each command, and handles input/output redirection. It also logs the PID, start time, and execution duration of each command in the procLog.txt file. 

In the main() function, the program reads input from the user, parses it, and handles special commands like history which is used to display previous commands and exit which is used to terminate the shell. The history for the functions is tracked by the shell logging all commands into the cmdLog.txt file. For regular commands, it constructs a Pipeline structure containing the parsed commands and then calls execute_pipeline() to run them.. 

Some of the commands that the shell cannot execute are:

cd: The code uses execvp() to execute commands, which is intended for external programs found in the system's path. cd is a built-in shell command and it won't work because it would require modifying the shell's internal state, but execvp() creates a separate child process.  

Complex Combinations of Redirection and Piping: Pipes and redirection combined together will also not work on this shell as redirection has not been handled in the code. For example, a command like ls > file.txt will not work as it will not be able to redirect its output to the given file. 

Contributions:

We collaborated on the project by first discussing the problem in detail and developing a solution together. After deciding on an approach, we divided the tasks based on our strengths and maintained regular communication throughout. We assisted each other with coding, debugging, and refining the final solution. Our joint efforts ensured that we both contributed equally to all parts of the assignment.
