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

from .common import FormattedBytes, FormatSpecification
from .enums import FileType
from .exceptions import ValidationError

from typing import Any, Optional

class Magic(FormatSpecification):
  """
    Format conversion for the standard magic present at the beginning of every
    HPCToolkit data file.
  """
  __slots__ = ['_view']
  def __init__(self, *args, **kwargs) -> None:
    super().__init__(*args, **kwargs)
    self._view = FormattedBytes(self._bytes, '10s4sBB', **self._kwargs)
    if self._blank:
      self.magic = b'HPCTOOLKIT'

  @FormattedBytes.property
  def magic(self): return self._view, 0
  @FormattedBytes.property
  def format(self): return self._view, 1
  @FormattedBytes.property
  def majorVersion(self): return self._view, 2
  @FormattedBytes.property
  def minorVersion(self): return self._view, 3

  def validate(self):
    if self.magic != b'HPCTOOLKIT':
      raise ValidationError(f"Invalid magic: expected b'HPCTOOLKIT', got {self.magic!r}")
    form = self.format
    try:
      FileType(form)
    except ValueError:
      raise ValidationError(f"Invalid format: got {form!r}") from None
    return self

  def __str__(self):
    return f"Magic({self.format}, v{self.majorVersion}.{self.minorVersion})"
  def version(self):
    return f"v{self.majorVersion}.{self.minorVersion}"
