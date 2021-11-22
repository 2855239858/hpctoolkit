#include "cupti-range.h"

#include <string.h>
#include <stdbool.h>

#include <hpcrun/cct/cct.h>
#include <hpcrun/gpu/gpu-metrics.h>
#include <hpcrun/gpu/gpu-range.h>
#include <hpcrun/gpu/gpu-correlation-id.h>

#include "cuda-api.h"
#include "cupti-api.h"
#include "cupti-ip-norm-map.h"
#include "cupti-pc-sampling-api.h"
#include "cupti-cct-trie.h"
#include "cupti-range-thread-list.h"

static cupti_range_mode_t cupti_range_mode = CUPTI_RANGE_MODE_NONE;

static uint32_t cupti_range_interval = CUPTI_RANGE_DEFAULT_INTERVAL;
static uint32_t cupti_range_sampling_period = CUPTI_RANGE_DEFAULT_SAMPLING_PERIOD;

#ifdef NEW_CUPTI_ANALYSIS
static __thread int sampled_times = 0;
#endif

static bool
cupti_range_pre_enter_callback
(
 uint64_t correlation_id,
 void *args
)
{
  TMSG(CUPTI_TRACE, "Enter CUPTI range pre correlation_id %lu, range_id %u", correlation_id, gpu_range_id_get());
  return cupti_range_mode != CUPTI_RANGE_MODE_NONE;
}


static void
cupti_range_kernel_count_increase
(
 cct_node_t *kernel_ph,
 uint32_t context_id,
 uint32_t range_id
)
{
  // Range processing
  cct_node_t *node = hpcrun_cct_children(kernel_ph);
  node = hpcrun_cct_insert_context(node, context_id);
  node = hpcrun_cct_insert_range(node, range_id);

  // Increase kernel count
  gpu_metrics_attribute_kernel_count(node, 1, 1);
}


static bool
cupti_range_mode_even_is_enter
(
 CUcontext context,
 cct_node_t *kernel_ph,
 uint64_t correlation_id,
 uint32_t range_id
)
{
	uint32_t context_id = ((hpctoolkit_cuctx_st_t *)context)->context_id;
  // Increase kernel count for postmortem apportion based on counts
  cupti_range_kernel_count_increase(kernel_ph, context_id, range_id);
  return (GPU_CORRELATION_ID_UNMASK(correlation_id) % cupti_range_interval) == 0;
}


static bool
cupti_range_mode_is_sampled
(
)
{
  int left = rand() % cupti_range_sampling_period;
  if (left == 0) {
    return true;
  } else {
    return false;
  }
}


