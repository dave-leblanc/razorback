pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = logNugget.la

logNugget_la_SOURCES = file_log_nugget.c
logNugget_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
logNugget_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

