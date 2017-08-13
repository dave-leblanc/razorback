pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = syslogNugget.la

syslogNugget_la_SOURCES = syslog_nugget.c
syslogNugget_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
syslogNugget_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

