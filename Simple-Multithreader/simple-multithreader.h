#include <iostream>
#include <functional>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <chrono>
#include <pthread.h>

// Function declarations
void* vectorThreadFunction(void* args);
void* matrixThreadFunction(void* args);
int user_main(int argc, char** argv);
void runLambda(std::function<void()> &&lambda);

// Type aliases and structs
using FunctionPtr = void(*)(int);

struct VectorThreadArgs {
    int start_index;
    int end_index;
    std::function<void(int)> func;
};

struct MatrixThreadArgs {
    int row_start;
    int row_end;
    int col_count;
    std::function<void(int, int)> func;
};

// Function implementations

void runLambda(std::function<void()> &&lambda) {
    lambda();
}

void parallel_for(int start, int end, std::function<void(int)> &&func, int num_threads) {
    time_t start_time, end_time;
    time(&start_time);

    // Allocate thread and argument arrays
    pthread_t* threads = new pthread_t[num_threads];
    VectorThreadArgs* args_array = new VectorThreadArgs[num_threads];
    int chunk_size = 1 + ((end - 1) / num_threads);

    // Launch threads
    for (int i = 0; i < num_threads; ++i) {
        int chunk_start = i * chunk_size;
        int chunk_end = (i + 1) * chunk_size;
        if (chunk_end > end) {
            chunk_end = end;
        }
        args_array[i].start_index = chunk_start;
        args_array[i].end_index = chunk_end;
        args_array[i].func = func;
        int ret = pthread_create(&threads[i], nullptr, vectorThreadFunction, &args_array[i]);
        if (ret != 0) {
            std::cerr << "Error creating thread " << i << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Join threads
    for (int i = 0; i < num_threads; ++i) {
        int ret = pthread_join(threads[i], nullptr);
        if (ret != 0) {
            std::cerr << "Error joining thread " << i << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    time(&end_time);
    double elapsed_time = difftime(end_time, start_time) * 1000;
    printf("Parallel for-loop (1D) executed in: %.2f milliseconds\n", elapsed_time);

    // Clean up
    delete[] threads;
    delete[] args_array;
}

void* vectorThreadFunction(void* args) {
    VectorThreadArgs* arg = static_cast<VectorThreadArgs*>(args);
    for (int i = arg->start_index; i < arg->end_index; ++i) {
        arg->func(i);
    }
    return nullptr;
}

void parallel_for(int row_start, int row_end, int col_start, int col_end, std::function<void(int, int)> &&func, int num_threads) {
    time_t start_time, end_time;
    time(&start_time);

    // Allocate thread and argument arrays
    pthread_t* threads = new pthread_t[num_threads];
    MatrixThreadArgs* args_array = new MatrixThreadArgs[num_threads];
    int chunk_size = 1 + ((row_end - 1) / num_threads);

    // Launch threads
    for (int i = 0; i < num_threads; ++i) {
        int chunk_row_start = i * chunk_size;
        int chunk_row_end = (i + 1) * chunk_size;
        if (chunk_row_end > row_end) {
            chunk_row_end = row_end;
        }
        args_array[i].row_start = chunk_row_start;
        args_array[i].row_end = chunk_row_end;
        args_array[i].func = func;
        args_array[i].col_count = col_end;
        int ret = pthread_create(&threads[i], nullptr, matrixThreadFunction, &args_array[i]);
        if (ret != 0) {
            std::cerr << "Error creating thread " << i << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Join threads
    for (int i = 0; i < num_threads; ++i) {
        int ret = pthread_join(threads[i], nullptr);
        if (ret != 0) {
            std::cerr << "Error joining thread " << i << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    time(&end_time);
    double elapsed_time = difftime(end_time, start_time) * 1000;
    printf("Parallel for-loop (2D) executed in: %.2f milliseconds\n", elapsed_time);

    // Clean up
    delete[] threads;
    delete[] args_array;
}

void* matrixThreadFunction(void* args) {
    MatrixThreadArgs* arg = static_cast<MatrixThreadArgs*>(args);
    for (int i = arg->row_start; i < arg->row_end; ++i) {
        for (int j = 0; j < arg->col_count; ++j) {
            arg->func(i, j);
        }
    }
    return nullptr;
}

int main(int argc, char** argv) {
    int assignment_number = 1, version = 5;

    auto welcome_lambda = [version, &assignment_number]() {
        assignment_number = 5;
        std::cout << "*** Starting Processing for Assignment " << assignment_number << " ***\n";
    };
    runLambda(welcome_lambda);

    int return_code = user_main(argc, argv);

    auto completion_lambda = []() {
        std::cout << "*** Processing Completed Successfully ***\n";
    };
    runLambda(completion_lambda);

    return return_code;
}

#define main user_main
