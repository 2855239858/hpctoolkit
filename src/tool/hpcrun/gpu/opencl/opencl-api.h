// -*-Mode: C++;-*- // technically C99

// * BeginRiceCopyright *****************************************************
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2002-2022, Rice University
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

#ifndef _OPENCL_API_H_
#define _OPENCL_API_H_

//******************************************************************************
// local includes
//******************************************************************************

#include <hpcrun/gpu/gpu-activity.h>
#include <lib/prof-lean/hpcrun-opencl.h>
#include "opencl-memory-manager.h"



//************************ Forward Declarations ******************************

typedef struct opencl_object_t opencl_object_t;



//******************************************************************************
// interface operations
//******************************************************************************

cl_basic_callback_t
opencl_cb_basic_get
(
 opencl_object_t *cb_data
);


void
opencl_cb_basic_print
(
 cl_basic_callback_t cb_basic,
 char *title
);


void
opencl_initialize_correlation_id
(
 void
);


void
opencl_subscriber_callback
(
 opencl_object_t *cb_info
);


void
opencl_activity_completion_callback
(
 cl_event,
 cl_int,
 void *
);


void
opencl_timing_info_get
(
 gpu_interval_t *,
 cl_event
);


cct_node_t *
opencl_api_node_get
(
 void
);


void
clSetEventCallback_wrapper
(
 cl_event,
 cl_int,
 void (CL_CALLBACK*)(cl_event, cl_int, void *),
 void *
);


void
opencl_api_initialize
(
 void
);


int
opencl_bind
(
 void
);


void
opencl_instrumentation_enable
(
 void
);


void
opencl_api_thread_finalize
(
 void *args,
 int how
);


void
opencl_api_process_finalize
(
 void *args,
 int how
);


#endif  //_OPENCL_API_H_
