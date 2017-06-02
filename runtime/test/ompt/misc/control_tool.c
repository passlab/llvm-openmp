// RUN: %libomp-compile-and-run | FileCheck %s
// REQUIRES: ompt
#include "callback.h"
#include <omp.h>

int main()
{
  #pragma omp parallel num_threads(1)
  {
    omp_control_tool(omp_control_tool_flush, 1, NULL);
  }

  // Check if libomp supports the callbacks for this test.
  // CHECK-NOT: {{^}}0: Could not register callback 'ompt_callback_control_tool'

  // CHECK: 0: NULL_POINTER=[[NULL:.*$]]

  // CHECK: {{^}}[[MASTER_ID:[0-9]+]]: ompt_event_control_tool: command=3, modifier=1, arg=(nil), codeptr_ra={{0x[0-f]*}}

  return 0;
}
