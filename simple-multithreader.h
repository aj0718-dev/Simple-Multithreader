#include <iostream>
#include <list>
#include <functional>
#include <stdlib.h>
#include <cstring>

int user_main(int argc, char **argv);

/* Demonstration on how to pass lambda as parameter.
 * "&&" means r-value reference. You may read about it online.
 */
void demonstration(std::function<void()> && lambda) {
  lambda();
}

int main(int argc, char **argv) {
  /* 
   * Declaration of a sample C++ lambda function
   * that captures variable 'x' by value and 'y'
   * by reference. Global variables are by default
   * captured by reference and are not to be supplied
   * in the capture list. Only local variables must be 
   * explicity captured if they are used inside lambda.
   */
  int x=5,y=1;
  // Declaring a lambda expression that accepts void type parameter
  auto /*name*/ lambda1 = /*capture list*/[/*by value*/ x, /*by reference*/ &y](void) {
    /* Any changes to 'x' will throw compilation error as x is captured by value */
    y = 5;
    std::cout<<"====== Welcome to Assignment-"<<y<<" of the CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  // Executing the lambda function
  demonstration(lambda1); // the value of x is still 5, but the value of y is now 5

  int rc = user_main(argc, argv);
 
  auto /*name*/ lambda2 = [/*nothing captured*/]() {
    std::cout<<"====== Hope you enjoyed CSE231(A) ======\n";
    /* you can have any number of statements inside this lambda body */
  };
  demonstration(lambda2);
  return rc;
}

#define main user_main

class SimpleMultithreader {
public:
    // One-dimensional parallel_for
    static void parallel_for(int low, int high, std::function<void(int)> &&lambda, int numThreads) {
        auto start_time = std::chrono::high_resolution_clock::now();

        // Define the thread function
        struct ThreadData {
            int start, end;
            std::function<void(int)> func;
        };

        auto thread_function = [](void* arg) -> void* {
            ThreadData* data = static_cast<ThreadData*>(arg);
            for (int i = data->start; i < data->end; ++i) {
                data->func(i);
            }
            return nullptr;
        };

        // Calculate the workload for each thread
        int range = high - low;
        int chunkSize = range / numThreads;
        int remainder = range % numThreads;

        std::vector<pthread_t> threads(numThreads);
        std::vector<ThreadData> threadData(numThreads);

        for (int t = 0; t < numThreads; ++t) {
            int start = low + t * chunkSize + std::min(t, remainder);
            int end = start + chunkSize + (t < remainder ? 1 : 0);
            threadData[t] = {start, end, lambda};

            if (pthread_create(&threads[t], nullptr, thread_function, &threadData[t]) != 0) {
                std::cerr << "Error creating thread " << t << std::endl;
                exit(1);
            }
        }

        // Join all threads
        for (int t = 0; t < numThreads; ++t) {
            pthread_join(threads[t], nullptr);
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        std::cout << "Execution time for 1D parallel_for: " << duration << " ms" << std::endl;
    }
}