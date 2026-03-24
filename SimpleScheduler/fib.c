#include <stdio.h>
#include <time.h>

unsigned long long fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main() {
    int n = 40; // You can adjust this number for more computation
    clock_t start, end;

    printf("Calculating Fibonacci(%d)...\n", n);
    
    start = clock(); // Start timer
    unsigned long long result = fibonacci(n);
    end = clock();   // End timer

    double time_taken = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Fibonacci(%d) = %llu\n", n, result);
    printf("Time taken: %.2f seconds\n", time_taken);
    
    return 0;
}
