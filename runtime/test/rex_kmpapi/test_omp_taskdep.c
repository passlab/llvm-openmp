#include <omp.h>
#include <stdio.h>
#include <unistd.h>
int main() {
	int a,b,c;
 
	#pragma omp parallel
        {
            #pragma omp master
            {
                #pragma omp task depend(out:a)
                {
                    #pragma omp critical
                    printf ("Task 1\n");
                }
    
                #pragma omp task depend(out:b)
                {
                    #pragma omp critical
                    printf ("Task 2\n");

    
                    #pragma omp task depend(out:a,b,c)
                    {
                        sleep(1);
                        #pragma omp critical
                        printf ("Task 5\n");
                    }

                }
    
                #pragma omp task depend(in:a,b) depend(out:c)
                {
                    printf ("Task 3\n");
                }
    
                #pragma omp task depend(in:c)
                {
                    printf ("Task 4\n");
                }
           }
           if (omp_get_thread_num () == 1)
                sleep(1);
      }
         return 0;
}

