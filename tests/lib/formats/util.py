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

import io
from itertools import repeat
import mmap
import sys
from typing import Any, Optional, Union

def viewof(src: Optional[Any] = None, *args, **kwargs):
    """
      Convert the given object into a ViewSlice of the appropriate subclass.
    """
    # Pass instances through without changes
    if isinstance(src, ViewSlice):
      return src

    # If src is none (which is possible), use a growable backing memory region.
    if src is None:
      return InfiniteMemorySlice(*args, **kwargs)

    # If src supports the buffer protocol, use a MemorySlice
    v = None
    try:
      v = memoryview(src)
    except TypeError:
      pass
    else:
      return MemorySlice(v, *args, **kwargs)

    # If that failed, it must be a binary file object.
    # The constructor will handle the errors from here.
    return MappedFileSlice(src, *args, **kwargs)

class ViewSlice:
  """
    Efficient access to part of an arbitrary array of bytes somewhere out there.
    Unlike a memoryview, also allows creating "sibling" slices in other parts
    of the original data blob.
  """

  __slots__ = ['_slice']
  _slice: slice

  def __new__(cls, *args, **kw):
    if cls is ViewSlice:
      raise RuntimeError("Base class, instantiate a subclass!")
    return super().__new__(cls)

  def __init__(self, *, start: int = 0, stop: Optional[int] = None,
               _size: Optional[int] = 0) -> None:
    """
      Initialize a generic ViewSlice.
    """
    if _size is not None:
      # Use the consistent slice interpretation for finite-sized bits
      start, stop, _ = slice(start, stop, None).indices(_size)
    else:
      # Ensure negative values are not used
      if start < 0:
        raise IndexError(f"Relative-to-end start index cannot be used in infinite Slices: {start:d}")
      if stop is not None:
        if stop < 0:
          raise IndexError(f"Relative-to-end stop index cannot be used in infinite Slices: {stop:d}")
        stop = max(stop, start)
    self._slice = slice(start, stop, None)

  def __repr__(self) -> str:
    return f"{self.__class__.__name__}(..., start={self._slice.start:d}, stop={self._slice.stop})"

  def __len__(self) -> int:
    if self._slice.stop is None:
      return sys.maxsize
    return int(self._slice.stop - self._slice.start)

  def __getitem__(self, sl):
    if isinstance(sl, tuple):
      if len(sl) == 2:
        if sl[0] is abs:
          sl = sl[1]
          if sl.start is not None and sl.start < self._slice.start:
            raise ValueError(f"Request for data before start of ViewSlice: {sl.start} < {self._slice.start}")
          if sl.stop is not None and sl.stop < self._slice.start:
            raise ValueError(f"Request for data before start of ViewSlice: {sl.stop} < {self._slice.start}")
          start = max(sl.start - self._slice.start, 0)
          stop = None if sl.stop is None else max(sl.stop - self._slice.start, 0)
          sl = slice(start, stop, None)
        else:
          start, length = sl
          sl = slice(start, start + length, None)
      elif len(sl) == 3:
        if sl[0] is not abs:
          raise ValueError(f"Invalid index modifier: {sl[0]}")
        _, start, length = sl
        if start < self._slice.start:
          raise ValueError(f"Request for data before start of ViewSlice: {start} < {self._slice.start}")
        stop = max(start + length - self._slice.start, 0)
        start = max(start - self._slice.start, 0)
        sl = slice(start, stop, None)
      else:
        raise ValueError(f"Invalid size for tuple index: {len(sl)} is not <= 3")
    if not isinstance(sl, slice):
      raise TypeError(f"Invalid index for a ViewSlice: {type(sl)!r}")
    if sl.step is not None:
      raise ValueError(f"Invalid step for ViewSlice slicing: {sl.step!r}")
    start, stop = None, None
    if self._slice.stop is None:
      if sl.stop is None:
        start, stop = sl.indices(sys.maxsize)[0], None
      else:
        start, stop, _ = sl.indices(sl.stop)
        stop += self._slice.start
    else:
      start, stop, _ = sl.indices(self._slice.stop - self._slice.start)
      if sl.stop is not None and stop < sl.stop:
        raise ValueError(f"Request for data off end of ViewSlice: {stop} < {sl.stop}")
      stop += self._slice.start
    if sl.start is not None and start < sl.start:
      raise ValueError(f"Request for data off end of ViewSlice: {start} < {sl.start}")
    start += self._slice.start
    return self.__class__(self, start=start, stop=stop)

  @property
  def range(self):
    """
      Get the slice of data visible in this slice.
    """
    return self._slice

  @property
  def widened(self):
    """
      Return a copy of this slice, but re-ranged to view the entire (available)
      backed memory region.
    """
    return self.__class__(self, start=0, stop=None)

  @property
  def view(self) -> memoryview:
    """
      Get a buffer-protocol-compatible reference to the byte array referred to
      by this slice.
    """
    raise NotImplementedError  # pragma: no cover


