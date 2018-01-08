#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "omp.h"        /* extern "C" declarations of user-visible routines */
#include "rex_kmp.h"
double read_timer_ms() {
    struct timeval t;
    gettimeofday(&t, 0);
    return t.tv_sec * 1000ULL + t.tv_usec / 1000ULL;
}

double read_timer() {
    return read_timer_ms() * 1.0e-3;
}

void parallel_func(char *hello, char *world, int arg3)
{
    int global_tid = rex_get_global_thread_num();
    int tid = omp_get_thread_num();
    printf("%d (gid: %d) of %d: msg: %s %s %d\n", tid, global_tid, omp_get_num_threads(), hello, world, arg3);
    /* for master construct */
    if (rex_master(global_tid)) {
        printf("Master greeting: (gid: %d)\n", global_tid);
        rex_end_master(global_tid);
    }

    /* for master with barrier */
    if (rex_barrier_master(global_tid)) {
        printf("Master greeting and hold: (gid: %d)\n", global_tid);
        rex_end_barrier_master(global_tid);
    }

    /* for single construct */
    if (rex_single(global_tid)) {
        printf("Solo greeting: (gid: %d:%d)\n", global_tid, tid);
        rex_end_single(global_tid);
    }

    rex_barrier(global_tid);
    
}

int main(int argc, char *argv[])
{
    int nums = 8;
    if (argc == 2) nums = atoi(argv[1]);
    int i;
    int num_runs = 1;
    char *hello = "Hello";
    char *world = "World";
    int int20 = 20;
    double times = read_timer();
    printf("%s %s %d\n", hello, world, int20);
    int current_thread = rex_get_global_thread_num();
    printf("========================================================\n");
    rex_parallel(nums, (rex_function_t)&parallel_func, 3, hello, world, int20);

    printf("========================================================\n");
    for (i=0; i<num_runs; i++) {
        rex_parallel(nums/2, (rex_function_t)&parallel_func, 3, hello, world, int20);
    }
    nums = rex_get_total_num_threads();
    printf("Result (%d threads): %f, current_thread: %d\n", nums, times/num_runs, current_thread);
    times = read_timer() - times;
    
    times = read_timer();
    printf("========================================================\n");
    rex_parallel(nums*2, (rex_function_t)&parallel_func, 3, hello, world, int20);
    times = read_timer() - times;

    nums = rex_get_total_num_threads();
    printf("Result (%d threads): %f, current_thread: %d\n", nums, times/num_runs, current_thread);
    return 0;
}

