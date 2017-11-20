#include <stdio.h>
#include <omp.h>

int fib(int n)
{
  int x, y;
  if (n<2)
    return n;
  else
    {
       #pragma omp task shared(x) firstprivate(n)
       x=fib(n-1);

       y=fib(n-2);

       #pragma omp taskwait
       return x+y;
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
  #pragma omp parallel shared(n, result, tm_elaps)
  {
    int id = omp_get_thread_num();
    #pragma omp single
    {
	int i;
        result = fib(n);
    }
  }
  printf ("fib(%d) = %d\n", n, fib(n));
  return 0;
}
