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

import pytest
import warnings

from .metadb import MetaDB

def test_metadb():
  m = MetaDB(bytearray(
    b'HPCTOOLKITmeta\x04\x00'                              # 0x000
    + bytes.fromhex('3000000000000000 0000000000000000')   # 0x010
    + bytes.fromhex('3000000000000000 0000000000000000')   # 0x010
  ))
  assert type(m.general) is GeneralPropertiesSec
  assert type(m.idNames) is IdentifierNamesSec

from .metadb import GeneralPropertiesSec

def test_generalv0():
  b = bytearray(bytes.fromhex('1300000000000000 2000000000000000')
                + b'\xFF'*3 + b'Title\x00' + b'\xFF'*7 + b'Description\x00')
  gps = GeneralPropertiesSec(b, minor = 0)
  assert gps.pTitle == 0x13
  assert gps.pDescription == 0x20
  assert gps.title == 'Title'
  assert gps.description == 'Description'

  gps.title = 'NewTitle'
  assert b[0x13:].startswith(b'NewTitle\x00')
  assert gps.title == 'NewTitle'
  gps.description = 'desCription'
  assert b[0x20:].startswith(b'desCription\x00')
  assert gps.description == 'desCription'

  with pytest.raises(ValueError, match=r'^Unterminated string'):
    GeneralPropertiesSec(bytes.fromhex('1000000000000000 2000000000000000')
                         + b'Unterminated', minor=0).title
  with pytest.raises(ValueError, match=r'^Unterminated string'):
    GeneralPropertiesSec(bytes.fromhex('1000000000000000 1200000000000000')
                         + b'T\x00Unterminated', minor=0).description

  assert gps.sectionBounds() == slice(0, len(b))

from .metadb import IdentifierNamesSec

def test_idnamesv0():
  b = bytearray(bytes.fromhex('1000000000000000 02 FFFFFFFFFFFFFF 3000000000000000 3400000000000000')
                + b'\xFF'*16 + b'foo\x00bar\x00')
  hins = IdentifierNamesSec(b, minor=0)
  assert hins.ppNames == 0x10
  assert hins.nKinds == 2
  assert list(hins.pNames) == [0x30, 0x34]
  assert list(hins.names) == ['foo', 'bar']

  hins.pNames = [0x32, 0x35]
  assert list(hins.names) == ['o', 'ar']

  hins.pNames = [0x30, 0x34]
  hins.names = ['ba', 'fo']
  assert list(hins.names) == ['ba', 'fo']