static bool
cupti_range_mode_context_sensitive_is_enter
(
 CUcontext context,
 cct_node_t *kernel_ph,
 uint64_t correlation_id,
 uint32_t range_id
)
{
  static bool first_range = true;

	uint32_t context_id = ((hpctoolkit_cuctx_st_t *)context)->context_id;

  // Add the current thread to the list
  cupti_range_thread_list_add();

  // First handle my pending notification
  cupti_cct_trie_notification_process();

  // Then handle the current request
  ip_normalized_t kernel_ip = hpcrun_cct_addr(hpcrun_cct_children(kernel_ph))->ip_norm;
  cct_node_t *api_node = hpcrun_cct_parent(kernel_ph);
  cupti_ip_norm_map_ret_t map_ret_type = cupti_ip_norm_map_lookup_thread(kernel_ip, api_node);
  cupti_ip_norm_map_ret_t global_map_ret_type = cupti_ip_norm_global_map_lookup(kernel_ip, api_node);

  bool active = cupti_pc_sampling_active();
  uint32_t prev_range_id = GPU_RANGE_NULL;
  uint32_t next_range_id = range_id;
  if (map_ret_type == CUPTI_IP_NORM_MAP_DUPLICATE ||
    global_map_ret_type == CUPTI_IP_NORM_MAP_DUPLICATE) {
    // If active, we add a sampled kernel count; otherwise, we add a non-sampled kernel count
    // If logic, we don't unwind the current path in the cct trie
    bool logic = map_ret_type != CUPTI_IP_NORM_MAP_DUPLICATE;
    prev_range_id = cupti_cct_trie_flush(context_id, range_id, active, logic);
    if (active) {
      // If active, we have to flush pc samples and attribute them to nodes with prev_range_id
      // It is an early collection mode different than other modes
      // The whole range is repeated with a previous range
      // Special case: if prev_range_id == GPU_RANGE_NULL, it means no thread has made any progress
      cupti_pc_sampling_range_context_collect(prev_range_id, context);
    }
    if (!logic) {
      // After a real flushing,
      // we clean up ccts in the previous range and start a new range
      cupti_ip_norm_map_clear_thread();
    } 
    next_range_id += 1;
  }

  // Add a new node
  cupti_ip_norm_map_insert_thread(kernel_ip, api_node);
  cupti_ip_norm_global_map_insert(kernel_ip, api_node);

  // Update active status
  active = cupti_pc_sampling_active();
 
  bool repeated = cupti_cct_trie_append(next_range_id, api_node);
  bool sampled = false;
  bool new_range = false;

  printf("thread %d global %d local %d prev_range_id %d cur_range_id %d next_range_id %d active %d\n", cupti_range_thread_list_id_get(), global_map_ret_type, map_ret_type, prev_range_id, range_id, next_range_id, active);
  
  if (!active) {
    if (map_ret_type == CUPTI_IP_NORM_MAP_DUPLICATE ||
      global_map_ret_type == CUPTI_IP_NORM_MAP_DUPLICATE) {
      // 1. abc | (a1)bc
      // a1 conflicts a, it must be a new rnage
      new_range = true;
      if (cupti_range_mode_is_sampled()) {
        sampled = true;
      }
    } else if (!repeated) {
      // 2. abc | abc | d
      // We haven't seen d before, though turning on sampling, it is not a new range
      sampled = true;

      if (!first_range && map_ret_type == CUPTI_IP_NORM_MAP_NOT_EXIST) {
        // Flush does not affect the node just inserted, so we need to unwind it and reinsert it
        cupti_cct_trie_unwind();
        // We are going to extend the path of the current trie, so don't unwind to the root
        cupti_cct_trie_flush(context_id, range_id, active, true);
        cupti_cct_trie_append(next_range_id, api_node);
      }
    } 
      
    if (sampled) {
#ifdef NEW_CUPTI_ANALYSIS
      ++sampled_times;
#endif
      cupti_pc_sampling_start(context);
    }
  }

  // We always turn on pc sampling for the first range.
  // So the first range does not increase range_id
  if (first_range) {
    first_range = false;
    new_range = false;
  } 

  return new_range;
}


static bool
cupti_range_post_enter_callback
(
 uint64_t correlation_id,
 void *args
)
{
  TMSG(CUPTI_TRACE, "Enter CUPTI range post correlation_id %lu range_id %u", correlation_id, gpu_range_id_get());

  CUcontext context;
  cuda_context_get(&context);
  uint32_t range_id = gpu_range_id_get();
  cct_node_t *kernel_ph = (cct_node_t *)args;

  bool ret = false;

  if (cupti_range_mode == CUPTI_RANGE_MODE_EVEN) {
    ret = cupti_range_mode_even_is_enter(context, kernel_ph, correlation_id, range_id);
  } else if (cupti_range_mode == CUPTI_RANGE_MODE_CONTEXT_SENSITIVE) {
    ret = cupti_range_mode_context_sensitive_is_enter(context, kernel_ph, correlation_id, range_id);
  }

  // TODO(Keren): check if pc sampling buffer is full
  // If full, ret = true

  return ret;
}


static bool
cupti_range_pre_exit_callback
(
 uint64_t correlation_id,
 void *args
)
{
  TMSG(CUPTI_TRACE, "Exit CUPTI range pre correlation_id %lu range_id %u", correlation_id, gpu_range_id_get());

  return cupti_range_mode != CUPTI_RANGE_MODE_NONE;
}


static void
cupti_range_mode_even_is_exit
(
 uint64_t correlation_id,
 CUcontext context
)
{
  if (!gpu_range_is_lead()) {
    return;
  }

  // Collect pc samples from all contexts
  uint32_t range_id = gpu_range_id_get();
  cupti_pc_sampling_range_context_collect(range_id, context);
  if (cupti_range_mode_is_sampled()) {
    // Restart pc sampling immediately if sampled
    cupti_pc_sampling_start(context);
  }
}


