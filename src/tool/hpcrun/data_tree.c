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


/******************************************************************************
 * local include files
 *****************************************************************************/

#include <messages/messages.h>
#include <lib/prof-lean/spinlock.h>
#include <lib/prof-lean/splay-macros.h>

#include <include/queue.h> // slist

#include <memory/hpcrun-malloc.h>

#include "data_tree.h"

/******************************************************************************
 * macros
 *****************************************************************************/

#define DATATREE_DEBUG 1


/******************************************************************************
 * private data
 *****************************************************************************/

static spinlock_t datatree_lock = SPINLOCK_UNLOCKED;

static struct datatree_info_s  *datacentric_tree_root = NULL;



/******************************************************************************
 * PRIVATE splay operations
 *****************************************************************************/


static struct datatree_info_s *
splay(struct datatree_info_s *root, void *key)
{
  REGULAR_SPLAY_TREE(datatree_info_s, root, key, memblock, left, right);
  return root;
}

static struct datatree_info_s **
interval_splay(struct datatree_info_s **root, void *key, void **start, void **end)
{
  INTERVAL_SPLAY_TREE(datatree_info_s, *root, key, memblock, rmemblock, left, right);
  return root;
}

#if DATATREE_DEBUG
// for debugging purpose
static int
splay_tree_count_size(struct datatree_info_s *current)
{
  int result = 0;

  if (!current) return 0;
  result += splay_tree_count_size(current->left);
  result += splay_tree_count_size(current->right);

  return result + 1;
}


static int
splay_tree_size()
{
  return splay_tree_count_size(datacentric_tree_root);
}
#endif

/******************************************************************************
 * PUBLIC splay operations
 *****************************************************************************/

struct datatree_info_s*
datatree_info_insert_ext(struct datatree_info_s **data_root,
                         spinlock_t *data_lock,
                         struct datatree_info_s *node)
{
  void *memblock = node->memblock;

  node->left = node->right = NULL;

  spinlock_lock(data_lock);

  if (*data_root != NULL) {

    *data_root = splay(*data_root, memblock);
    struct datatree_info_s *root = *data_root;

    if (memblock < (*data_root)->memblock) {
      node->left  = root->left;
      node->right = root;
      root->left = NULL;

    } else if (memblock > root->memblock) {
      node->left  = root;
      node->right = root->right;
      root->right = NULL;
    }
  }
  *data_root = node;

#if DATATREE_DEBUG
  TMSG(DATACENTRIC, "[%x] %d items  addr %x (%d bytes)",
      node->magic, splay_tree_size(), node->memblock, node->bytes);
#endif

  spinlock_unlock(data_lock);
  return *data_root;
}


/* interface for data-centric analysis */
struct datatree_info_s *
datatree_info_lookup_ext( struct datatree_info_s **data_root,
                      spinlock_t *lock,
                      void *key, void **start, void **end)
{
  if(!data_root || !*data_root || !key) {
    return NULL;
  }

  spinlock_lock(lock);

  struct datatree_info_s **root = NULL;
  root = interval_splay(data_root, key, start, end);
  if (!root) {
    spinlock_unlock(lock);
    return NULL;
  }

  struct datatree_info_s *info = *root;

#if DATATREE_DEBUG
  TMSG(DATACENTRIC, "lookup key: %p  %result: %p  ", key, info);
#endif

  if(info && (info->memblock <= key) && (info->rmemblock > key)) {
    *start = info->memblock;
    *end   = info->rmemblock;

    spinlock_unlock(lock);
    return info;
  }

  spinlock_unlock(lock);

  return NULL;
}

/*
 * Insert a node
 */ 
void
datatree_info_insert(struct datatree_info_s *node)
{
  datatree_info_insert_ext(&datacentric_tree_root, &datatree_lock, node);
}

/*
 * remove a node containing a memory block
 */ 
struct datatree_info_s *
datatree_info_delete(void *memblock)
{
  struct datatree_info_s *result = NULL;

  if (datacentric_tree_root == NULL) {
    return NULL;
  }

  spinlock_lock(&datatree_lock);

  datacentric_tree_root = splay(datacentric_tree_root, memblock);

  if (!datacentric_tree_root ||
      memblock != datacentric_tree_root->memblock) {
    spinlock_unlock(&datatree_lock);
    return NULL;
  }

  result = datacentric_tree_root;


  if (datacentric_tree_root->left == NULL) {
    datacentric_tree_root = datacentric_tree_root->right;
    spinlock_unlock(&datatree_lock);
    return result;
  }

  datacentric_tree_root->left = splay(datacentric_tree_root->left, memblock);
  datacentric_tree_root->left->right = datacentric_tree_root->right;
  datacentric_tree_root =  datacentric_tree_root->left;

  spinlock_unlock(&datatree_lock);
  return result;
}


/* interface for data-centric analysis */
struct datatree_info_s *
datatree_info_lookup(void *key, void **start, void **end)
{
  return datatree_info_lookup_ext(&datacentric_tree_root, &datatree_lock, key, start, end);
}
