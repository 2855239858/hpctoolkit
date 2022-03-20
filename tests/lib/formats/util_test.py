# * BeginRiceCopyright *****************************************************
#
# $HeadURL$
# $Id$
#
# --------------------------------------------------------------------------
# Part of HPCToolkit (hpctoolkit.org)
#
# Information about sources of support for research and development of
# HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
# --------------------------------------------------------------------------
#
# Copyright ((c)) 2022-2022, Rice University
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of Rice University (RICE) nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# This software is provided by RICE and contributors "as is" and any
# express or implied warranties, including, but not limited to, the
# implied warranties of merchantability and fitness for a particular
# purpose are disclaimed. In no event shall RICE or contributors be
# liable for any direct, indirect, incidental, special, exemplary, or
# consequential damages (including, but not limited to, procurement of
# substitute goods or services; loss of use, data, or profits; or
# business interruption) however caused and on any theory of liability,
# whether in contract, strict liability, or tort (including negligence
# or otherwise) arising in any way out of the use of this software, even
# if advised of the possibility of such damage.
#
# ******************************************************* EndRiceCopyright *

from .util import viewof, ViewSlice, MemorySlice, InfiniteMemorySlice, MappedFileSlice

import io
import re
import pytest
import sys
from tempfile import TemporaryFile

def test_viewof():
  # External memory-based views
  assert type(viewof(b'foobar')) is MemorySlice
  assert type(viewof(bytearray(b'foobar'))) is MemorySlice
  assert type(viewof(memoryview(b'foobar'))) is MemorySlice
  # Internally memory-backed views
  assert type(viewof()) is InfiniteMemorySlice
  assert type(viewof(None)) is InfiniteMemorySlice
  # Mapped file-backed views
  with TemporaryFile() as f:
    f.write(b'foobar')
    assert type(viewof(f)) is MappedFileSlice
  # Passthrough
  v = viewof(b'foobar')
  assert viewof(v) is v
  # Invalid objects
  with pytest.raises(TypeError):
    viewof(42)
  with pytest.raises(ValueError):
    viewof(io.BytesIO())

def test_base():
  # Check that the base class cannot be instantated directly
  with pytest.raises(RuntimeError):
    ViewSlice()
  # Check basic operations
  v = MemorySlice(b'foobar')
  assert re.search('start=0, stop=6', repr(v))
  assert len(v) == 6
  # Check slicing operations
  with pytest.raises(TypeError):
    v['hello']
  with pytest.raises(ValueError):
    v[1:2:3]
  assert v[1:3].range == slice(1, 3)
  assert v[1:].range == slice(1, 6)
  assert v[:-3].range == slice(0, 3)
  assert v[1,2].range == v[1:3].range
  with pytest.raises(ValueError):
    v[10:]
  with pytest.raises(ValueError):
    v[2:10]
  # Check subslicing operations
  assert v[1:][1:].range == slice(2, 6)
  assert v[1:4][1:].range == slice(2, 4)
  with pytest.raises(ValueError):
    v[2:4][abs,1:]
  with pytest.raises(ValueError):
    v[2:4][abs,:1]
  with pytest.raises(ValueError):
    v[2:4][abs,2:5]
  with pytest.raises(ValueError):
    v[2:4][abs,1,3]
  assert v[2:4][abs,2:].range == slice(2, 4)
  assert v[2:4][abs,2:3].range == slice(2, 3)
  assert v[2:4][abs,2,1].range == slice(2, 3)
  with pytest.raises(ValueError):
    v[1,2,3,4]
  with pytest.raises(ValueError):
    v[1,2,3]
  # Check widening operations
  assert v[1:3].widened.range == v.range

def roundtrip(v, fetch, reset = None):
  if reset is not None:
    reset()
  try:
    fetch()[:6] = b'barfoo'
  except Exception:
    pass
  v.view[:6] = b'foobar'
  assert fetch()[:6] == b'foobar'

def test_memory():
  with pytest.raises(TypeError):
    MemorySlice(42)
  # Check simple memory arrangements
  b = bytearray(6)
  v = MemorySlice(b)
  roundtrip(v, lambda: b)
  # Check offset memory
  b = bytearray(6)
  v = MemorySlice(b, offset=10)
  assert v.range == slice(0, 16)
  with pytest.raises(IndexError):
    v.view
  roundtrip(v[10:], lambda: b)
  # Check double-sliced memory
  b = bytearray(6)
  v = MemorySlice(b)
  v2 = MemorySlice(v)
  assert v is not v2
  roundtrip(v, lambda: v2.view)
  roundtrip(v2, lambda: v.view)

def test_infinite():
  with pytest.raises(TypeError):
    InfiniteMemorySlice(42)
  # Check infinite slicing properties
  with pytest.raises(IndexError):
    InfiniteMemorySlice(start=-4)
  with pytest.raises(IndexError):
    InfiniteMemorySlice(stop=-4)
  assert InfiniteMemorySlice(stop=4).range.stop == 4
  # Check basic operations
  v = InfiniteMemorySlice()
  assert len(v) == sys.maxsize
  assert len(v[:4]) == 4
  # Check slicing operations
  assert v[4:].range == slice(4, None)
  assert v[4:10].range == slice(4, 10)
  with pytest.raises(IndexError):
    v.view
  # Check roundtripping and double-slicing
  v2 = InfiniteMemorySlice(v)[:6]
  roundtrip(v[:6], lambda: v2.view)

def test_mapped():
  with pytest.raises(TypeError):
    MappedFileSlice(42)
  with pytest.raises(TypeError):
    MappedFileSlice(io.StringIO())
  with TemporaryFile() as f:
    v = MappedFileSlice(f)
    with pytest.raises(IndexError):
      v.view
    v = v[:6]
    # Check simple roundtripping
    def read():
      f.seek(0)
      return f.read()
    def reset():
      f.seek(0)
      f.write(b'barfoo')
    roundtrip(v, read, reset)
    # Check double-referenced files
    v2 = MappedFileSlice(v)[:6]
    roundtrip(v, lambda: v2.view)
    roundtrip(v2, lambda: v.view)
