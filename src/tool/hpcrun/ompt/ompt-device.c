// -*-Mode: C++;-*- // technically C99

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$ 
// $Id$
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2002-2013, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *


#include "ompt-device.h"

#if HAVE_CUPTI_H

/******************************************************************************
 * global include files
 *****************************************************************************/

#include <sys/param.h>



//*****************************************************************************
// local includes
//*****************************************************************************

#include <hpcrun/safe-sampling.h>
#include <hpcrun/thread_data.h>
#include <hpcrun/device-finalizers.h>
#include <hpcrun/sample_event.h>

#include "ompt-interface.h"
#include "ompt-device-map.h"
#include "ompt-placeholders.h"

#include "sample-sources/nvidia/cupti-api.h"
#include "sample-sources/nvidia/cupti-record.h"
#include "sample-sources/nvidia/nvidia.h"



//*****************************************************************************
// macros
//*****************************************************************************

#define OMPT_ACTIVITY_DEBUG 0

#if OMPT_ACTIVITY_DEBUG
#define PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define PRINT(...)
#endif

#define OMPT_API_FNTYPE(fn) fn##_t

#define OMPT_API_FUNCTION(return_type, fn, args)  \
  typedef return_type (*OMPT_API_FNTYPE(fn)) args

#define OMPT_TARGET_API_FUNCTION(return_type, fn, args)  \
  OMPT_API_FUNCTION(return_type, fn, args) 

#define FOREACH_OMPT_TARGET_FN(macro) \
  macro(ompt_get_device_time) \
  macro(ompt_translate_time) \
  macro(ompt_set_trace_native) \
  macro(ompt_start_trace) \
  macro(ompt_pause_trace) \
  macro(ompt_stop_trace) \
  macro(ompt_get_record_type) \
  macro(ompt_get_record_native) \
  macro(ompt_get_record_abstract) \
  macro(ompt_advance_buffer_cursor) \
  macro(ompt_set_pc_sampling) 



//*****************************************************************************
// types 
//*****************************************************************************

OMPT_TARGET_API_FUNCTION(void, ompt_set_pc_sampling, 
(
 ompt_device_t *device,
 int enable,
 int pc_sampling_frequency
));



//*****************************************************************************
// static variables
//*****************************************************************************

static bool ompt_pc_sampling_enabled = false;

static device_finalizer_fn_entry_t device_finalizer;


//------------------------------------------------
// declare function pointers for target functions
//------------------------------------------------

#define ompt_decl_name(fn) \
  fn ## _t  fn;

FOREACH_OMPT_TARGET_FN(ompt_decl_name)

#undef ompt_decl_name


//*****************************************************************************
// thread-local variables
//*****************************************************************************

static __thread cct_node_t *target_node = NULL;




//*****************************************************************************
// device operations
//*****************************************************************************

static void
hpcrun_ompt_op_id_notify(ompt_id_t host_op_id,  ompt_placeholder_t ph)
{
  // create a cct node for the placeholder as a child of target_node
  cct_addr_t frm;
  memset(&frm, 0, sizeof(cct_addr_t));
  frm.ip_norm = ph.pc_norm;
  cct_node_t* cct_child = hpcrun_cct_insert_addr(target_node, &frm);

  // inform the worker about the placeholder
  // FIXME(Keren): NULL func node does not work for OMPT
  cupti_worker_notification_apply(host_op_id, cct_child, NULL);
}


