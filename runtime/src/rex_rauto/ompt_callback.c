#include <stdio.h>
#include <inttypes.h>
#include <execinfo.h>
#include <sched.h>

#ifdef OMPT_USE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

#include <omp.h>
#include <ompt.h>
#include <rex.h>
#include "ompt-internal.h"
#include "omptool.h"

static ompt_get_unique_id_t ompt_get_unique_id;

static void
on_ompt_callback_idle_spin(void * data) {
    const void *codeptr_ra  =  &on_ompt_callback_idle_spin;
    const void *frame  =  OMPT_GET_FRAME_ADDRESS(0);
    int thread_id = rex_get_global_thread_num();
    thread_event_map_t *emap = get_event_map(thread_id);
#ifdef OMPT_TRACING_SUPPORT
    ompt_trace_record_t *record = add_trace_record(thread_id, ompt_callback_idle_spin, frame, codeptr_ra);
#endif
  //  printf("Thread: %d idle spin\n", thread_id);
}

static void
on_ompt_callback_idle_suspend(void * data) {
    const void *codeptr_ra  =  &on_ompt_callback_idle_suspend;
    const void *frame  =  OMPT_GET_FRAME_ADDRESS(0);
    int thread_id = rex_get_global_thread_num();
    thread_event_map_t *emap = get_event_map(thread_id);
#ifdef OMPT_TRACING_SUPPORT
    ompt_trace_record_t *record = add_trace_record(thread_id, ompt_callback_idle_suspend, frame, codeptr_ra);
#endif
//    printf("Thread: %d idle suspend\n", thread_id);
}


static void on_ompt_callback_idle(
        ompt_scope_endpoint_t endpoint) {
    const void *codeptr_ra  =  &on_ompt_callback_idle;
    const void *frame  =  OMPT_GET_FRAME_ADDRESS(0);
    int thread_id = rex_get_global_thread_num();
    thread_event_map_t *emap = get_event_map(thread_id);
    ompt_lexgion_t * lgp = ompt_lexgion_begin(emap, (void*)&on_ompt_callback_idle);
#ifdef OMPT_TRACING_SUPPORT
    ompt_trace_record_t *record = add_trace_record(thread_id, ompt_callback_idle, frame, codeptr_ra);
    record->event_id_additional = endpoint;
#endif

    switch (endpoint) {
        case ompt_scope_begin: {
#ifdef PE_OPTIMIZATION_SUPPORT
            int id = sched_getcpu();
            int coreid = id % TOTAL_NUM_CORES;
            int pair_id;
            if (id < TOTAL_NUM_CORES) pair_id = id + TOTAL_NUM_CORES;
            else pair_id = id - TOTAL_NUM_CORES;

            HWTHREADS_IDLE_FLAG[id] = 0;
            /* TODO: memory fence here */
            if (HWTHREADS_IDLE_FLAG[id] == 0 && HWTHREADS_IDLE_FLAG[pair_id] == 0) {
                record->frequency = pe_adjust_freq(coreid, CORE_LOW_FREQ);
                HWTHREADS_FREQ[id] = record->frequency;
            }

            //if((id != TOTAL_NUM_CORES || id != 0) && event_maps[0].time_consumed > 0.1)
            if (id != TOTAL_NUM_CORES && id != 0) {
                /*set up the state of kernel cpu id as the beginning of idle.*/
                record->frequency = pe_adjust_freq(id, CORE_LOW_FREQ);
                /*pair id of the current in the same core*/
                /*if both kernel cpu id are at the idle state, set up both as low frequency */
            }
#endif
#ifdef OMPT_TRACING_SUPPORT
            add_record_lexgion(lgp, record);
#endif
            push_lexgion(emap, lgp);
   //         printf("Thread: %d idle begin\n", thread_id);
   //         print_frame(0);
   //         printf("frame  address: %p\n", OMPT_GET_FRAME_ADDRESS(0));
   //         printf("return address: %p\n", OMPT_GET_RETURN_ADDRESS(0));
            break;
        }
        case ompt_scope_end: {
#ifdef PE_OPTIMIZATION_SUPPORT
            int id = sched_getcpu();
            int pair_id;

            if (id != TOTAL_NUM_CORES && id != 0) {
                /*set up the state of kernel cpu id as the beginning of idle.*/
                HWTHREADS_IDLE_FLAG[id] = 1;
                record->frequency = pe_adjust_freq(id, CORE_HIGH_FREQ);
            }
#endif
#ifdef OMPT_TRACING_SUPPORT
            ompt_trace_record_t *begin_record = get_last_lexgion_record(emap);
            /* link two event together */
            link_records(begin_record, record);
#endif
            pop_lexgion(emap);
            //printf("Thread: %d idle end\n", thread_id);
            break;
        }
    }
}
#define REX_RAUTO_TUNING 1

