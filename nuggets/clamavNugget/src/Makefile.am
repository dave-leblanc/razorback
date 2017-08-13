pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = clamavNugget.la

clamavNugget_la_SOURCES = ClamAVNugget.c
clamavNugget_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
clamavNugget_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

