#include<stdio.h>
#include<omp.h>

int main(int argc, char * argv[])
{
#pragma omp parallel 
{
    int id = omp_get_thread_num();
    int num_threads = omp_get_num_threads();
    printf("Hello World from thread %d/%d\n", id, num_threads);
}
    int i;
    int N = 100;
    int A[N];
    int B[N];

#pragma omp parallel for schedule(guided, 16)
    for (i=0; i<N; i++) {
	A[i] = A[i] + 3.14 * B[i];
    }
    return 0;
}
