# SimpleScheduler - A Priority-Based Process Scheduler

**Overview**  
SimpleScheduler is a C program that schedules and executes commands based on priority and time-slicing. It allows user commands to run sequentially or in parallel, handling background processes, maintaining command logs, and supporting signal handling for robust task management.

---

### Key Features

- **Priority Scheduling**: Tasks are scheduled by priority and executed in time slices, resuming based on availability and priority.
- **Signal Handling**: Responds to `SIGUSR1` to start scheduling and manages `SIGCONT` and `SIGSTOP` for task switching.
- **Shared Memory & Semaphores**: Coordinates process information via shared memory with semaphores for synchronized access.
- **Command Logging**: Tracks each command’s start time, PID, duration, and completion in a history log.
- **User Interaction**: Supports "history" to view past commands and "exit" to terminate the scheduler.

---

### Program Structure

1. **Global Variables**: Store process data, command history, and runtime information.
2. **Data Structures**: `process_node` and `process_queue` for task information and priority management.
3. **Process Queue (PCB)**: Manages task addition, scheduling, and priority-based execution.
4. **Scheduler Execution**: Runs high-priority tasks in time slices with signal-based control.
5. **Logging & History**: Logs all executed commands and displays command history on request.

### Limitations

- **Unsupported Commands**: Commands like `cd` are unsupported, as are advanced redirection and piping operations.

### Contributions

We collaborated to design and develop the scheduler, with each member contributing equally to coding, debugging, and testing.
