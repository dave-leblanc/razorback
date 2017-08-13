pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = smtpParser.la

smtpParser_la_SOURCES = smtp_parser.c
smtpParser_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
smtpParser_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

