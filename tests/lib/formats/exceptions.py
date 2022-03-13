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

from .enums import FileType

from typing import Optional

class ValidationError(Exception):
  """
    Exception raised when there are errors in the interpereted data.
  """

class MajorVersionError(RuntimeError):
  """
    Exception raised when the major versions conflict between what's supported,
    available, and requested.
  """
  def __init__(self, expected: int, got: Optional[int] = None) -> None:
    if got is None:
      super().__init__(f"Unsupported major version: v{expected:d}")
    else:
      super().__init__(f"Mismatched major version: expected v{expected:d}, got v{got:d}")

class FormatError(RuntimeError):
  """
    Exception raised when the exact file format conflicts between what's
    supported, available, and requested.
  """
  def __init__(self, expected: FileType, got: Optional[FileType] = None,
               major: Optional[int] = None) -> None:
    postfix = "" if major is None else f" for v{major:d}"
    if got is None:
      super().__init__(f"Unsupported file format{postfix}: {FileType(expected).name}")
    else:
      super().__init__(f"Mismatched file format{postfix}: expected {FileType(expected).name}, got {FileType(got).name}")

class FutureVersionWarning(UserWarning):
  """
    Warning thrown when the requested or supported version is lower than the
    version of the file itself.

    I.e: limited forward compatibility is in effect.
  """
  def __init__(self, major: int, minor: int, maxMinor: int) -> None:
    super().__init__(f"Parsing forward-compatible version v{major:d}.{minor:d} as v{major:d}.{maxMinor:d}")
