// RUN: %libomp-compile-and-run | FileCheck %s
// REQUIRES: ompt
#include "callback.h"
#include <omp.h>

int main()
{
  omp_lock_t lock;
  omp_init_lock(&lock);
  omp_set_lock(&lock);
  omp_unset_lock(&lock);
  omp_destroy_lock(&lock);


  omp_nest_lock_t nest_lock;
  omp_init_nest_lock(&nest_lock);
  omp_set_nest_lock(&nest_lock);
  omp_set_nest_lock(&nest_lock);
  omp_unset_nest_lock(&nest_lock);
  omp_unset_nest_lock(&nest_lock);
  omp_destroy_nest_lock(&nest_lock);


  #pragma omp critical
  {
    print_ids(0);
  }


  int x = 3;
  #pragma omp atomic
  x++;


  #pragma omp ordered
  {
    print_ids(0);
  }


  // Check if libomp supports the callbacks for this test.
  // CHECK-NOT: {{^}}0: Could not register callback 'ompt_callback_mutex_acquire'
  // CHECK-NOT: {{^}}0: Could not register callback 'ompt_callback_mutex_acquired'
  // CHECK-NOT: {{^}}0: Could not register callback 'ompt_callback_mutex_released'
  // CHECK-NOT: {{^}}0: Could not register callback 'ompt_callback_nest_lock'

  // CHECK: 0: NULL_POINTER=[[NULL:.*$]]

  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_init_lock: wait_id=[[WAIT_ID:[0-9]+]], hint={{.*}}, impl={{.*}}, return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_wait_lock: wait_id=[[WAIT_ID:[0-9]+]], hint={{.*}}, impl={{.*}}, return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_acquired_lock: wait_id=[[WAIT_ID:[0-9]+]], return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_release_lock: wait_id=[[WAIT_ID:[0-9]+]], return_address={{.*}}

  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_init_nest_lock: wait_id=[[WAIT_ID:[0-9]+]], hint={{.*}}, impl={{.*}}, return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_wait_nest_lock: wait_id=[[WAIT_ID:[0-9]+]], hint={{.*}}, impl={{.*}}, return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_acquired_nest_lock_first: wait_id=[[WAIT_ID:[0-9]+]], return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_wait_nest_lock: wait_id=[[WAIT_ID:[0-9]+]], hint={{.*}}, impl={{.*}}, return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_acquired_nest_lock_next: wait_id=[[WAIT_ID:[0-9]+]], return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_release_nest_lock_prev: wait_id=[[WAIT_ID:[0-9]+]], return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_release_nest_lock_last: wait_id=[[WAIT_ID:[0-9]+]], return_address={{.*}}

  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_wait_critical: wait_id=[[WAIT_ID:[0-9]+]], hint={{.*}}, impl={{.*}}, return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_acquired_critical: wait_id=[[WAIT_ID:[0-9]+]], return_address={{.*}}

  // atomic cannot be tested because it is implemented with atomic hardware instructions 
  // disabled_CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_wait_atomic: wait_id=[[WAIT_ID:[0-9]+]], hint={{.*}}, impl={{.*}}, return_address={{.*}}
  // disabled_CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_acquired_atomic: wait_id=[[WAIT_ID:[0-9]+]], return_address={{.*}}

  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_wait_ordered: wait_id=[[WAIT_ID:[0-9]+]], hint={{.*}}, impl={{.*}}, return_address={{.*}}
  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_acquired_ordered: wait_id=[[WAIT_ID:[0-9]+]], return_address={{.*}}

  return 0;
}
