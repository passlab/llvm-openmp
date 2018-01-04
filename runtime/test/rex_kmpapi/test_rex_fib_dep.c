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

static void fib_task_n_2 (void * args, void *shared) {
    fib_task_args_t * fibargs = (fib_task_args_t * ) args;
    *fibargs->x = fib(fibargs->n - 2);
}

typedef struct fib_task_sum_args {
    int *x;
    int *y;
    int *result;
} fib_task_sum_args_t;

static void fib_task_sum (void * args, void *shared) {
    fib_task_sum_args_t * fibargs = (fib_task_sum_args_t * ) args;
    *fibargs->result = *fibargs->x + *fibargs->y;
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
      fib_task_args_t args;
      args.n = n;
      args.x = &x;
      rex_task_t * task = rex_create_task(&fib_task_n_1, sizeof(fib_task_args_t), &args, NULL, 1);
      rex_task_add_dependency(task, &x, sizeof(int), REX_TASK_DEPTYPE_OUT);
      rex_sched_task(task);
      
      fib_task_args_t args2;
      args2.n = n;
      args2.x = &y;
      task = rex_create_task(&fib_task_n_2, sizeof(fib_task_args_t), &args2, NULL, 1);
      rex_task_add_dependency(task, &y, sizeof(int), REX_TASK_DEPTYPE_OUT);
      rex_sched_task(task);

      fib_task_sum_args_t sumargs;
      sumargs.x = &x;
      sumargs.y = &y;
      sumargs.result = &result;
      task = rex_create_task(&fib_task_sum, sizeof(fib_task_sum_args_t), &sumargs, NULL, 2);
      rex_task_add_dependency(task, &x, sizeof(int), REX_TASK_DEPTYPE_IN);
      rex_task_add_dependency(task, &y, sizeof(int), REX_TASK_DEPTYPE_IN);
      rex_sched_task(task);
     //  #pragma omp taskwait
      rex_taskwait();
       return result;
    }
}


void parallel_func(int * global_tid, int *tid, int n, int * result, void*notused ) {
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
  rex_parallel(4, (rex_pfunc_t)parallel_func, (void*)n, &result, NULL);
  printf ("fib(%d) = %d\n", n, fib(n));
  return 0;
}
