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

from .util import viewof, ViewSlice

from typing import Any, Optional, Union
from collections.abc import Callable
from struct import Struct

class FormattedBytes:
  """
    View on a series of bytes with some proper interpretation, through the
    standard struct classes.
  """
  __slots__ = ['_struct', '_bytes', '_blank']
  _struct: Struct
  _bytes: ViewSlice
  _blank: Union[bool, list[bool]]

  def __init__(self, viewsl: Any, form: str, *, blank: bool = False) -> None:
    """
      Create a new interpretation for the given series of bytes.
      Also supports views on file objects.
    """
    self._struct = Struct(form)
    self._bytes = viewof(viewsl)[:self._struct.size]
    assert len(self._bytes) == self._struct.size
    self._blank = blank

  def __len__(self) -> int:
    return len(self._struct.unpack(self._bytes.view))

  def __getitem__(self, idx: int) -> Any:
    if not isinstance(idx, int):
      raise TypeError
    if isinstance(self._blank, bool):
      if self._blank:
        raise ValueError(f"Attempt to access blank field: {idx}")
    elif self._blank[idx]:
      raise ValueError(f"Attempt to access blank field: {idx}")
    return self._struct.unpack(self._bytes.view)[idx]

  def __setitem__(self, idx: int, val: Any) -> None:
    values = list(self._struct.unpack(self._bytes.view))
    values[idx] = val
    self._struct.pack_into(self._bytes.view, 0, *values)
    if self._blank:
      if not isinstance(self._blank, list):
        self._blank = [True] * len(values)
      self._blank[idx] = False

  def isblank(self, idx: int) -> bool:
    if isinstance(self._blank, bool):
      return self._blank
    return self._blank[idx]

  @staticmethod
  def property(fn):
    """
      Shorthand to expose the fields of a FormattedBytes as a property.
    """
    def getx(self) -> Any:
      fb, idx = fn(self)
      try:
        return fb[idx]
      except ValueError as e:
        raise ValueError(f"Attempt to access blank field: {fn.__name__}") from e
    def setx(self, value: Any) -> None:
      fb, idx = fn(self)
      fb[idx] = value
    return property(fget=getx, fset=setx, doc=fn.__doc__)


class FormatSpecification:
  """
    Base class for all format specifications written in Python.
  """
  __slots__ = ['_bytes', '_blank']
  _bytes: ViewSlice
  _blank: bool

  def __init__(self, src: Optional[Any] = None, *,
               blank: Optional[bool] = None) -> None:
    self._bytes = viewof(src)
    self._blank = blank if blank is not None else src is None

  @property
  def _kwargs(self):
    return {'blank': self._blank}

  def validate(self) -> Any:
    raise NotImplementedError