void 
ompt_bind_names(ompt_function_lookup_t lookup)
{
#define ompt_bind_name(fn) \
  fn = (fn ## _t ) lookup(#fn);

  FOREACH_OMPT_TARGET_FN(ompt_bind_name)

#undef ompt_bind_name
}


#define BUFFER_SIZE (1024 * 1024 * 8)

void 
ompt_callback_buffer_request
(
 int device_id,
 ompt_buffer_t **buffer,
 size_t *bytes
)
{
  *bytes = BUFFER_SIZE;
  *buffer = (ompt_buffer_t *)malloc(*bytes);
  assert(buffer);
}


void 
ompt_callback_buffer_complete
(
 int device_id,
 ompt_buffer_t *buffer,
 size_t bytes,
 ompt_buffer_cursor_t begin,
 int buffer_owned
)
{
  // handle notifications
  cupti_cupti_notification_apply(cupti_notification_handle);
  // signal advance to return pointer to first record
  ompt_buffer_cursor_t next = begin;
  int status = 0;
  do {
    // TODO(keren): replace cupti_activity_handle with device_activity handle
    CUpti_Activity *activity = (CUpti_Activity *)next;
    cupti_activity_process(activity);
    status = cupti_buffer_cursor_advance(buffer, bytes, (CUpti_Activity **)&next);
  } while(status);
}


void
ompt_pc_sampling_enable()
{
  ompt_pc_sampling_enabled = true;
}


void
ompt_pc_sampling_disable()
{
  ompt_pc_sampling_enabled = false;
}


void
ompt_trace_configure(ompt_device_t *device)
{
  int flags = 0;

  // specify desired monitoring
  flags |= ompt_native_driver;

  flags |= ompt_native_runtime;

  flags |= ompt_native_kernel_invocation;

  flags |= ompt_native_kernel_execution;

  flags |= ompt_native_data_motion_explicit;

  // indicate desired monitoring
  ompt_set_trace_native(device, 1, flags);
  
  // set pc sampling after other traces
  if (ompt_pc_sampling_enabled) {
    ompt_set_pc_sampling(device, true, cupti_pc_sampling_frequency_get());
  }

  // turn on monitoring previously indicated
  ompt_start_trace(device, ompt_callback_buffer_request, ompt_callback_buffer_complete);
}


void
ompt_device_initialize(uint64_t device_num,
                       const char *type,
                       ompt_device_t *device,
                       ompt_function_lookup_t lookup,
                       const char *documentation)
{
  PRINT("ompt_device_initialize->%s, %d\n", type, device_num);

  ompt_bind_names(lookup);

  ompt_trace_configure(device);

  ompt_device_map_insert(device_num, device, type);
}


void 
ompt_device_finalize(uint64_t device_num)
{
}


void 
ompt_device_load(uint64_t device_num,
                 const char *filename,
                 int64_t file_offset,
                 const void *file_addr,
                 size_t bytes,
                 const void *host_addr,
                 const void *device_addr,
                 uint64_t module_id)
{
  PRINT("ompt_device_load->%s, %d\n", filename, device_num);
  cupti_load_callback_cuda(module_id, host_addr, bytes);
}


void 
ompt_device_unload(uint64_t device_num,
                   uint64_t module_id)
{
  //cubin_id_map_delete(module_id);
}


static int 
get_load_module
(
  cct_node_t *node
)
{
    cct_addr_t *addr = hpcrun_cct_addr(target_node); 
    ip_normalized_t ip = addr->ip_norm;
    return ip.lm_id;
}

void 
ompt_target_callback
(
  ompt_target_t kind,
  ompt_scope_endpoint_t endpoint,
  uint64_t device_num,
  ompt_data_t *task_data,
  ompt_id_t target_id,
  const void *codeptr_ra
)
{
  PRINT("ompt_target_callback->target_id %d\n", target_id);
  // If a thread creates a target region, we init records
  // and it must be flushed in the finalizer
  cupti_stop_flag_set();
  // TODO(Keren) ignore cupti thread callback
  cupti_record_init();

  // process cupti records
  cupti_worker_activity_apply(cupti_activity_handle);

  // sample a record
  hpcrun_metricVal_t zero_metric_incr = {.i = 0};
  int zero_metric_id = 0; // nothing to see here

  ucontext_t uc;
  getcontext(&uc);
  thread_data_t *td = hpcrun_get_thread_data();
  td->overhead++;
  // NOTE(keren): hpcrun_safe_enter prevent self interruption
  hpcrun_safe_enter();
  
  int skip_this_frame = 1; // omit this procedure frame on the call path
  target_node = 
    hpcrun_sample_callpath(&uc, zero_metric_id, zero_metric_incr, 
                           skip_this_frame, 1, NULL).sample_node; 

  // the load module for the runtime library that supports offloading
  int lm = get_load_module(target_node); 

  // drop nodes on the call chain until we find one that is not in the load 
  // module for runtime library that supports offloading
  for (;;) { 
    target_node = hpcrun_cct_parent(target_node);
    if (get_load_module(target_node) != lm) break;
  }

  hpcrun_safe_exit();
  td->overhead--;
}

#define FOREACH_OMPT_DATA_OP(macro)				     \
  macro(op, ompt_target_data_alloc, ompt_tgt_alloc)		     \
  macro(op, ompt_target_data_delete, ompt_tgt_delete)		     \
  macro(op, ompt_target_data_transfer_to_device, ompt_tgt_copyin)    \
  macro(op, ompt_target_data_transfer_from_device, ompt_tgt_copyout) 

void
ompt_data_op_callback(ompt_id_t target_id,
                      ompt_id_t host_op_id,
                      ompt_target_data_op_t optype,
                      void *host_addr,
                      void *device_addr,
                      size_t bytes)
{
  ompt_placeholder_t op = ompt_placeholders.ompt_tgt_none;
  switch (optype) {                       
#define ompt_op_macro(op, ompt_op_type, ompt_op_class) \
    case ompt_op_type:                                 \
      op = ompt_placeholders.ompt_op_class;                              \
      break;
    
    FOREACH_OMPT_DATA_OP(ompt_op_macro);

#undef ompt_op_macro
    default:
      break;
  }
  hpcrun_ompt_op_id_notify(host_op_id, op);
}


void
ompt_submit_callback(ompt_id_t target_id,
                     ompt_id_t host_op_id)
{
  PRINT("ompt_submit_callback enter->target_id %d\n", target_id);
  hpcrun_ompt_op_id_notify(host_op_id, ompt_placeholders.ompt_tgt_kernel);
  PRINT("ompt_submit_callback exit->target_id %d\n", target_id);
}


void
ompt_map_callback(ompt_id_t target_id,
                  unsigned int nitems,
                  void **host_addr,
                  void **device_addr,
                  size_t *bytes,
                  unsigned int *mapping_flags)
{
}



void
prepare_device()
{
  PRINT("ompt_initialize->prepare_device enter\n");

  device_finalizer.fn = cupti_device_flush;
  device_finalizer_register(device_finalizer_type_flush, &device_finalizer);

  ompt_set_callback(ompt_callback_device_initialize, ompt_device_initialize);
  ompt_set_callback(ompt_callback_device_finalize, ompt_device_finalize);
  ompt_set_callback(ompt_callback_device_load, ompt_device_load);
  ompt_set_callback(ompt_callback_device_unload, ompt_device_unload);
  ompt_set_callback(ompt_callback_target, ompt_target_callback);
  ompt_set_callback(ompt_callback_target_data_op, ompt_data_op_callback);
  ompt_set_callback(ompt_callback_target_submit, ompt_submit_callback);
  ompt_set_callback(ompt_callback_target_map, ompt_map_callback);

  PRINT("ompt_initialize->prepare_device exit\n");
}

#endif
