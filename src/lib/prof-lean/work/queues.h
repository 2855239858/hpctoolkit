#ifndef queues_h
#define queues_h


//*****************************************************************************
// local includes
//*****************************************************************************

#include "../stdatomic.h"

//*****************************************************************************
// macros 
//*****************************************************************************

#define queues_macro_body_ignore(x) ;
#define queues_macro_body_show(x) x;

#define typed_queue_declare_type(type)		\
  typedef typed_queue_elem(type) * typed_queue_elem_ptr(type)

#define typed_queue_declare(type, flavor)		\
  typed_queue(type, flavor, queues_macro_body_ignore)

#define typed_queue_impl(type, flavor)			\
  typed_queue(type, flavor, queues_macro_body_show)

// routine name for a queue operation
#define queue_op(qtype, op) \
  qtype ## _ ## op

// typed queue pointer
#define typed_queue_elem_ptr(type) \
  type ## _ ## s_element_ptr_t

#define typed_queue_elem(type) \
  type ## _ ## s_element_t

#define typed_queue_elem_fn(type, fn) \
  type ## _ ## s_element_ ## fn

// routine name for a typed queue operation
#define typed_queue_op(type, qtype, op) \
  type ## _ ## qtype ## _ ## op


// Operations:
// set:
// get:
// swap:
// push:
// pop:
// steal:
// forall:

// ptr set routine name for a typed queue 
#define typed_queue_elem_ptr_set(type, qtype) \
  typed_queue_op(type, qtype, ptr_set)

// ptr get routine name for a typed queue 
#define typed_queue_elem_ptr_get(type, qtype) \
  typed_queue_op(type, qtype, ptr_get)

// swap routine name for a typed queue 
#define typed_queue_swap(type, qtype) \
  typed_queue_op(type, qtype, swap)

// push routine name for a typed queue 
#define typed_queue_push(type, qtype) \
  typed_queue_op(type, qtype, push)

// pop routine name for a typed queue 
#define typed_queue_pop(type, qtype) \
  typed_queue_op(type, qtype, pop)

// steal routine name for a typed queue 
#define typed_queue_steal(type, qtype) \
  typed_queue_op(type, qtype, steal)

// forall routine name for a typed queue 
#define typed_queue_forall(type, qtype) \
  typed_queue_op(type, qtype, forall)


// define typed wrappers for a queue type 
#define typed_queue(type, qtype, macro) \
  void \
  typed_queue_elem_ptr_set(type, qtype) \
  (typed_queue_elem_ptr(type) e, typed_queue_elem(type) *v) \
  macro({ \
    queue_op(qtype,ptr_set)((s_element_ptr_t *) e, \
      (s_element_t *) v); \
  }) \
\
  typed_queue_elem(type) * \
  typed_queue_elem_ptr_get(type, qtype) \
  (typed_queue_elem_ptr(type) *e) \
  macro({ \
    typed_queue_elem(type) *r = (typed_queue_elem(type) *) \
    queue_op(qtype,ptr_get)((s_element_ptr_t *) e); \
    return r; \
  }) \
\
  typed_queue_elem(type) * \
  typed_queue_swap(type, qtype) \
  (typed_queue_elem_ptr(type) *s, typed_queue_elem(type) *v) \
  macro({ \
    typed_queue_elem(type) *e = (typed_queue_elem(type) *) \
    queue_op(qtype,swap)((s_element_ptr_t *) s, \
      (s_element_t *) v); \
    return e; \
  }) \
\
  void \
  typed_queue_push(type, qtype) \
  (typed_queue_elem_ptr(type) *s, typed_queue_elem(type) *e) \
  macro({ \
    queue_op(qtype,push)((s_element_ptr_t *) s, \
    (s_element_t *) e); \
  }) \
\
  typed_queue_elem(type) * \
  typed_queue_pop(type, qtype) \
  (typed_queue_elem_ptr(type) *s) \
  macro({ \
    typed_queue_elem(type) *e = (typed_queue_elem(type) *) \
    queue_op(qtype,pop)((s_element_ptr_t *) s); \
    return e; \
  }) \
\
  typed_queue_elem(type) * \
  typed_queue_steal(type, qtype) \
  (typed_queue_elem_ptr(type) *s) \
  macro({ \
    typed_queue_elem(type) *e = (typed_queue_elem(type) *) \
    queue_op(qtype,steal)((s_element_ptr_t *) s); \
    return e; \
  }) \
\
 void typed_queue_forall(type, qtype) \
 (typed_queue_elem_ptr(type) *s, \
  void (*fn)(typed_queue_elem(type) *, void *), void *arg) \
  macro({ \
    queue_op(qtype,forall)((s_element_ptr_t *) s, (queue_forall_fn_t) fn, arg);	\
  })



//*****************************************************************************
// types
//*****************************************************************************

typedef struct s_element_ptr_t {
  _Atomic(struct s_element_ptr_t *) aptr;
} s_element_ptr_t;


typedef struct s_element_t {
  s_element_ptr_t next;
} s_element_t;


typedef void (*queue_forall_fn_t)(s_element_t *e, void *arg);



//*****************************************************************************
// interface functions
//*****************************************************************************

//-----------------------------------------------------------------------------
// sequential FIFO queue interface operations
//-----------------------------------------------------------------------------

void
squeue_ptr_set
(
 s_element_ptr_t *e,
 s_element_t *v
);


s_element_t *
squeue_ptr_get
(
 s_element_ptr_t *e
);


// set s->next to point to e and return old value of s->next
s_element_t *
squeue_swap
(
 s_element_ptr_t *s,
 s_element_t *e
);


// push a singleton e or a chain beginning with e onto s
void
squeue_push
(
 s_element_ptr_t *s,
 s_element_t *e
);


// pop a singlegon from s or return 0
s_element_t *
squeue_pop
(
 s_element_ptr_t *s
);


// steal the entire chain rooted at s 
s_element_t *
squeue_steal
(
 s_element_ptr_t *s
);


// reverse the entire chain rooted at s and set s to be the previous tail
void
squeue_reverse
(
 s_element_ptr_t *s
);


// iterate over each element e in the queue, call fn(e, arg) 
void
squeue_forall
(
 s_element_ptr_t *s,
 queue_forall_fn_t fn,
 void *arg
);


//-----------------------------------------------------------------------------
// concurrent FIFO queue interface operations
//-----------------------------------------------------------------------------

void
cqueue_ptr_set
(
 s_element_ptr_t *e,
 s_element_t *v
);


s_element_t *
cqueue_ptr_get
(
 s_element_ptr_t *e
);


// set s->next to point to e and return old value of s->next
s_element_t *
cqueue_swap
(
 s_element_ptr_t *s,
 s_element_t *e
);


// push a singleton e or a chain beginning with e onto s 
void
cqueue_push
(
 s_element_ptr_t *s,
 s_element_t *e
);


// pop a singlegon from s or return 0
s_element_t *
cqueue_pop
(
 s_element_ptr_t *s
);


// steal the entire chain rooted at s 
s_element_t *
cqueue_steal
(
 s_element_ptr_t *s
);


// iterate over each element e in the queue, call fn(e, arg) 
// note: unsafe to use concurrently with cqueue_steal or cqueue_pop 
void
cqueue_forall
(
 s_element_ptr_t *s,
 queue_forall_fn_t fn,
 void *arg
);




#endif