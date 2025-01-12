# Simple Multithreader

This project demonstrates a simple multithreading framework implemented in `simple-multithreader.h`. The header file provides abstractions to efficiently parallelize computational tasks using C++ threads (pthread), focusing on 1D and 2D parallel processing patterns.

## Features

### `runLambda()`
The `runLambda()` function is a simple wrapper that executes a given lambda function. It takes a movable `std::function` as an argument and directly calls it. This function enables flexible lambda execution and is used in the main program flow for initialization and completion messages.

### `parallel_for()` (1D Vector Processing)
The 1D parallel processing function divides a given range into chunks distributed across multiple threads. It calculates the chunk size by dividing the total work range by the number of threads. Each thread receives a specific start and end index along with the function to execute. The implementation uses POSIX threads (`pthread`) to create and manage threads, with each thread executing the provided function for its assigned index range.

- Timing is tracked using the `time()` function, measuring the total execution time in milliseconds.
- After thread creation, the function waits for all threads to complete using `pthread_join()`.
- Upon completion, it prints the total execution time and frees allocated thread and argument resources.

### `parallel_for()` (2D Matrix Processing)
The 2D parallel processing function follows a similar pattern to the 1D version but operates on a two-dimensional space. It divides rows among threads, with each thread processing its assigned row range across all columns. The implementation uses separate thread and argument structures to handle the additional complexity of two-dimensional iteration.

- Like the 1D version, it dynamically allocates threads, distributes work, tracks execution time, and manages thread synchronization.
- The function allows for parallel processing of matrix-like operations by providing a lambda that receives both row and column indices.

### Thread Functions

- `vectorThreadFunction()`: The core thread execution routine for vector processing. It iterates through its assigned index range, calling the provided function for each index.
- `matrixThreadFunction()`: The core thread execution routine for matrix processing. It adds an additional nested loop to iterate through columns within the assigned row range.

## Program Flow

The `matrix.cpp` and `vector.cpp` files leverage this header by:
- Including `simple-multithreader.h`
- Using `parallel_for()` with lambda functions
- Performing parallel matrix multiplication or vector addition

The `main()` function in `simple-multithreader.h` wraps the user's main function, providing setup and completion messages. This structure provides a consistent initialization and finalization process for parallel processing tasks.

## Abstraction

The library abstracts thread management, allowing developers to focus on the computational logic rather than low-level threading details.

## Contributions

We collaborated to design and develop the scheduler, with each member contributing equally to coding, debugging, and testing.
