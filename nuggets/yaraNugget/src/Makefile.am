pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = yaraNugget.la

yaraNugget_la_SOURCES = yara.c
yaraNugget_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
yaraNugget_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@ -I$(top_srcdir)/libyara

