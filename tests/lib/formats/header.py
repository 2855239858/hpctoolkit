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
from .exceptions import ValidationError, FutureVersionWarning

from typing import Any, Optional
from warnings import warn

class SectionPointer(FormatSpecification):
  """
    FormatSpecification for a standardized section header.
    Can technically be used alone, but best used in combination with functions
    or methods marked as Sections (see Section below).
  """
  __slots__ = ['_view', '_fn', '_name']
  _view: FormattedBytes
  _fn: Any
  _name: str

  def __init__(self, fn: Any, name: str, *args, **kwargs) -> None:
    super().__init__(*args, **kwargs)
    self._view = FormattedBytes(self._bytes, 'QQ', **kwargs)
    self._fn = fn
    self._name = name

  @FormattedBytes.property
  def ptr(self): return self._view, 1

  @property
  def real(self):
    return self._fn(self._bytes.widened[abs,self.ptr:])
  @real.setter
  def real(self, v):
    if not isinstance(v, FormatSpecification):
      raise TypeError
    self.ptr = v._bytes.range.start

  @property
  def size(self):
    if self._view.isblank(0):
      if self._view.isblank(1):
        raise ValueError(f"Attempt to access blank section: {self._name}")
      self.fixbounds()
    return self._view[0]
  @size.setter
  def size(self, sz: int):
    self._view[0] = sz

  def fixbounds(self) -> None:
    b = self.real.sectionBounds()
    self.size = b.stop - b.start

  def validate(self) -> Any:
    if self.ptr == 0 or self.size == 0:
      raise ValidationError
    return self

class FileHeader(FormatSpecification):
  """
    Format spec subclass for the common bits of all file headers, the magic
    identifier and such.
  """
  __slots__ = ['_view']
  _view: FormattedBytes

  def __init__(self, *args, minor: Optional[int] = None, **kwargs) -> None:
    super().__init__(*args, **kwargs)
    self._view = FormattedBytes(self._bytes, '10s4sBB', **self._kwargs)
    if self._blank:
      self.magic = b'HPCTOOLKIT'
      if self.__class__ is not FileHeader:
        self.format = self._fileType.value
        self.majorVersion = self._majorVersion
        self.minorVersion = self._maxMinorVersion
      if minor is not None:
        self.minorVersion = minor
    elif self.__class__ is not FileHeader:
      if self.minorVersion > self._maxMinorVersion:
        warn(FutureVersionWarning(self.majorVersion, self.minorVersion, self._maxMinorVersion))

  def __init_subclass__(cls, *, filetype: FileType = None, major: int = None,
                        minor: int = None, **kwargs):
    super().__init_subclass__(**kwargs)
    if filetype is None or major is None or minor is None:
      raise ValueError(f"FileHeader must have filetype, major and minor specified!")
    cls._fileType = FileType(filetype)
    cls._majorVersion = int(major)
    cls._maxMinorVersion = int(minor)

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
      form = FileType(form)
    except ValueError:
      raise ValidationError(f"Invalid format: got {form!r}") from None
    if self.__class__ is not FileHeader:
      if self.majorVersion is not self._majorVersion:
        raise ValidationError(f"Invalid major version for {self._fileType.name}: expected {self._majorVersion}, got {self.majorVersion}")
      if form is not self._fileType:
        raise ValidationError(f"Invalid format identifier for {self._fileType.name}: expected {self._fileType.value!r}, got {self.format!r}")
    return self

  def __str__(self):
    if self.__class__ is not FileHeader:
      name = self._fileType.name
      if self.format != self._fileType.value:
        name = f"[not {name}]"
    else:
      name = self.format
      try:
        name = FileType(name).value
      except ValueError:
        pass
    return f"{name}({self.version()})"

  def version(self):
    return f"v{self.majorVersion}.{self.minorVersion}"

  class _Section:
    """
      Clever method descriptor for standardized handling of sections within an
      HPCToolkit file. If the method was named fooBar, the class will have
      3 properties after the hooks fire during class creation:
       - fooBar: the section object itself created by passing a ViewSlice to the
         marked function, and
       - szFooBar and pFooBar: the size and absolute offset of the section itself
         as written in the header.

      The instance attribute _fooBar is used internally to house the
      SectionPointer object for the section itself. The other properties defer to
      this method.
    """
    __slots__ = ['_fn', '_offset', '_minorVer']
    _fn: Any
    _offset: int
    _minorVer: int

    def __init__(self, fn: Any, idx: int, *, minor: int = 0) -> None:
      self._fn = fn
      self._offset = 0x10 + int(idx) * 0x10
      self._minorVer = int(minor)

    def __set_name__(sself, cls, name: str) -> None:
      """
        Hook to adjust the class housing a Section-marked method.
      """
      if not issubclass(cls, FileHeader):
        raise TypeError("Section can only be applied to methods in a FileHeader class!")  # pragma: no cover
      _name = '_'+name
      Name = name[0].upper() + name[1:]

      # Add and overwrite the attributes
      def getsz(self):
        return getattr(self, _name).size
      def setsz(self, v):
        getattr(self, _name).size = v
      setattr(cls, 'sz'+Name, property(getsz, setsz))

      def getptr(self):
        return getattr(self, _name).ptr
      def setptr(self, v):
        getattr(self, _name).ptr = v
      setattr(cls, 'p'+Name, property(getptr, setptr))

      def getreal(self):
        return getattr(self, _name).real
      def setreal(self, v):
        getattr(self, _name).real = v
      setattr(cls, name, property(getreal, setreal))

      # Extend __init__ to fill the new bits
      oldinit = cls.__init__
      def newinit(self, *args, minor: Optional[int] = None, **kwargs):
        oldinit(self, *args, minor=minor, **kwargs)
        setattr(self, _name,
                SectionPointer(lambda x: sself._fn(self, x), name,
                               self._bytes[sself._offset:], **self._kwargs))
      cls.__init__ = newinit

  @staticmethod
  def section(idx: int, *, minor: int = 0):
    idx = int(idx)
    minor = int(minor)
    def apply(fn):
      return FileHeader._Section(fn, idx, minor=minor)
    return apply
  section.__doc__ = _Section.__doc__