void on_rex_rauto_parallel_begin(uint32_t requested_team_size) {
    const void *codeptr_ra  =  OMPT_GET_RETURN_ADDRESS(2); /* address of the function who calls __kmpc_fork_call */
    const void *frame  =  OMPT_GET_FRAME_ADDRESS(0);
    int thread_id = rex_get_global_thread_num();
    thread_event_map_t * emap = &event_maps[thread_id];
    ompt_lexgion_t * lgp = ompt_lexgion_begin(emap, codeptr_ra);
    push_lexgion(emap, lgp);
    int team_size = requested_team_size;
    int diff;
#ifdef OMPT_MEASUREMENT_SUPPORT
    if (lgp->total_record == 1) { /* the first record */
        ompt_measure_init(&lgp->current);
        memset(&lgp->accu, 0, sizeof(ompt_measurement_t));
    } else {
#ifdef REX_RAUTO_TUNING
        if (lgp->current.requested_team_size == requested_team_size) { /* if we are executing the parallel lexgion of the same problem size */
            if (lgp->total_record == 2) { /* second time, tuning started */
                    memcpy(&lgp->best, &lgp->current, sizeof(ompt_measurement_t));
                    lgp->best_counter = 1;
                    team_size = requested_team_size - 1;
                    if (team_size <= 0) team_size = 1;
                    __kmp_push_num_threads(NULL, thread_id, team_size);
            } else {
                if (lgp->best_counter < 5) { /* we will keep auto-tuning for at least 5 times */
                    diff = ompt_measure_compare(&lgp->best, &lgp->current);
                    //printf("perf improvement of last one over the best: %d%%\n", diff);
                    if (diff >= 0){ /* the second time, or better performance */
                        memcpy(&lgp->best, &lgp->current, sizeof(ompt_measurement_t));
                        lgp->best_counter = 1;
                        team_size = lgp->current.team_size - 1;
                        if (team_size <= 0) team_size = 1;
                        __kmp_push_num_threads(NULL, thread_id, team_size);
                        printf("set team size for %X: %d->%d, improvement %d%%\n", codeptr_ra, requested_team_size, team_size, diff);
                    } else {
                        team_size = lgp->best.team_size;
                        lgp->best_counter++;
                        __kmp_push_num_threads(NULL, thread_id, team_size);
                    }
                } else {
                    team_size = lgp->best.team_size;
                    lgp->best_counter++;
                    __kmp_push_num_threads(NULL, thread_id, team_size);
                    //printf("No tune anymore for this lexgion: %X, at count: %d\n", codeptr_ra, lgp->total_record);
                }
            }
        }
#endif
    }
    lgp->current.requested_team_size = requested_team_size;
    lgp->current.team_size = team_size;
#endif

#ifdef OMPT_TRACING_SUPPORT
    ompt_trace_record_t *record = add_trace_record(thread_id, ompt_callback_parallel_begin, frame, codeptr_ra);
    add_record_lexgion(lgp, record);
    //
    record->requested_team_size = requested_team_size;
    record->team_size = team_size;
    record->codeptr_ra = codeptr_ra;
    //record->user_frame = OMPT_GET_FRAME_ADDRESS(0); /* the frame of the function who calls __kmpc_fork_call */
//    record->user_frame = parent_task_frame->reenter_runtime_frame;
#endif

#ifdef OMPT_MEASUREMENT_SUPPORT
    ompt_measure(&lgp->current);
#endif

    //printf("Thread: %d parallel begin: FRAME_ADDRESS: %p, LOCATION: %p, exit_runtime_frame: %p, reenter_runtime_frame: %p, codeptr_ra: %p\n",
    //thread_id, record->user_frame, record->codeptr_ra, parent_task_frame->exit_runtime_frame, parent_task_frame->reenter_runtime_frame, codeptr_ra);
    //print_ids(4);
}

void on_rex_rauto_parallel_end() {
    int thread_id = rex_get_global_thread_num();
    thread_event_map_t *emap = get_event_map(thread_id);
    ompt_lexgion_t * lgp = ompt_lexgion_end(emap);

#ifdef OMPT_MEASUREMENT_SUPPORT
    ompt_measure_consume(&lgp->current);
    ompt_measure_accu(&lgp->accu, &lgp->current);
    if (lgp->current.team_size != lgp->current.requested_team_size) {
        omp_set_num_threads(lgp->current.requested_team_size);
    }
#endif

    const void *codeptr_ra  =  OMPT_GET_RETURN_ADDRESS(2); /* address of the function who calls __kmpc_fork_call */
    const void *frame  =  OMPT_GET_FRAME_ADDRESS(0);

#ifdef OMPT_TRACING_SUPPORT
    ompt_trace_record_t *end_record = add_trace_record(thread_id, ompt_callback_parallel_end, frame, codeptr_ra);
    /* find the trace record for the begin_event of the parallel region */
    ompt_trace_record_t *begin_record = get_last_lexgion_record(emap);
    /* pair the begin and end event together so we create a double-link between each other */
    link_records(begin_record, end_record);

#ifdef OMPT_MEASUREMENT_SUPPORT
    begin_record->measurement = lgp->current;
#endif
#ifdef OMPT_ONLINE_TRACING_PRINT
    printf("Thread: %d, parallel: %X, instance: %d \t| ", thread_id, codeptr_ra, begin_record->record_id);
    ompt_measure_print_header(&lgp->current);
    printf("                                       \t\t| ");
    ompt_measure_print(&lgp->current);
#endif
#endif
    pop_lexgion(emap);

//    printf("Thread: %d parallel end\n", thread_id);
/*
  if(inner_counter == iteration_ompt)
  {
	int i;
	for(i = 0;i<72;i++)
		if(i != 0 || i != TOTAL_NUM_CORES)
		{
		cpufreq_set_frequency(i,CORE_LOW_FREQ);
                event_maps[i].frequency = CORE_LOW_FREQ;
		}
  }
  else inner_counter++;
*/
}

