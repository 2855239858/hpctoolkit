#include "queues.h"


//*****************************************************************************
// interface functions
//*****************************************************************************


#define Ad(q) q.aptr
#define Ap(q) q->aptr


void
squeue_ptr_set
(
 s_element_ptr_t *p,
 s_element_t *v
)
{
  //todo
}


s_element_t *
squeue_ptr_get
(
 s_element_ptr_t *e
)
{
  //todo
}


s_element_t *
squeue_swap
(
 s_element_ptr_t *q,
 s_element_t *r
)
{
  //todo
}


void
squeue_push
(
 s_element_ptr_t *q,
 s_element_t *e
)
{
  //todo
}


s_element_t *
squeue_pop
(
 s_element_ptr_t *q
)
{
  //todo
}


s_element_t *
squeue_steal
(
 s_element_ptr_t *q
)
{
  //todo
}


void
squeue_reverse
(
 s_element_ptr_t *q
)
{
  //todo
}


void
squeue_forall
(
 s_element_ptr_t *q,
 queue_forall_fn_t fn,
 void *arg
)
{
  //todo
}


void
cqueue_ptr_set
(
 s_element_ptr_t *e,
 s_element_t *v
)
{
  //todo
}


s_element_t *
cqueue_ptr_get
(
 s_element_ptr_t *e
)
{
  //todo
}


s_element_t *
cqueue_swap
(
 s_element_ptr_t *q,
 s_element_t *r
)
{
  //todo
}


void
cqueue_push
(
 s_element_ptr_t *q,
 s_element_t *e
)
{
  //todo
}


s_element_t *
cqueue_pop
(
 s_element_ptr_t *q
)
{
  //todo
}


s_element_t *
cqueue_steal
(
 s_element_ptr_t *q
)
{
  //todo
}


void
cqueue_forall
(
 s_element_ptr_t *q,
 queue_forall_fn_t fn,
 void *arg
)
{
  //todo
}