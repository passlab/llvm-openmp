#include <stdio.h>
#include <omp.h>
#include "rex_kmp.h"

void for_body(int i, void * arg1, void * arg2, void * arg3) {
    int *A = (int*) arg1;
    int *B = (int*) arg2;
    A[i] = A[i] + 3.14 * B[i];
}

void parallel_func(int *A, int *B, int N) {
   rex_for(0, N-1, 1, 30, &for_body, (void*)A, (void*)B, (void*)N);
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
    rex_parallel(4, (rex_function_t)parallel_func, 3, A, B, (void*)N);
    rex_parallel_for(4, 0, N-1, 1, 30, &for_body, A, B, (void*)N);
    return 0;
}