class MemorySlice(ViewSlice):
  """
    Slice accessing part of a larger memory region. The backing memory region
    may be offset from the absolute origin of "whole region."
  """

  __slots__ = ['_view', '_offset']
  _view: memoryview
  _offset: int

  def __init__(self, view: Any, *, offset: int = 0, **kwargs) -> None:
    """
      Construct a slice from a fragment of a larger memory region.
    """
    if isinstance(view, MemorySlice):
      offset = view._offset
      view = view._view
    super().__init__(_size=offset + len(view), **kwargs)
    self._view = memoryview(view)
    self._offset = int(offset)

  @property
  def view(self) -> memoryview:
    if self._slice.start < self._offset:
      raise IndexError(f"MemoryFragmentSlice refers to region before the origin: {self._slice.start:d} < {self._offset:d}")
    start, stop = self._slice.start - self._offset, self._slice.stop - self._offset
    return self._view[start:stop]


class InfiniteMemorySlice(ViewSlice):
  """
    Slice accessing a growable ("infinite") memory region.
  """

  __slots__ = ['_arr']
  _arr: bytearray

  def __init__(self, cpfrom = None, **kwargs) -> None:
    super().__init__(_size=None, **kwargs)
    if cpfrom is not None:
      if not isinstance(cpfrom, InfiniteMemorySlice):
        raise TypeError
      self._arr = cpfrom._arr
    else:
      self._arr = bytearray()

  @property
  def view(self) -> memoryview:
    if self._slice.stop is None:
      raise IndexError(f"InfiniteMemorySlice needs a concrete upper bound before access!")
    if len(self._arr) < self._slice.stop:
      self._arr.extend(repeat(0, self._slice.stop - len(self._arr)))
    return memoryview(self._arr)[self._slice]


class MappedFileSlice(ViewSlice):
  """
    Slice efficiently accessing the bits of a file through mmapped memory.
  """

  __slots__ = ['_file', '_mapping']
  _file: io.IOBase
  _mapping: mmap.mmap

  def __init__(self, file, **kwargs) -> None:
    if isinstance(file, MappedFileSlice):
      file = file._file
    if not isinstance(file, io.IOBase):
      raise TypeError(f"Invalid file for MappedFileSlice: {file!r}")
    if isinstance(file, io.TextIOBase):
      raise TypeError(f"Text I/O is not supported for MappedFileSlice: {file!r}")
    try:
      fileno = file.fileno()
    except io.UnsupportedOperation:
      raise ValueError(f"File must be mappable: {file!r}") from None
    super().__init__(_size=None, **kwargs)
    print(f"{self._slice=!r}")
    self._file = file
    self._mapping = None
    if self._slice.stop is not None:
      pos = file.tell()
      file.seek(0, io.SEEK_END)
      if file.tell()+1 < self._slice.stop:
        file.write(b'\x00' * (self._slice.stop - file.tell()))
      file.seek(pos)
      self._mapping = mmap.mmap(fileno, self._slice.stop)

  @property
  def view(self) -> memoryview:
    if self._mapping is None:
      raise IndexError(f"MappedFileSlice needs a concrete upper bound before access!")
    return memoryview(self._mapping)[self._slice.start:]
