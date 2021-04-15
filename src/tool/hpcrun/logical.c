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
// Copyright ((c)) 2002-2021, Rice University
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

#include "logical.h"

#include "cct_backtrace_finalize.h"
#include "hpcrun-malloc.h"
#include "messages/messages.h"
#include "thread_data.h"

static void logicalize_bt(backtrace_info_t*, int);

// ---------------------------------------
// Logical stack mutation functions
// ---------------------------------------

#define REGIONS_PER_SEGMENT \
  (sizeof( ((struct logical_region_segment_t*)0)->regions ) \
   / sizeof( ((struct logical_region_segment_t*)0)->regions[0] ))

void hpcrun_logical_stack_init(logical_region_stack_t* s) {
  s->depth = 0;
  s->head = NULL;
  s->spare = NULL;
}

logical_region_t* hpcrun_logical_stack_top(logical_region_stack_t* s) {
  if(s->depth == 0) return NULL;
  return &s->head->regions[(s->depth-1) % REGIONS_PER_SEGMENT];
}

logical_region_t* hpcrun_logical_stack_push(logical_region_stack_t* s,
                                            const logical_region_t* r) {
  // Figure out where the next element goes, allocate some space if we must
  if(s->depth % REGIONS_PER_SEGMENT == 0) {
    if(s->spare != NULL) {
      // Pop from spare for the next segment
      struct logical_region_segment_t* newhead = s->spare;
      s->spare = newhead->prev;
      newhead->prev = s->head;
      s->head = newhead;
    } else {
      // Allocate a new entry for the next segment
      struct logical_region_segment_t* newhead = hpcrun_malloc(sizeof *newhead);
      newhead->prev = s->head;
      s->head = newhead;
    }
  }
  logical_region_t* next = &s->head->regions[s->depth % REGIONS_PER_SEGMENT];

  // Copy over the region data, and return the new top
  memcpy(next, r, sizeof *next);
  s->depth++;
  ETMSG(LOGICAL_CTX, "Pushed region [%d] %p, enter = %p", s->depth, next, next->enter);
  return next;
}

size_t hpcrun_logical_stack_settop(logical_region_stack_t* s, size_t n) {
  if(n >= s->depth) return n - s->depth;
  // Pop off segments until we've got the right number
  size_t n_segs = n / REGIONS_PER_SEGMENT;
  if(n % REGIONS_PER_SEGMENT > 0) n_segs++; // Partially-filled segment
  size_t s_segs = s->depth / REGIONS_PER_SEGMENT;
  if(s->depth % REGIONS_PER_SEGMENT > 0) s_segs++;  // Partially-filled segment
  for(; s_segs > n_segs; s_segs--) {
    struct logical_region_segment_t* newhead = s->head->prev;
    s->head->prev = s->spare;
    s->spare = s->head;
    s->head = newhead;
  }
  // We're within the right segment, so we can just set the depth now.
  s->depth = n;
  ETMSG(LOGICAL_CTX, "Settop to [%d]", s->depth);
  return 0;
}

// ---------------------------------------
// Backtrace modification engine
// ---------------------------------------

static bool logicalize_bt_registered = false;
static cct_backtrace_finalize_entry_t logicalize_bt_entry = {logicalize_bt};
void hpcrun_logical_register() {
  if(!logicalize_bt_registered) {
    cct_backtrace_finalize_register(&logicalize_bt_entry);
    logicalize_bt_registered = true;
  }
}