static bool
cupti_range_post_exit_callback
(
 uint64_t correlation_id,
 void *args
)
{
  TMSG(CUPTI_TRACE, "Exit CUPTI range post correlation_id %lu range_id %u", correlation_id, gpu_range_id_get());

  CUcontext context;
  cuda_context_get(&context);

  if (cupti_range_mode == CUPTI_RANGE_MODE_SERIAL) {
    // Collect pc samples from the current context
    cupti_pc_sampling_correlation_context_collect(cupti_kernel_ph_get(), context);
  } else if (cupti_range_mode == CUPTI_RANGE_MODE_EVEN) {
    cupti_range_mode_even_is_exit(correlation_id, context);
  }

  return false;
}


void
cupti_range_config
(
 const char *mode_str,
 int interval,
 int sampling_period
)
{
  TMSG(CUPTI, "Enter cupti_range_config mode %s, interval %d, sampling period %d", mode_str, interval, sampling_period);

  gpu_range_enable();

  cupti_range_interval = interval;
  cupti_range_sampling_period = sampling_period;

  // Range mode is only enabled with option "gpu=nvidia,pc"
  //
  // In the even mode, pc samples are collected for every n kernels.
  // If n == 1, we fall back to the serialized pc sampling collection,
  // in which pc samples are collected after every kernel launch.
  //
  // In the context sensitive mode, pc samples are flushed based on
  // the number of kernels belong to different contexts.
  // We don't flush pc samples unless a kernel in the range is launched
  // by two different contexts.
  if (strcmp(mode_str, "EVEN") == 0) {
    if (cupti_range_sampling_period != CUPTI_RANGE_DEFAULT_SAMPLING_PERIOD ||
      interval > CUPTI_RANGE_DEFAULT_INTERVAL) {
      cupti_range_mode = CUPTI_RANGE_MODE_EVEN;
    } else {
      cupti_range_mode = CUPTI_RANGE_MODE_SERIAL;
    }
  } else if (strcmp(mode_str, "CONTEXT_SENSITIVE") == 0) {
    cupti_range_mode = CUPTI_RANGE_MODE_CONTEXT_SENSITIVE;
  }

  if (cupti_range_mode != CUPTI_RANGE_MODE_NONE) {
    gpu_range_enter_callbacks_register(cupti_range_pre_enter_callback,
      cupti_range_post_enter_callback);
    gpu_range_exit_callbacks_register(cupti_range_pre_exit_callback,
      cupti_range_post_exit_callback);
  }

  TMSG(CUPTI, "Exit cupti_range_config");
}


cupti_range_mode_t
cupti_range_mode_get
(
 void
)
{
  return cupti_range_mode;
}


uint32_t
cupti_range_interval_get
(
 void
)
{
  return cupti_range_interval;
}


uint32_t
cupti_range_sampling_period_get
(
 void
)
{
  return cupti_range_sampling_period;
}


void
cupti_range_thread_last
(
)
{
  if (cupti_range_mode != CUPTI_RANGE_MODE_CONTEXT_SENSITIVE) {
    return;
  }

  gpu_range_lock();

  cupti_cct_trie_notification_process();
  cupti_ip_norm_map_clear_thread();

  gpu_range_unlock();
}


void
cupti_range_last
(
)
{
  if (cupti_range_mode == CUPTI_RANGE_MODE_SERIAL) {
    return;
  }

  CUcontext context;
  cuda_context_get(&context);
  uint32_t range_id = gpu_range_id_get();

  if (cupti_range_mode == CUPTI_RANGE_MODE_EVEN) {
    cupti_pc_sampling_range_context_collect(range_id, context);
  } else if (cupti_range_mode == CUPTI_RANGE_MODE_CONTEXT_SENSITIVE) {
    uint32_t context_id = ((hpctoolkit_cuctx_st_t *)context)->context_id;
    bool active = cupti_pc_sampling_active();
    // No need to unwind to the root since this is the last flush call
    uint32_t prev_range_id = cupti_cct_trie_flush(context_id, range_id, active, true);

    if (active) {
      // The whole range is repeated with a previous range
      cupti_pc_sampling_range_context_collect(prev_range_id, context);
    }
  }

#ifdef NEW_CUPTI_ANALYSIS
  printf("sampled times %d\n", sampled_times);
#endif
}
