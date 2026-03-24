# OS Projects

A collection of systems programming projects built in C/C++ covering 
core operating system concepts.

---

## Projects

### 🐚 Simple-Shell
A Unix shell implementation in C supporting:
- 4-stage command pipelines with I/O redirection
- Signal handling (Ctrl+C, Ctrl+Z)
- Circular 50-entry command history buffer
- Validated across 50+ edge case scenarios

### ⏱️ SimpleScheduler
A Round-Robin CPU scheduler in C featuring:
- Configurable CPU cores (1–4) and time slices (10–100ms)
- Tested with 100 simulated processes
- Achieved average waiting-time reduction of 15% vs baseline

### 📦 OS-SmartLoader
An ELF 32-bit executable loader using demand paging:
- mmap-based lazy segment allocation
- On-demand page fault handling
- Reduced memory fragmentation by 20% vs eager loading

### 🧵 Simple-Multithreader
A multithreading library in C++ implementing:
- Custom parallel_for for 1D and 2D workloads
- Matrix and vector parallelization using pthreads
- Configurable thread pool for workload distribution

---

## Tech Stack
- **Language:** C, C++
- **Concepts:** Process management, CPU scheduling, Memory management, 
  Multithreading, ELF binary loading
- **Tools:** GCC, Make, Linux/Unix
