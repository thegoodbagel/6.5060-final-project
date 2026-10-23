// src/main.cpp
#include <iostream>
#ifdef PL_HAVE_OPENMP
#include <omp.h>
#endif

int main() {
    std::cout << "partition_experiment: build OK\n";
#ifdef PL_HAVE_OPENMP
    std::cout << "OpenMP threads available: " << omp_get_max_threads() << "\n";
#else
    std::cout << "OpenMP: off\n";
#endif
}
