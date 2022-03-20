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

from ..common import FormatSpecification, FormattedBytes
from ..enums import FileType
from ..header import FileHeader

from itertools import count

class MetaDB(FileHeader, filetype = FileType.MetaDB, major=4, minor=0):
  """
    File header for the meta.db format, version 4.0.
  """
  @FileHeader.section(0)
  def general(self, src):
    return GeneralPropertiesSec(src, **self._sec_kwargs)
  @FileHeader.section(1)
  def idNames(self, src):
    return IdentifierNamesSec(src, **self._sec_kwargs)

def nullTerminated(sl):
  stop = 0
  for b in sl.view:
    if b == 0: break
    stop += 1
  else:
    raise ValueError(f"Unterminated string encountered")
  return sl[:stop]

class ntString:
  def __init__(self, fn = None):
    self._fn = fn
  def __get__(self, inst, _ = None):
    _bytes = self._fn(inst) if self._fn is not None else inst
    return nullTerminated(_bytes).view.tobytes().decode('UTF-8')
  def __set__(self, inst, v):
    v = bytes(str(v), 'UTF-8') + b'\x00'
    _bytes = self._fn(inst) if self._fn is not None else inst
    _bytes[:len(v)].view[:] = v

class GeneralPropertiesSec(FormatSpecification):
  """
    Section header for the General Properties section.
  """
  def __init__(self, *args, minor: int, **kwargs):
    super().__init__(*args, **kwargs)
    self._hdr = FormattedBytes(self._bytes, 'QQ', **self._kwargs)

  @FormattedBytes.property
  def pTitle(self): return self._hdr, 0
  @FormattedBytes.property
  def pDescription(self): return self._hdr, 1

  @ntString
  def title(self): return self._bytes[abs,self.pTitle:]
  @ntString
  def description(self): return self._bytes[abs,self.pDescription:]

  def sectionBounds(self):
    title = nullTerminated(self._bytes[abs,self.pTitle:]).range
    description = nullTerminated(self._bytes[abs,self.pDescription:]).range
    return slice(self._bytes.range.start, max(title.stop, description.stop) + 1)

class IdentifierNamesSec(FormatSpecification):
  """
    Section header for the Hierarchical Identifier Names section.
  """
  def __init__(self, *args, minor: int, **kwargs):
    super().__init__(*args, **kwargs)
    self._hdr = FormattedBytes(self._bytes, 'QB', **self._kwargs)

  @FormattedBytes.property
  def ppNames(self): return self._hdr, 0
  @FormattedBytes.property
  def nKinds(self): return self._hdr, 1

  class _PNames(FormatSpecification):
    def __init__(self, cnt, *args, **kwargs):
      super().__init__(*args, **kwargs)
      self._cnt = int(cnt)
      self._view = FormattedBytes(self._bytes, 'Q'*self._cnt, **self._kwargs)
    def __len__(self):
      return self._cnt
    def __getitem__(self, idx):
      return self._view[idx]
    def __setitem__(self, idx, value):
      self._view[idx] = value

  @property
  def pNames(self):
    return self.__class__._PNames(self.nKinds, self._bytes[abs,self.ppNames:])
  @pNames.setter
  def pNames(self, values) -> None:
    pn = self.pNames
    for i, x in enumerate(values): pn[i] = x

  class _Names:
    def __init__(self, view, pNames):
      self._bytes = view
      self._pNames = pNames
    def __len__(self):
      return len(self._pNames)
    def __getitem__(self, idx):
      return ntString().__get__(self._bytes[abs,self._pNames[idx]:])
    def __setitem__(self, idx, v):
      ntString().__set__(self._bytes[abs,self._pNames[idx]:], v)

  @property
  def names(self): return self.__class__._Names(self._bytes, self.pNames)
  @names.setter
  def names(self, values) -> None:
    ns = self.names
    for i, x in enumerate(values): ns[i] = x

