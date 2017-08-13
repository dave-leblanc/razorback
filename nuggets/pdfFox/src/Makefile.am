pkglibdir = $(libdir)/razorback

pkglib_LTLIBRARIES = pdfFox.la

noinst_HEADERS = \
foxdebug.h \
foxdecode.h \
foxdetect.h \
foxjsutil.h \
foxobjtab.h \
foxparse.h \
foxrecovery.h \
foxreport.h \
foxsemantics.h \
foxtoken.h

pdfFox_la_SOURCES = \
foxdebug.c \
foxdecode.c \
foxdetect.c \
foxjsutil.c \
foxobjtab.c \
foxparse.c \
foxrecovery.c \
foxreport.c \
foxsemantics.c \
foxtoken.c \
pdfFoxNugget.c

pdfFox_la_LDFLAGS = -module -export-dynamic @XCCFLAGS@ -version-info @NUGGET_VERSION@
pdfFox_la_CFLAGS = -DBUILDING_SO

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@


