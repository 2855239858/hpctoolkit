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
from .util import ViewSlice

import pytest

def test_formattedbytes():
  # Test basic sequence-type operations
  b = bytearray(b'foobarfaaborXXX')
  fb = FormattedBytes(b, '6s6s')
  assert len(fb) == 2
  assert fb[0] == b'foobar'
  assert fb[1] == b'faabor'
  fb[0] = b'barfoo'
  assert fb[0] == b'barfoo'
  assert b[:6] == b'barfoo'
  with pytest.raises(TypeError):
    fb['foo']
  with pytest.raises(IndexError):
    fb[2]
  # Test property wrapping
  @FormattedBytes.property
  def first(fb): return fb, 0
  assert first.__get__(fb) == b'barfoo'
  first.__set__(fb, b'foofoo')
  assert fb[0] == b'foofoo'
  # Test blank value handling
  fb = FormattedBytes(b, '6s6s', blank=True)
  assert fb.isblank(1)
  with pytest.raises(ValueError, match=r'^Attempt to access blank field'):
    fb[1]
  fb[1] = b'foobar'
  assert fb.isblank(0)
  with pytest.raises(ValueError, match=r'^Attempt to access blank field'):
    fb[0]
  with pytest.raises(ValueError, match=r'^Attempt to access blank field'):
    first.__get__(fb)
  fb[0] = b'barfoo'
  assert fb[0] == b'barfoo'

def test_formatspec():
  # Test the blank variant
  fs = FormatSpecification()
  assert isinstance(fs._bytes, ViewSlice)
  assert fs._kwargs == {'blank': True}
  with pytest.raises(NotImplementedError):
    fs.validate()
  # Test the non-blank variant
  fs = FormatSpecification(bytearray(10))
  assert isinstance(fs._bytes, ViewSlice)
  assert fs._kwargs == {'blank': False}

