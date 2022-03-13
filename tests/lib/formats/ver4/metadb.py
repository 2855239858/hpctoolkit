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

from ..common import FormatSpecification
from ..enums import FileType
from ..exceptions import ValidationError, FutureVersionWarning
from ..header import Magic

from typing import Optional, Any
from warnings import warn

class Header(FormatSpecification):
  """
    File header for the meta.db format, version 4.0.
  """
  __slots__ = ['magic']
  def __init__(self, *args, minor: Optional[int] = None, **kwargs) -> None:
    super().__init__(*args, **kwargs)
    self.magic = Magic(self._bytes, **self._kwargs)
    if self._blank:
      self.magic.format = FileType.MetaDB.value
      self.magic.majorVersion = 4
      self.magic.minorVersion = minor if minor is not None else 0
    else:
      if self.magic.minorVersion > 0:
        warn(FutureVersionWarning(self.magic.majorVersion, self.magic.minorVersion, 0))

  def validate(self):
    self.magic.validate()
    if FileType(self.magic.format) is not FileType.MetaDB:
      raise ValidationError(f"Invalid format identifier for MetaDB: expected {FileType.MetaDB.value!r}, got {self.magic.format!r}")
    return self

  def __str__(self):
    return f"MetaDB({self.magic.version()}, ...)"
