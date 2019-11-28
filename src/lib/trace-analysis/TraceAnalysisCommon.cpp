// -*-Mode: C++;-*-

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
// Copyright ((c)) 2002-2017, Rice University
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

/* 
 * File:   TraceAnalysisCommon.cpp
 * Author: Lai Wei <lai.wei@rice.edu>
 *
 * Created on March 5, 2018, 2:00 PM
 * 
 * Trace analysis common types and utilities.
 */

#include <stdarg.h>
#include <math.h>

#include "TraceAnalysisCommon.hpp"

namespace TraceAnalysis {
  Time startTime = 0;
  
  Time getStartTime() {
    return startTime;
  }
  
  void setStartTime(Time time) {
    startTime = time;
  }
  
  string vmaToHexString(VMA vma) {
    static const char* digits = "0123456789abcdef";
    
    size_t hex_len = 6;
    for (; hex_len < sizeof(VMA)*2; hex_len++)
      if ((vma >> (hex_len*4)) == 0) break;
    
    std::string rc(hex_len,'0');
    for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
        rc[i] = digits[(vma>>j) & 0x0f];
    return rc;
  }
  
  string timeToString(Time time) {
    double t = (double)time / 1000000.0;
    return std::to_string(t) + "s";
  }
  
  #define MSG_LEVEL MSG_PRIO_HIGH
  void print_msg(int level, const char *fmt,...) {
    if (level >= MSG_LEVEL) {
      va_list args;
      va_start(args, fmt);

      vprintf(fmt, args);

      va_end(args);
    }
  }
  #undef MSG_LEVEL

  const Time computeWeightedAverage(Time time1, long w1, Time time2, long w2) {
    double total = (double)time1 * (double)w1 + (double)time2 * (double)w2;
    double average = round(total / (double)(w1 + w2));
    return (Time) average;
  }
  
  void int8ToBigEndian(char* buf, int64_t value) {
    int k = 0;
    for (int shift = 56; shift >= 0; shift -= 8) {
      buf[k] = (value >> shift) & 0xff;
      k++;
    }
  }
}
