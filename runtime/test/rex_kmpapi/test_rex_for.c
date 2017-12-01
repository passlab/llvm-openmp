#include <stdio.h>
#include <omp.h>
#include "rex_kmp.h"

typedef struct for_body_arg_t {
	int * A;
	int * B;
} for_body_arg_t;

void for_body(int i, void * varg) {
    for_body_arg_t* arg = (for_body_arg_t*)varg;
    arg->A[i] = arg->A[i] + 3.14 * arg->B[i];
}

void parallel_func(int *global_id, int num_args, int *A, int *B, int N) {
	
   for_body_arg_t arg;
   arg.A = A;
   arg.B = B;
	
   rex_for(0, N-1, 1, 30, for_body, &arg);
	
}

int main(int argc, char * argv[])
{

    int i;
    int N = 1000;
    int A[N];
    int B[N];

/* this call and the above parallel_fun, for_body (pseudo code)
   implements the following omp parallel for:
	
#pragma omp parallel for schedule(guided, 16)
    for (i=0; i<N; i++) {
	A[i] = A[i] + 3.14 * B[i];
    }
*/
    rex_parallel(4, (rex_pfunc_t)parallel_func, 3, A, B, N);
	
    return 0;
}
