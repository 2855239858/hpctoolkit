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


/******************************************************************************
 * system includes
 *****************************************************************************/

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>



/******************************************************************************
 * libmonitor
 *****************************************************************************/

#include <monitor.h>



/******************************************************************************
 * local includes
 *****************************************************************************/

#include <hpcrun/hpcrun_options.h>
#include <hpcrun/disabled.h>
#include <hpcrun/metrics.h>
#include <sample_event.h>
#include "sample_source_obj.h"
#include "common.h"
#include <main.h>
#include <hpcrun/sample_sources_registered.h>
#include "simple_oo.h"
#include <hpcrun/thread_data.h>

#include <messages/messages.h>


static int hpmpi_metric_id = -1;

/******************************************************************************
 * method definitions
 *****************************************************************************/

static void
METHOD_FN(init)
{
  self->state = INIT; 
}


static void
METHOD_FN(thread_init)
{
  TMSG(MPI, "thread init (no action needed)");
}

static void
METHOD_FN(thread_init_action)
{
  TMSG(MPI, "thread action (noop)");
}

static void
METHOD_FN(start)
{
  TMSG(MPI,"starting MPI");

  TD_GET(ss_state)[self->sel_idx] = START;
}

static void
METHOD_FN(thread_fini_action)
{
  TMSG(MPI, "thread fini (noop)");
}

static void
METHOD_FN(stop)
{
  TMSG(MPI,"stopping MPI");
  TD_GET(ss_state)[self->sel_idx] = STOP;
}


static void
METHOD_FN(shutdown)
{
  METHOD_CALL(self,stop); // make sure stop has been called
  self->state = UNINIT;
}


static bool
METHOD_FN(supports_event,const char *ev_str)
{
  return hpcrun_ev_is(ev_str,"MPI_MSG");
}
 

// MPI_MSG creates one metrics: bytes sent/received in bytes

static void
METHOD_FN(process_event_list,int lush_metrics)
{
  kind_info_t *mpi_kind = hpcrun_metrics_new_kind();
  hpmpi_metric_id = hpcrun_set_new_metric_info(mpi_kind, "MPI_MSG(B)");
  hpcrun_close_kind(mpi_kind);
  TMSG(MPI, "Setting up metrics for MPI: %d", hpmpi_metric_id);
}

static void
METHOD_FN(finalize_event_list)
{
}

//
// Event sets not relevant for this sample source
// Events are generated by user code
//
static void
METHOD_FN(gen_event_set,int lush_metrics)
{
}


//
//
//
static void
METHOD_FN(display_events)
{
  printf("===========================================================================\n");
  printf("Available MPI events\n");
  printf("===========================================================================\n");
  printf("Name\t\tDescription\n");
  printf("---------------------------------------------------------------------------\n");
  printf("MPI_MSG\t\tThe number of bytes for all sent/received messages\n");
  printf("\n");
}


/***************************************************************************
 * object
 ***************************************************************************/

//
// sync class is "SS_SOFTWARE" so that both synchronous and asynchronous sampling is possible
// 

#define ss_name hpmpi
#define ss_cls SS_SOFTWARE

#include "ss_obj.h"


// ***************************************************************************
//  Interface functions
// ***************************************************************************

// increment the return count
//
// N.B. : This function is necessary to avoid exposing the retcnt_obj.
//        For the case of the retcnt sample source, the handler (the trampoline)
//        is separate from the sample source code.
//        Consequently, the interaction with metrics must be done procedurally

int
hpcrun_mpi_metric_id()
{
  return hpmpi_metric_id;
}



void
hpcrun_mpi_inc(cct_node_t* node, int incr)
{
  if (node != NULL) {
    TMSG(MPI, "\tmpi (cct node %p): metric[%d] += %d",
	 node, hpmpi_metric_id, incr);
    cct_metric_data_increment(hpmpi_metric_id,
			       node,
			      (cct_metric_data_t){.i = incr});
  }
}


