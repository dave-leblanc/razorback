pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = archiveInflate.la

archiveInflate_la_SOURCES = archive.c
archiveInflate_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
archiveInflate_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