static void logicalize_bt(backtrace_info_t* bt, int isSync) {
  thread_data_t* td = hpcrun_get_thread_data();
  if(td->logical.depth == 0) return;  // No need for our services

  ETMSG(LOGICAL_CTX, "========= Logicalizing backtrace =========");

  frame_t* bt_cur = bt->begin;

  struct logical_region_segment_t* seg;
  size_t off = td->logical.depth % REGIONS_PER_SEGMENT;
  bool first = true;
  for(seg = td->logical.head; seg != NULL; seg = seg->prev) {
    for(; off > 0; off--) {
      logical_region_t* cur = &seg->regions[off-1];

      // Progress the cursor until the first frame_t within this logical segment
      if(cur->exit != NULL) {
        while(bt_cur->cursor.sp != cur->exit) {
          ETMSG(LOGICAL_CTX, " sp = %p ip = %s +0x%x", bt_cur->cursor.sp,
                hpcrun_loadmap_findById(bt_cur->ip_norm.lm_id)->name, bt_cur->ip_norm.lm_ip);
          if(bt_cur == bt->last) goto earlyexit;
          bt_cur++;
        }
        ETMSG(LOGICAL_CTX, "== Exit from logical range @ %p ==", cur->exit);
      } else {
        assert(first && "Only the topmost logical region can have exit = NULL!");
        ETMSG(LOGICAL_CTX, "== Within logical range ==");
      }
      first = false;
      frame_t* logical_start = bt_cur;

      // Scan through until we've seen everything including the enter
      while(bt_cur->cursor.sp != cur->enter) {
        ETMSG(LOGICAL_CTX, " sp = %p ip = %s +0x%x", bt_cur->cursor.sp,
              hpcrun_loadmap_findById(bt_cur->ip_norm.lm_id)->name, bt_cur->ip_norm.lm_ip);
        if(bt_cur == bt->last) goto earlyexit;
        bt_cur++;
      }
      ETMSG(LOGICAL_CTX, " sp = %p ip = %s +0x%x", bt_cur->cursor.sp,
            hpcrun_loadmap_findById(bt_cur->ip_norm.lm_id)->name, bt_cur->ip_norm.lm_ip);
      if(bt_cur == bt->last) goto earlyexit;
      bt_cur++;
      frame_t* logical_end = bt_cur;

      ETMSG(LOGICAL_CTX, "== Logically the above is replaced by the following ==");
      assert(cur->expected > 0 && "Logical regions should always have at least 1 logical frame!");

      // If needed, move later physical frames down to make room for the logical ones
      if(logical_start + cur->expected > logical_end) {
        size_t newused = bt->last - bt->begin + (logical_start + cur->expected - logical_end);
        ptrdiff_t bt_begin_diff = bt->begin - td->btbuf_beg;
        ptrdiff_t bt_last_diff = bt->last - td->btbuf_beg;
        ptrdiff_t bt_cur_diff = bt_cur - td->btbuf_beg;
        ptrdiff_t logical_start_diff = logical_start - td->btbuf_beg;
        ptrdiff_t logical_end_diff = logical_end - td->btbuf_beg;
        while(td->btbuf_end - td->btbuf_beg < newused) hpcrun_expand_btbuf();
        logical_end = td->btbuf_beg + logical_end_diff;
        logical_start = td->btbuf_beg + logical_start_diff;
        bt_cur = td->btbuf_beg + bt_cur_diff;
        bt->last = td->btbuf_beg + bt_last_diff;
        bt->begin = td->btbuf_beg + bt_begin_diff;

        memmove(logical_start + cur->expected, logical_end, (bt->last - logical_end + 1)*sizeof(frame_t));
        bt->last = logical_start + cur->expected + (bt->last - logical_end);
        logical_end = logical_start + cur->expected;
      }

      // Overwrite the physical frames with logical ones
      void* store = NULL;
      size_t index = 0;
      while(cur->generator(cur->generator_arg, &store, index, logical_start + index)) {
        ETMSG(LOGICAL_CTX, "(logical) ip = %d +0x%x", (logical_start+index)->ip_norm.lm_id, (logical_start+index)->ip_norm.lm_ip);
        assert(index < cur->expected && "Expected number of logical frames is too low!");
        index++;
      }
      ETMSG(LOGICAL_CTX, "(logical) ip = %d +0x%x", (logical_start+index)->ip_norm.lm_id, (logical_start+index)->ip_norm.lm_ip);
      ETMSG(LOGICAL_CTX, "== Entry to logical range @ %p ==", cur->enter);

      // Shift the physical frames down to fill the gap
      bt_cur = logical_start + index + 1;
      memmove(bt_cur, logical_end, (bt->last - logical_end + 1)*sizeof(frame_t));
      bt->last = bt_cur + (bt->last - logical_end);
    }
    off = REGIONS_PER_SEGMENT;
  }

earlyexit:
  if(seg != NULL) {
    ETMSG(LOGICAL_CTX, "WARNING: The following logical regions did not match:");
    for(; seg != NULL; seg = seg->prev) {
      for(; off > 0; off--) {
        logical_region_t* cur = &seg->regions[off-1];
        ETMSG(LOGICAL_CTX, " sp @ exit = 0x%x, sp @ enter = 0x%x",
              cur->exit, cur->enter);
      }
    }
  }

  for(; bt_cur != bt->last; bt_cur++)
    ETMSG(LOGICAL_CTX, " sp = %p ip = %s +0x%x", bt_cur->cursor.sp,
          hpcrun_loadmap_findById(bt_cur->ip_norm.lm_id)->name, bt_cur->ip_norm.lm_ip);

  ETMSG(LOGICAL_CTX, "========= END Logicalizing backtrace =========");
}
