pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = libemuNugget.la

libemuNugget_la_SOURCES = libemu.c
libemuNugget_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
libemuNugget_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

