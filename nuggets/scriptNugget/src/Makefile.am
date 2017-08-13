pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = scriptNugget.la

scriptNugget_la_SOURCES = scriptNugget.c
scriptNugget_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
scriptNugget_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

