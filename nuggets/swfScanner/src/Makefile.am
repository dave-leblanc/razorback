pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = swfScanner.la

swfScanner_la_SOURCES = swf_scanner.c swf_scanner.h swf_detection.h
swfScanner_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
swfScanner_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

