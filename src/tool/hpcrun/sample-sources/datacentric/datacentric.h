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
// Copyright ((c)) 2002-2019, Rice University
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


#ifndef sample_source_datacentric_h
#define sample_source_datacentric_h

/******************************************************************************
 * local includes 
 ******************************************************************************/

#include <cct/cct.h>                       // cct_node_t
#include "sample-sources/perf/perf-util.h" // event_info_t
#include "sample-sources/perf/event_custom.h" // event_custom_t


/******************************************************************************
 *  MACROs
 ******************************************************************************/

#define DATACENTRIC_METRIC_PREFIX  ""


/******************************************************************************
 *  interface operations
 ******************************************************************************/

void datacentric_init();

int datacentric_is_active();

int datacentric_get_metric_variable_size();


// ----------------------------------------------
// to be implemented by specific hardware
// ----------------------------------------------

// called to create events and the metric has to be stored by store_events_and_info
int 
datacentric_hw_register(sample_source_t          *self, 
                        event_custom_t           *event,
                        struct event_threshold_s *period);

// called when a sample occurs
void
datacentric_hw_handler(perf_mmap_data_t *mmap_data,
                       cct_node_t       *datacentric_node,
                       cct_node_t       *sample_node);

#endif // sample_source_memleak_h

