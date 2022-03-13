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

from .exceptions import ValidationError
from .magic import Magic

import pytest

def test_basic():
  # Ensure all the elements are where they're supposed to be
  m = Magic(b'ABCDEFGHIJxyzw\x07\x10')
  assert m.magic == b'ABCDEFGHIJ'
  assert m.format == b'xyzw'
  assert m.majorVersion == 0x07
  assert m.minorVersion == 0x10

  # Ensure the default settings from "blank" are right
  m = Magic()
  assert m.magic == b'HPCTOOLKIT'

  # Ensure changes affect the backend memory only after a flush
  b = bytearray(0x10)
  m = Magic(b)
  m.magic = b'ABCDEFGHIJ'
  m.format = b'xyzw'
  m.majorVersion = 0x07
  m.minorVersion = 0x10
  assert b == b'ABCDEFGHIJxyzw\x07\x10'

  # Ensure string transformations come out ok
  assert str(m) == "Magic(b'xyzw', v7.16)"
  assert m.version() == "v7.16"

  # Ensure local validations works properly
  with pytest.raises(ValidationError, match=r'Invalid magic.*ABCDEFGHIJ'):
    Magic(b'ABCDEFGHIJxyzw\x07\x10').validate()
  with pytest.raises(ValidationError, match=r'Invalid format.*xyzw'):
    Magic(b'HPCTOOLKITxyzw\x07\x10').validate()
  m = Magic(b'HPCTOOLKITmeta\x07\x10')
  assert m.validate() is m
