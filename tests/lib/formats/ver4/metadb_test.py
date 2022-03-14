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

from .metadb import MetaDB, GeneralPropertiesSec

import pytest
import warnings

def test_metadb():
  m = MetaDB(bytearray(
    b'HPCTOOLKITmeta\x04\x00'                              # 0x000
    + bytes.fromhex('0000000000000000 0000000000000000')   # 0x010
    + b'PADDING_PADDING_'                                  # 0x020
    + bytes.fromhex('4000000000000000 4600000000000000')   # 0x030
    + b'Title\x00Descriptio'                               # 0x040
    + b'n\x00'                                             # 0x042
  ))
  m.pGeneral = 0x30
  assert type(m.general) is GeneralPropertiesSec
  assert m.general.title == 'Title'
  assert m.szGeneral == 0
  m.fixbounds()
  assert m.szGeneral == 0x22

def test_general():
  b = bytearray(bytes.fromhex('1300000000000000 2000000000000000')
                + b'\xFF'*3 + b'Title\x00' + b'\xFF'*7 + b'Description\x00')
  gps = GeneralPropertiesSec(b)
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
                         + b'Unterminated').title
  with pytest.raises(ValueError, match=r'^Unterminated string'):
    GeneralPropertiesSec(bytes.fromhex('1000000000000000 1200000000000000')
                         + b'T\x00Unterminated').description

  assert gps.sectionBounds() == slice(0, len(b))
