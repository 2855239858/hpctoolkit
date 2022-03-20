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

from .common import FormatSpecification
from .enums import FileType
from .exceptions import ValidationError, FutureVersionWarning
from .header import FileHeader, SectionPointer
from .util import viewof

import pytest
import warnings

def test_magic():
  class ProfDB(FileHeader, filetype=FileType.ProfileDB, major=1, minor=2):
    pass

  # Ensure all the elements are where they're supposed to be
  m = FileHeader(b'ABCDEFGHIJxyzw\x07\x10')
  assert m.magic == b'ABCDEFGHIJ'
  assert m.format == b'xyzw'
  assert m.majorVersion == 0x07
  assert m.minorVersion == 0x10

  # Ensure the default settings from "blank" are right
  m = FileHeader()
  assert m.magic == b'HPCTOOLKIT'
  m = FileHeader(minor=4)
  assert m.magic == b'HPCTOOLKIT'
  assert m.minorVersion == 4
  m = ProfDB()
  assert m.magic == b'HPCTOOLKIT'
  assert m.format == FileType.ProfileDB.value
  assert m.majorVersion == 1
  assert m.minorVersion == 2

  # Ensure changes affect the backend memory
  b = bytearray(0x10)
  m = FileHeader(b)
  m.magic = b'ABCDEFGHIJ'
  m.format = b'xyzw'
  m.majorVersion = 0x07
  m.minorVersion = 0x10
  assert b == b'ABCDEFGHIJxyzw\x07\x10'

  # Ensure string transformations come out ok
  assert str(m) == "b'xyzw'(v7.16)"
  assert m.version() == "v7.16"
  assert str(ProfDB(b'HPCTOOLKITprof\x01\x00')) == "ProfileDB(v1.0)"
  assert str(ProfDB(b'HPCTOOLKITxyzw\x01\x00')) == "[not ProfileDB](v1.0)"

  # Ensure local validations works properly
  with pytest.raises(ValidationError, match=r'^Invalid magic.*ABCDEFGHIJ'):
    FileHeader(b'ABCDEFGHIJxyzw\x07\x10').validate()
  with pytest.raises(ValidationError, match=r'^Invalid format.*xyzw'):
    FileHeader(b'HPCTOOLKITxyzw\x07\x10').validate()
  with pytest.raises(ValidationError, match=r'^Invalid major version'):
    ProfDB(b'HPCTOOLKITprof\x02\x00').validate()
  with pytest.raises(ValidationError, match=r'^Invalid format identifier'):
    ProfDB(b'HPCTOOLKITmeta\x01\x00').validate()
  m = FileHeader(b'HPCTOOLKITmeta\x07\x10')
  assert m.validate() is m
  m = ProfDB(b'HPCTOOLKITprof\x01\x00')
  assert m.validate() is m

  # Ensure the forward compatibility warning is in effect
  with warnings.catch_warnings():
    warnings.simplefilter('error')
    ProfDB(b'HPCTOOLKIT'+FileType.ProfileDB.value+b'\x01\x00')
  with pytest.warns(FutureVersionWarning):
    ProfDB(b'HPCTOOLKIT'+FileType.ProfileDB.value+b'\x01\xFF')

  # Ensure subclass settings must be preset
  with pytest.raises(ValueError):
    class XDB(FileHeader):
      pass

def test_sectionpointer():
  # Check parsing from a non-blank slate
  b = bytearray.fromhex('0700000000000000 1000000000000000 01020304050607')
  sp = SectionPointer(FormatSpecification, 'foo', b)
  assert sp.ptr == 0x10
  assert sp.size == 0x7
  assert type(sp.real) is FormatSpecification
  assert sp.real._bytes.range.start == 0x10
  assert sp.real._bytes.view == bytes.fromhex('01020304050607')
  sp.real = FormatSpecification(viewof(b)[8:])
  with pytest.raises(TypeError):
    sp.real = 42

  # Check blanks handling
  b = bytearray(0x10 + 7)
  class Foo(FormatSpecification):
    def sectionBounds(self): return self._bytes.range
  sp = SectionPointer(Foo, 'foo', b, blank=True)
  with pytest.raises(ValueError, match=r'^Attempt to access blank section: foo$'):
    sp.size
  sp.ptr = 0x10
  assert sp.size == 7

  # Check validation
  def noop(_): raise NotImplementedError  # pragma: no cover
  sp = SectionPointer(noop, 'foo', bytearray(0x10))
  with pytest.raises(ValidationError):
    sp.validate()
  sp.ptr = 0x10
  sp.size = 7
  assert sp.validate() is sp

def test_sections():
  class Bounded(FormatSpecification):
    def sectionBounds(self):
      return slice(self._bytes.range.start, self._bytes.range.start+4)
  class FooBar(FileHeader, filetype=FileType.ProfileDB, major=1, minor=2):
    __slots__ = ['_foo', '_bar']
    @FileHeader.section(0)
    def foo(self, src):
      return Bounded(src, **self._kwargs)
    @FileHeader.section(1)
    def bar(self, src):
      return Bounded(src, **self._kwargs)
  assert '_bar' in FooBar.__slots__

  # Check that parsing works as intended
  b = bytearray(b'HPCTOOLKITprof\x01\x01' + bytes.fromhex('0700000000000000 2000000000000000 0800000000000000 3000000000000000'))
  f = FooBar(b)
  assert type(f._foo) is SectionPointer
  assert type(f._bar) is SectionPointer
  assert f.pFoo == 0x20
  assert f.szFoo == 0x07
  assert f.pBar == 0x30
  assert f.szBar == 0x08
  assert type(f.foo) is Bounded
  assert type(f.bar) is Bounded

  f.szFoo = 0x11
  assert b[0x10] == 0x11
  f.pFoo = 0x39
  assert b[0x18] == 0x39
  f.bar = Bounded(viewof(b)[0x10,10])
  assert f.pBar == 0x10

  f.fixbounds()
  assert f.szFoo == 4
  assert f.szBar == 4
