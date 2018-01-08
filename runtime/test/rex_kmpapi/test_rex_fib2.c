#include <stdio.h>
#include <omp.h>
#include "rex_kmp.h"

int fib(int n);

static void fib_task_n_1 (int n, int *x) {
    *x = fib(n - 1);
}

int fib(int n)
{
  int x, y;
  int notused;
  if (n<2)
    return n;
  else
    {
//       #pragma omp task shared(x) firstprivate(n)
//       x=fib(n-1);
      REX_TASK(0, (rex_function_t)&fib_task_n_1, 2, n, &x);

      y=fib(n-2);

     //  #pragma omp taskwait
      rex_taskwait();
       return x+y;
    }
}


void parallel_func(int n, int * result, void *notused ) {
    int global_tid = rex_get_global_thread_num();
    printf("Thread %d of %d\n", omp_get_thread_num(), omp_get_num_threads());
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
  rex_parallel(4, (rex_function_t)parallel_func, 3, (void*) n, &result, NULL);
  printf ("fib(%d) = %d\n", n, fib(n));
  return 0;
}
