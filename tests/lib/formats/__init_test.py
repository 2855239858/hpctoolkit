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

from . import open as hpcopen
from . import _majorVersions
from .exceptions import MajorVersionError, FormatError
from .enums import FileType

from . import ver4

import pytest

def test_errors():
  with pytest.raises(ValueError, match=r'format must be specified'):
    hpcopen(None, blank=True)
  with pytest.raises(MajorVersionError, match=r'^Mismatched major version'):
    hpcopen(b'HPCTOOLKITmeta\x01\x00', major=2)
  with pytest.raises(FormatError, match=r'^Mismatched file format'):
    hpcopen(b'HPCTOOLKITprof\x01\x00', format=b'meta')
  with pytest.raises(MajorVersionError, match=r'^Unsupported major version'):
    hpcopen(b'HPCTOOLKITmeta\x00\x00')
  # Hackily push a new major version 255 in for this test
  _majorVersions[255] = object()
  with pytest.raises(FormatError, match=r'^Unsupported file format'):
    hpcopen(b'HPCTOOLKITmeta\xFF\x00')

def test_creation():
  suffix = b'\x00' * 100
  assert type(hpcopen(None, blank=True, major=4, format=FileType.MetaDB)) == ver4.MetaDB
  assert type(hpcopen(None, blank=True, format=FileType.MetaDB)) == ver4.MetaDB
  assert type(hpcopen(b'HPCTOOLKITmeta\x04\x00', major=4)) == ver4.MetaDB
  assert type(hpcopen(b'HPCTOOLKITmeta\x04\x00', format=FileType.MetaDB)) == ver4.MetaDB

def test_autodetection():
  suffix = b'\x00' * 100
  assert type(hpcopen(b'HPCTOOLKITmeta\x04\x00'+suffix)) is ver4.MetaDB
