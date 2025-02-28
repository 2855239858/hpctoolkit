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


//******************************************************************************
// File: ss-list.h
//
// Purpose: 
//   This file contains a list of sample sources wrapped by a call to an
//   unspecified macro. The intended use of this file is to define the
//   macro, include the file elsewhere one or more times to register the
//   sample sources. This is not defined as a FORALL macro that applies
//   a macro to each of the sample source names so that this file can
//   contain ifdefs if a sample source is unused on a platform.
//
//******************************************************************************

#include <include/hpctoolkit-config.h>

SAMPLE_SOURCE_DECL_MACRO(ga)
SAMPLE_SOURCE_DECL_MACRO(io)  
#ifdef ENABLE_CLOCK_REALTIME
SAMPLE_SOURCE_DECL_MACRO(itimer)  
#endif

#ifdef HPCRUN_SS_LINUX_PERF
SAMPLE_SOURCE_DECL_MACRO(linux_perf)  
#endif

SAMPLE_SOURCE_DECL_MACRO(memleak)  

SAMPLE_SOURCE_DECL_MACRO(none)  

#ifdef HPCRUN_SS_PAPI
SAMPLE_SOURCE_DECL_MACRO(papi)  
#endif

SAMPLE_SOURCE_DECL_MACRO(directed_blame)

#ifdef HOST_CPU_x86_64
SAMPLE_SOURCE_DECL_MACRO(retcnt)
#endif

#ifdef HPCRUN_SS_PAPI_C_CUPTI
SAMPLE_SOURCE_DECL_MACRO(papi_c_cupti)
#endif

#ifdef HPCRUN_SS_PAPI_C_ROCM
SAMPLE_SOURCE_DECL_MACRO(papi_c_rocm)
#endif

#ifdef HPCRUN_SS_NVIDIA
SAMPLE_SOURCE_DECL_MACRO(nvidia_gpu)
#endif

#ifdef HPCRUN_SS_AMD
#ifndef HPCRUN_STATIC_LINK
SAMPLE_SOURCE_DECL_MACRO(amd_gpu)
#endif
#endif

SAMPLE_SOURCE_DECL_MACRO(openmp_gpu)

#ifdef HPCRUN_SS_AMD
#ifndef HPCRUN_STATIC_LINK
SAMPLE_SOURCE_DECL_MACRO(amd_rocprof)
#endif
#endif

#ifdef HPCRUN_SS_LEVEL0
SAMPLE_SOURCE_DECL_MACRO(level0)
#endif
#ifndef HPCRUN_STATIC_LINK
#ifdef HPCRUN_SS_OPENCL
SAMPLE_SOURCE_DECL_MACRO(opencl)
#endif
#endif

