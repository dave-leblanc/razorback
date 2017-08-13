#
#  Copyright (C) 2002 - 2007 Tomasz Kojm <tkojm@clamav.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#    $(top_srcdir)/shared/output.c \
#    $(top_srcdir)/shared/output.h \
#    $(top_srcdir)/shared/optparser.c \
#    $(top_srcdir)/shared/optparser.h \
#    $(top_srcdir)/shared/getopt.c \
#    $(top_srcdir)/shared/getopt.h \
#    $(top_srcdir)/shared/misc.c \
#    $(top_srcdir)/shared/misc.h \
#
sbin_PROGRAMS = razord
noinst_HEADERS = \
    fan.h \
    fan-syscalllib.h \
	localserver.h \
    others.h \
    razord.h \
	scanner.h \
    server.h \
    session.h \
    shared.h \
	tcpserver.h \
    thrmgr.h

razord_SOURCES = \
	localserver.c \
    others.c \
    razord.c \
	scanner.c \
    server-th.c \
    session.c \
	tcpserver.c \
    thrmgr.c \
	qsort.c
#    fan.c 

razord_LDADD = @NUGGET_LIBRARIES@

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

