pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = pdfDissector.la

pdfDissector_la_SOURCES = pdf_dissector.c
pdfDissector_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
pdfDissector_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

