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


//*********************************************************************
// local includes
//*********************************************************************

#include <loadmap.h>

//*********************************************************************

int
fnbounds_init(const char *executable_name);

// fnbounds_enclosing_addr(): Given an instruction pointer (IP) 'ip',
// return the bounds [start, end) of the function that contains 'ip'.
// Also return the load module that contains 'ip' to make
// normalization easy.  All IPs are *unnormalized.*
bool
fnbounds_enclosing_addr(void *ip, void **start, void **end, load_module_t **lm);

load_module_t*
fnbounds_map_dso(const char *module_name, void *start, void *end, struct dl_phdr_info*);

void
fnbounds_fini();

void
fnbounds_release_lock(void);


// fnbounds_table_lookup(): Given an instruction pointer (IP) 'ip',
// return the bounds [start, end) of the function that contains 'ip'.
// All IPs are *normalized.*
int
fnbounds_table_lookup(void **table, int length, void *ip,
		      void **start, void **end);


#include "fnbounds_table_interface.h"