void on_rex_rauto_thread_begin(int thread_id) {
    thread_event_map_t * emap = init_thread_event_map(thread_id);
#ifdef OMPT_TRACING_SUPPORT
    emap->records = (ompt_trace_record_t *) malloc(sizeof(ompt_trace_record_t) * MAX_NUM_RECORDS);
#endif
#if 0
    const void *codeptr_ra  =  &on_rex_rauto_thread_begin; /* address of the function who calls __kmpc_fork_call */
    const void *frame  =  OMPT_GET_FRAME_ADDRESS(0);
    ompt_lexgion_t * lgp = ompt_lexgion_begin(emap, (void*)on_rex_rauto_thread_begin);
    push_lexgion(emap, lgp);
#ifdef OMPT_TRACING_SUPPORT
    emap->records = (ompt_trace_record_t *) malloc(sizeof(ompt_trace_record_t) * MAX_NUM_RECORDS);
    ompt_trace_record_t *record = add_trace_record(thread_id, ompt_callback_thread_begin, frame, codeptr_ra);
    add_record_lexgion(lgp, record);
#endif
    //printf("Thread: %d thread begin\n", thread_id);
#endif
    ompt_measure_init(&emap->thread_total);
    ompt_measure(&emap->thread_total);
}

void on_rex_rauto_thread_end(int thread_id ) {
    thread_event_map_t *emap = get_event_map(thread_id);
#if 0
    const void *codeptr_ra  =  &on_rex_rauto_thread_end; /* address of the function who calls __kmpc_fork_call */
    const void *frame  =  OMPT_GET_FRAME_ADDRESS(0);
    //printf("Thread: %d thread end\n", thread_id);
#ifdef OMPT_TRACING_SUPPORT
    ompt_trace_record_t *end_record = add_trace_record(thread_id, ompt_callback_thread_end, frame, codeptr_ra);
    ompt_trace_record_t *begin_record = get_last_lexgion_record(emap);

    /* pair the begin and end event together so we create a double-link between each other */
    link_records(begin_record, end_record);
#endif
//    fini_thread_event_map(thread_id);
    pop_lexgion(emap);
#endif
    ompt_measure_consume(&emap->thread_total);
}

int rex_rauto_init() {
#ifdef PE_OPTIMIZATION_SUPPORT
    /* check the system to find total number of hardware cores, total number of hw threads (kernel processors),
     * SMT way and mapping of hwthread id with core id
     */
    int i;
    int coreid = sched_getcpu() % TOTAL_NUM_CORES;
    int hwth2id = coreid + TOTAL_NUM_CORES; /* NOTES: this only works for 2-way hyperthreading/SMT */
    for(i = 0; i < TOTAL_NUM_CORES; i++)
    {
        if (coreid == i) {
            HWTHREADS_FREQ[i] = CORE_HIGH_FREQ;
            cpufreq_set_frequency(i, CORE_HIGH_FREQ);
        } else {
            HWTHREADS_FREQ[i] = CORE_LOW_FREQ;
            cpufreq_set_frequency(i, CORE_LOW_FREQ);
        }
    }

    for (; i<TOTAL_NUM_HWTHREADS; i++) {
        if (hwth2id == i) {
            HWTHREADS_FREQ[i] = CORE_HIGH_FREQ;
        } else {
            HWTHREADS_FREQ[i] = CORE_LOW_FREQ;
        }
    }
#endif

    ompt_measure_global_init( );
    ompt_measure_init(&total_consumed);
    ompt_measure(&total_consumed);

    return 1; //success
}

void rex_rauto_finalize() {
    // on_ompt_event_runtime_shutdown();
    ompt_measure_consume(&total_consumed);
    ompt_measure_global_fini( );
    printf("==============================================================================================\n");
    printf("Total OpenMP Execution: | ");
    ompt_measure_print_header(&total_consumed);
    printf("                        | ");
    ompt_measure_print(&total_consumed);
    printf("==============================================================================================\n");

    /*
    void* callstack[128];
    int i, frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i) {
        printf("%s\n", strs[i]);
    }
     */

//    int thread_id = rex_get_global_thread_num();
    thread_event_map_t *emap = get_event_map(0);
    list_past_lexgions(emap);
}
