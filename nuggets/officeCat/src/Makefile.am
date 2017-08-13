pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = officeCat.la

officeCat_la_SOURCES = OfficeCatNugget.c
officeCat_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
officeCat_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

