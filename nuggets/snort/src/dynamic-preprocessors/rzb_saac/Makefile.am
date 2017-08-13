## $Id
AUTOMAKE_OPTIONS=foreign no-dependencies

INCLUDES = -I../include -I${srcdir}/../libs

pkglibdir = ${exec_prefix}/lib/snort_dynamicpreprocessor

pkglib_LTLIBRARIES = sf_rzb_saac_preproc.la

sf_rzb_saac_preproc_la_LDFLAGS = -shared -export-dynamic -module @XCCFLAGS@
if SO_WITH_STATIC_LIB
PREPROCLIB=../libsf_dynamic_preproc.la
else
nodist_sf_rzb_saac_preproc_la_SOURCES = \
../include/sf_dynamic_preproc_lib.c \
../include/sf_ip.c \
../include/sfPolicyUserData.c
endif
sf_rzb_saac_preproc_la_LIBADD = ${PREPROCLIB} @RAZORBACK_LIBS@
sf_rzb_saac_preproc_la_CFLAGS = @RAZORBACK_CFLAGS@ -Werror

noinst_HEADERS= \
rzb_config.h \
rzb_core.h \
rzb_debug.h \
rzb_http.h \
rzb_includes.h \
rzb_smtp.h \
rzb_smtp-collector.h \
spp_rzb-saac.h

sf_rzb_saac_preproc_la_SOURCES = \
rzb_config.c \
rzb_core.c \
rzb_debug.c \
rzb_http.c \
rzb_http-client.c \
rzb_http-server.c \
rzb_smtp.c \
rzb_smtp-collector.c \
spp_rzb-saac.c

all-local: $(LTLIBRARIES)
	$(MAKE) DESTDIR=`pwd`/../build install-pkglibLTLIBRARIES

