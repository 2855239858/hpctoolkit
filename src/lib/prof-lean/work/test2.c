#include "stacks.h"

//*****************************************************************************
// unit test
//*****************************************************************************

#define UNIT_TEST 1
#if UNIT_TEST 

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

typedef struct {
    s_element_ptr_t next;
    int value;
} typed_stack_elem(int); // int_q_element_t

typed_stack_declare_type(int);

typed_stack_impl(int, cstack);

typed_stack_elem_ptr(int) queue;

void 
print(typed_stack_elem(int) *e, void *arg)
{
  printf("%d\n", e->value);
}

int main(int argc, char **argv)
{
  int i;
  for (i = 0; i < 10; i++) {
    typed_stack_elem_ptr(int) 
      item = (typed_stack_elem_ptr(int)) malloc(sizeof(typed_stack_elem(int)));
    item->value = i;
    typed_stack_elem_ptr_set(int, cstack)(item, 0);
    typed_stack_push(int, cstack)(&queue, item);
  }
  typed_stack_forall(int, cstack)(&queue, print, 0);
}

#endif

#if 0

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

typedef struct {
    s_element_ptr_t next;
    int value;
} typed_stack_elem(int); // int_q_element_t


typed_stack_elem_ptr(int) queue;

#define qtype cstack

typed_stack(int, qtype)

typed_stack_elem(int) *
typed_stack_elem_fn(int,new)(int value)
{
    typed_stack_elem(int) *e =
      (typed_stack_elem(int) *) malloc(sizeof(int_s_element_t));
    e->value = value;
    typed_stack_elem_ptr_set(int, qtype)(&e->next, 0);
}


void
pop
(
 int n
)
{
  int i;
  for(i = 0; i < n; i++) {
    typed_stack_elem(int) *e = typed_stack_pop(int, qtype)(&queue);
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
  for(i = min; i < min+n; i++) {
    printf("%d pushing %d\n", omp_get_thread_num(), i);
    typed_stack_push(int, qtype)(&queue, typed_stack_elem_fn(int, new)(i));
  }
}


void
dump
(
 int_s_element_t *e
)
{
  int i;
  for(; e; 
      e = (int_s_element_t *) typed_stack_elem_ptr_get(int,qtype)(&e->next)) {
    printf("%d stole %d\n", omp_get_thread_num(), e->value);
  }
}


int
main
(
 int argc,
 char **argv
)
{
  typed_stack_elem_ptr_set(int, qtype)(&queue, 0);
#pragma omp parallel
  {
    push(0, 30);
    pop(10);
    push(100, 12);
    // pop(100);
    int_s_element_t *e = typed_stack_steal(int, qtype)(&queue);
    dump(e);
    push(300, 30);
    typed_stack_push(int, qtype)(&queue, e);
    pop(100);
  }
}

#endif
