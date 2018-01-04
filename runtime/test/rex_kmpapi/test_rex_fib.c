#include <stdio.h>
#include <omp.h>
#include "rex_kmp.h"

int fib(int n);

typedef struct fib_task_args {
    int n;
    int *x;
} fib_task_args_t;

static void fib_task_n_1 (void * args, void *shared) {
    fib_task_args_t * fibargs = (fib_task_args_t * ) args;
    *fibargs->x = fib(fibargs->n - 1);
}

int fib(int n)
{
  int x, y;
  if (n<2)
    return n;
  else
    {
//       #pragma omp task shared(x) firstprivate(n)
//       x=fib(n-1);
      fib_task_args_t args;
      args.n = n;
      args.x = &x;
      rex_task_t * task = rex_create_task(&fib_task_n_1, sizeof(fib_task_args_t), &args, NULL, 0);

      rex_sched_task(task);

      y=fib(n-2);

     //  #pragma omp taskwait
      rex_taskwait();
       return x+y;
    }
}


void parallel_func(int * global_tid, int*tid, int n, int * result, void *notused ) {
    printf("Thread %d of %d\n", omp_get_thread_num(), omp_get_num_threads());
    if (rex_single(*global_tid)){
        int i;
        *result = fib(n);
        rex_end_single(*global_tid);
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
  rex_parallel(4, (rex_pfunc_t)parallel_func, (void*) n, &result, NULL);
  printf ("fib(%d) = %d\n", n, fib(n));
  return 0;
}
