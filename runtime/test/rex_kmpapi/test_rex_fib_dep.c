#include <stdio.h>
#include <omp.h>
#include "rex_kmp.h"

int fib(int n);

static void fib_task_n_1 (int n, int *x) {
    *x = fib(n - 1);
}

static void fib_task_n_2 (int n, int *y) {
    *y = fib(n - 2);
}

static void fib_task_sum (int x, int y, int *result) {
    *result = x + y;
}

int fib(int n)
{
  int x, y, result;
  if (n<2)
    return n;
  else
    {
//       #pragma omp task shared(x) firstprivate(n)
//       x=fib(n-1);
//      printf("X: %X, Y: %X\n", &x, &y);
      rex_task_t * task = rex_create_task(0, (rex_function_t)&fib_task_n_1, 2, n, &x);
      rex_task_add_dependency(task, &x, sizeof(int), REX_TASK_DEPTYPE_OUT);
      rex_sched_task(task);
      
      task = rex_create_task(0, (rex_function_t)&fib_task_n_2, 2, n, &y);
      rex_task_add_dependency(task, &y, sizeof(int), REX_TASK_DEPTYPE_OUT);
      rex_sched_task(task);

      task = rex_create_task(0, (rex_function_t)&fib_task_sum, 3, x, y, &result);
      rex_task_add_dependency(task, &x, sizeof(int), REX_TASK_DEPTYPE_IN);
      rex_task_add_dependency(task, &y, sizeof(int), REX_TASK_DEPTYPE_IN);
      rex_sched_task(task);
     //  #pragma omp taskwait
      rex_taskwait();
       return result;
    }
}


void parallel_func(int n, int * result, void*notused ) {
    int global_tid = rex_get_global_thread_num();
    if (rex_single(global_tid)){
        int i;
        *result = fib(n);
        rex_end_single(global_tid);
    }
}

int main(int argc, char * argv[])
{
  int n, result;
  if (argc != 2) {
     fprintf(stderr, "Usage: fib <n>\n");
     exit(1);
  }

  unsigned long long tm_elaps; 

  n = atoi(argv[1]);
  rex_parallel(4, (rex_function_t)parallel_func, 3, (void*)n, &result, NULL);
  printf ("fib(%d) = %d\n", n, fib(n));
  return 0;
}
