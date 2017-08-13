pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = virusTotal.la

virusTotal_la_SOURCES = virustotal.c
virusTotal_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
virusTotal_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

