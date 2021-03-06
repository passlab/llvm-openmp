# REX Support for OpenMP Interoperability with Threading and Tasking Programming Model

## Oversubscription 
[Our early proposal for addressing OpenMP oversubscription](https://link.springer.com/chapter/10.1007/978-3-319-45550-1_14) 
was discussed in IWOMP 2016. `omp_set_wait_policy` was introduced to address passive oversubscription. 
   1. Active oversubscription: Claiming or requesting more threads than what are available by the system. 
   1. Passive oversubscrip- tion: Thread resources are not released after parallel execution. It is important to note that holding hardware threads after parallel execution may not always hurt the performance overall, e.g. it may improve the start-up performance of the upcoming parallel region.

| Wait Policy          | Description                                    | Pseudo Code                          |
| -------------------- |:----------------------------------------------:| ------------------------------------:|
| ACTIVE: SPIN_BUSY    | Busy wait in user level                        | `while (!finished());`               |
| ACTIVE: SPIN_PAUSE   | Busy wait while pausing CPU                    | `while (!finished()) cpu pause();`   |
| PASSIVE: SPIN_YIELD  | Busy wait with yield                           | `while (!finished()) sched yield();` |
| PASSIVE: SUSPEND     | Thread sleeps and need others to wake it up.   | `mutex wait(); mutex wake();`        |
| TERMINATE            | Thread terminates                              | `pthread_exit();`                    |

The threading binding set of `omp_set_wait_policy(policy)` depends on where it being called, see below:

| call site            | Binding thread set                             |
| -------------------- |:----------------------------------------------:| 
| sequential region    | The whole contention group                     |
| parallel region      | The calling thread                             | 

The previous implementation onluy support the above two types binding thread set when setting wait policy for threads. It is often that thread-specific setting and setting according to the location provide better and finer grained control of the thread behavior. For example, we want to set wait policy for threads participating in barrier to be ACTIVE:SPIN_BUSY waiting while for threads after a join barrier to be PASSIVE:SPSPENDed. 
IMPLICIT BARRIER: e.g. those in workshare, single
JOIN BARRIER:
IDLE

