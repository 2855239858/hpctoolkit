
//*****************************************************************************
// unit test
//*****************************************************************************

#include "bistack.h"

#define UNIT_TEST 1
#if UNIT_TEST

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <unistd.h>

typedef struct {
  s_element_ptr_t next;
  int value;
} typed_stack_elem(int); //int_q_element_t

typedef s_element_ptr_t typed_stack_elem_ptr(int);	 //int_q_elem_ptr_t
typedef bistack_t typed_bistack(int);

//typed_queue_elem_ptr(int) queue;
typed_bistack(int) pair;

typed_bistack_impl(int)

typed_stack_elem(int) *
typed_stack_elem_fn(int,new)(int value)
{
  typed_stack_elem(int) *e = 
    (typed_stack_elem(int)* ) malloc(sizeof(int_s_element_t));
  e->value = value;
  cstack_ptr_set(&e->next, 0);
}


void
pop
(
 int n
)
{
  int i;
  for(i = 0; i < n; i++) {
    typed_stack_elem(int) *e = typed_bistack_pop(int)(&pair);
    if (e == 0) {
      printf("%d queue empty\n", omp_get_thread_num());
      break;
    } else {
      printf("%d popping %d\n", omp_get_thread_num(), e->value);
    }
  }
}


void
push
(
 int min,
 int n
)
{
  int i;
  for(i = min; i < min + n; i++) {
    printf("%d pushing %d\n", omp_get_thread_num(), i);
    typed_bistack_push(int)(&pair, typed_stack_elem_fn(int, new)(i));
  }
}


void
steal
(
)
{
  typed_bistack_steal(int)(&pair);
}


#ifdef DUMP_UNORDERED_STACK
void
dump
(
 int_s_element_t *e
)
{
  int i;
  for(; e; 
      e = (int_s_element_t *) typed_stack_elem_ptr_get(int,cstack)(&e->next)) {
    printf("%d stole %d\n", omp_get_thread_num(), e->value);
  }
}

#endif


int
main
(
 int argc,
 char **argv
)
{
  bistack_init(&pair);
#pragma omp parallel num_threads(6)
  {
    if (omp_get_thread_num() != 5 ) push(0, 30);
    if (omp_get_thread_num() == 5 ) {
      sleep(3);
      steal();
      pop(10);
    }
    if (omp_get_thread_num() != 5 ) push(100, 12);
    // pop(100);
    // int_bis_element_t *e = typed_bistack_steal(int, qtype)(&queue);
    //dump(e);
    if (omp_get_thread_num() != 5 ) push(300, 30);
    //typed_queue_
    if (omp_get_thread_num() == 5 ) {
      sleep(1);
      steal();
      pop(100);
    }
  }
}

#endif
