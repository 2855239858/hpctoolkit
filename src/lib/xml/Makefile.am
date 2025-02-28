# -*-Mode: makefile;-*-

## * BeginRiceCopyright *****************************************************
##
## $HeadURL$
## $Id$
##
## --------------------------------------------------------------------------
## Part of HPCToolkit (hpctoolkit.org)
##
## Information about sources of support for research and development of
## HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
## --------------------------------------------------------------------------
##
## Copyright ((c)) 2002-2022, Rice University
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are
## met:
##
## * Redistributions of source code must retain the above copyright
##   notice, this list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright
##   notice, this list of conditions and the following disclaimer in the
##   documentation and/or other materials provided with the distribution.
##
## * Neither the name of Rice University (RICE) nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## This software is provided by RICE and contributors "as is" and any
## express or implied warranties, including, but not limited to, the
## implied warranties of merchantability and fitness for a particular
## purpose are disclaimed. In no event shall RICE or contributors be
## liable for any direct, indirect, incidental, special, exemplary, or
## consequential damages (including, but not limited to, procurement of
## substitute goods or services; loss of use, data, or profits; or
## business interruption) however caused and on any theory of liability,
## whether in contract, strict liability, or tort (including negligence
## or otherwise) arising in any way out of the use of this software, even
## if advised of the possibility of such damage.
##
## ******************************************************* EndRiceCopyright *

#############################################################################
##
## File:
##   $HeadURL$
##
## Description:
##   *Process with automake to produce Makefile.in*
##
##   Note: All local variables are prefixed with MY to prevent name
##   clashes with automatic automake variables.
##
#############################################################################

# We do not want the standard GNU files (NEWS README AUTHORS ChangeLog...)
AUTOMAKE_OPTIONS = foreign

#############################################################################
# Common settings
#############################################################################

include $(top_srcdir)/src/Makeinclude.config

#############################################################################
# Local settings
#############################################################################

MYSOURCES = \
	xml.hpp xml.cpp

MYDTDHEADERS = \
	$(srcdir)/hpc-experiment.dtd.h \
	$(srcdir)/hpc-structure.dtd.h

MYCFLAGS   = @HOST_CFLAGS@   $(HPC_IFLAGS) 
MYCXXFLAGS = @HOST_CXXFLAGS@ $(HPC_IFLAGS) 

if IS_HOST_AR
  MYAR = @HOST_AR@
else
  MYAR = $(AR) cru
endif

MYLIBADD = @HOST_LIBTREPOSITORY@

MYCLEAN = @HOST_LIBTREPOSITORY@

#############################################################################
# Automake rules
#############################################################################

noinst_HEADERS = $(MYDTDHEADERS)
BUILT_SOURCES = $(MYDTDHEADERS)

noinst_LTLIBRARIES = libHPCxml.la

libHPCxml_la_SOURCES  = $(MYSOURCES)
libHPCxml_la_CFLAGS   = $(MYCFLAGS)
libHPCxml_la_CXXFLAGS = $(MYCXXFLAGS)
libHPCxml_la_AR       = $(MYAR)
libHPCxml_la_LIBADD   = $(MYLIBADD)

MOSTLYCLEANFILES = $(MYCLEAN)

#############################################################################
# Other rules
#############################################################################

# Regenerate DTD headers if DTDs have changed.
$(srcdir)/%.dtd.h : $(top_srcdir)/lib/dtd/%.dtd
	$(srcdir)/makedtdstring $<  > $@

#############################################################################
# Common rules
#############################################################################

include $(top_srcdir)/src/Makeinclude.rules

