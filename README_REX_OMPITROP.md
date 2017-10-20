# REX Support for OpenMP Interoperability with Threading and Tasking Programming Model

## Oversubscription 
[Our early proposal for addressing OpenMP oversubscription](https://link.springer.com/chapter/10.1007/978-3-319-45550-1_14) 
was discussed in IWOMP 2016. `omp_set_wait_policy` was introduced to address passive oversubscription. 
   1. Active oversubscription: Claiming or requesting more threads than what are available by the system. 
   1. Passive oversubscrip- tion: Thread resources are not released after parallel execution. It is important to note that holding hardware threads after parallel execution may not always hurt the performance overall, e.g. it may improve the start-up performance of the upcoming parallel region.

