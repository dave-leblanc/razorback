#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.64])
m4_include([common_m4/versions.m4])
AC_INIT([razorback], RELEASE_VERSION_NUMBER, [https://sourceforge.net/apps/trac/razorbacktm/])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign no-define tar-ustar])
    
   
ac_configure_args="$ac_configure_args --disable-master-nugget-check"

AC_ARG_ENABLE(everything, [AS_HELP_STRING([--enable-everything], [Build and install everything.])], [enable_everything=$enableval], [enable_everything="no"])

API=''
AC_ARG_ENABLE(api, [AS_HELP_STRING([--disable-api], [Do not build and install the api.])], [enable_api=$enableval], [enable_api="yes"])
if test "x$enable_api" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([api])
    # Build the rest of the packages against the packaged API.
    pwd=`pwd`
    ac_configure_args="$ac_configure_args --with-api=${pwd}/api"
    API='api'
    enable_api="yes"
fi
AC_SUBST(API)

DISPATCHER=''
AC_ARG_ENABLE(dispatcher, [AS_HELP_STRING([--enable-dispatcher], [Build and install the dispatcher.])], [enable_dispatcher=$enableval], [enable_dispatcher="no"])
if test "x$enable_dispatcher" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([dispatcher])
    DISPATCHER='dispatcher'
    enable_dispatcher="yes"
fi
AC_SUBST(DISPATCHER)

UI=''
AC_ARG_ENABLE(ui, [AS_HELP_STRING([--enable-ui], [Build and install the ui.])], [enable_ui=$enableval], [enable_ui="no"])
if test "x$enable_ui" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([ui])
    UI='ui'
    enable_ui="yes"
fi
AC_SUBST(UI)


## Start of nugget options

MASTER_NUGGET=''
AC_ARG_ENABLE(masterNugget, [AS_HELP_STRING([--enable-masterNugget], [Build and install the masterNugget.])], [enable_master_nugget=$enableval], [enable_master_nugget="no"])
if test "x$enable_master_nugget" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/masterNugget])
    MASTER_NUGGET='masterNugget'
    enable_master_nugget="yes"
fi
AC_SUBST(MASTER_NUGGET)

ARCHIVE_INFLATE=''
AC_ARG_ENABLE(archiveInflate, [AS_HELP_STRING([--enable-archiveInflate], [Build and install the archiveInflate nugget.])], [enable_archive_inflate=$enableval], [enable_archive_inflate="no"])
if test "x$enable_archive_inflate" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/archiveInflate])
    ARCHIVE_INFLATE='archiveInflate'
    enable_archive_inflate="yes"
fi
AC_SUBST(ARCHIVE_INFLATE)


CLAMAV_NUGGET=''
AC_ARG_ENABLE(clamavNugget, [AS_HELP_STRING([--enable-clamavNugget], [Build and install the clamavNugget nugget.])], [enable_clamav_nugget=$enableval], [enable_clamav_nugget="no"])
if test "x$enable_clamav_nugget" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/clamavNugget])
    CLAMAV_NUGGET='clamavNugget'
    enable_clamav_nugget="yes"
fi
AC_SUBST(CLAMAV_NUGGET)

FILE_INJECT=''
AC_ARG_ENABLE(fileInject, [AS_HELP_STRING([--enable-fileInject], [Build and install the fileInject nugget.])], [enable_file_inject=$enableval], [enable_file_inject="no"])
if test "x$enable_file_inject" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/fileInject])
    FILE_INJECT='fileInject'
    enable_file_inject="yes"
fi
AC_SUBST(FILE_INJECT)

FS_MONITOR=''
AC_ARG_ENABLE(fsMonitor, [AS_HELP_STRING([--enable-fsMonitor], [Build and install the fsMonitor nugget.])], [enable_fs_monitor=$enableval], [enable_fs_monitor="no"])
if test "x$enable_fs_monitor" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/fsMonitor])
    FS_MONITOR='fsMonitor'
    enable_fs_monitor="yes"
fi
AC_SUBST(FS_MONITOR)

FS_WALK=''
AC_ARG_ENABLE(fsWalk, [AS_HELP_STRING([--enable-fsWalk], [Build and install the fsWalk nugget.])], [enable_fs_walk=$enableval], [enable_fs_walk="no"])
if test "x$enable_fs_walk" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/fsWalk])
    FS_WALK='fsWalk'
    enable_fs_walk="yes"
fi
AC_SUBST(FS_WALK)

LOG_NUGGET=''
AC_ARG_ENABLE(logNugget, [AS_HELP_STRING([--enable-logNugget], [Build and install the logNugget nugget.])], [enable_log_nugget=$enableval], [enable_log_nugget="no"])
if test "x$enable_log_nugget" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/logNugget])
    LOG_NUGGET='logNugget'
    enable_log_nugget="yes"
fi
AC_SUBST(LOG_NUGGET)

LIBEMU_NUGGET=''
AC_ARG_ENABLE(libemuNugget, [AS_HELP_STRING([--enable-libemuNugget], [Build and install the libemuNugget nugget.])], [enable_libemu_nugget=$enableval], [enable_libemu_nugget="no"])
if test "x$enable_libemu_nugget" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/libemuNugget])
    LIBEMU_NUGGET='libemuNugget'
    enable_libemu_nugget="yes"
fi
AC_SUBST(LIBEMU_NUGGET)

OFFICE_CAT=''
AC_ARG_ENABLE(officeCat, [AS_HELP_STRING([--enable-officeCat], [Build and install the officeCat nugget.])], [enable_office_cat=$enableval], [enable_office_cat="no"])
if test "x$enable_office_cat" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/officeCat])
    OFFICE_CAT='officeCat'
    enable_office_cat="yes"
fi
AC_SUBST(OFFICE_CAT)


PDF_DISSECTOR=''
AC_ARG_ENABLE(pdfDissector, [AS_HELP_STRING([--enable-pdfDissector], [Build and install the pdfDissector nugget.])], [enable_pdf_dissector=$enableval], [enable_pdf_dissector="no"])
if test "x$enable_pdf_dissector" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/pdfDissector])
    PDF_DISSECTOR='pdfDissector'
    enable_pdf_dissector="yes"
fi
AC_SUBST(PDF_DISSECTOR)

PDF_FOX=''
AC_ARG_ENABLE(pdfFox, [AS_HELP_STRING([--enable-pdfFox], [Build and install the pdfFox nugget.])], [enable_pdf_fox=$enableval], [enable_pdf_fox="no"])
if test "x$enable_pdf_fox" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/pdfFox])
    PDF_FOX='pdfFox'
    enable_pdf_fox="yes"
fi
AC_SUBST(PDF_FOX)

SCRIPT_NUGGET=''
AC_ARG_ENABLE(scriptNugget, [AS_HELP_STRING([--enable-scriptNugget], [Build and install the script nugget.])], [enable_script_nugget=$enableval], [enable_script_nugget="no"])
if test "x$enable_script_nugget" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/scriptNugget])
    SCRIPT_NUGGET='scriptNugget'
    enable_script_nugget="yes"
fi
AC_SUBST(SCRIPT_NUGGET)

SMTP_PARSER=''
#AC_ARG_ENABLE(smtpParser, [AS_HELP_STRING([--enable-smtpParser], [Do not build and install the smtpParser nugget.])], [enable_smtp_parser=$enableval], [enable_smtp_parser="yes"])
#if test "x$enable_smtp_parser" = "xyes"; then
#    AC_CONFIG_SUBDIRS([nuggets/smtpParser])
#    SMTP_PARSER='smtpParser'
#fi
AC_SUBST(SMTP_PARSER)

SWF_SCANNER=''
AC_ARG_ENABLE(swfScanner, [AS_HELP_STRING([--enable-swfScanner], [Build and install the swfScanner nugget.])], [enable_swf_scanner=$enableval], [enable_swf_scanner="no"])
if test "x$enable_swf_scanner" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/swfScanner])
    SWF_SCANNER='swfScanner'
    enable_swf_scanner="yes"
fi
AC_SUBST(SWF_SCANNER)

SYSLOG_NUGGET=''
AC_ARG_ENABLE(syslogNugget, [AS_HELP_STRING([--enable-syslogNugget], [Build and install the syslogNugget nugget.])], [enable_syslog_nugget=$enableval], [enable_syslog_nugget="no"])
if test "x$enable_syslog_nugget" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/syslogNugget])
    SYSLOG_NUGGET='syslogNugget'
    enable_syslog_nugget="yes"
fi
AC_SUBST(SYSLOG_NUGGET)

RAZORD=''
AC_ARG_ENABLE(razord, [AS_HELP_STRING([--enable-razord], [Do not build and install the razord nugget.])], [enable_razord=$enableval], [enable_razord="no"])
if test "x$enable_razord" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/razord])
    RAZORD='razord'
    enable_razord="yes"
fi
AC_SUBST(RAZORD)

VIRUS_TOTAL=''
AC_ARG_ENABLE(virusTotal, [AS_HELP_STRING([--enable-virusTotal], [Build and install the virusTotal nugget.])], [enable_virus_total=$enableval], [enable_virus_total="no"])
if test "x$enable_virus_total" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/virusTotal])
    VIRUS_TOTAL='virusTotal'
    enable_virus_total="yes"
fi
AC_SUBST(VIRUS_TOTAL)

YARA_NUGGET=''
AC_ARG_ENABLE(yaraNugget, [AS_HELP_STRING([--enable-yaraNugget], [Build and install the yaraNugget nugget.])], [enable_yara_nugget=$enableval], [enable_yara_nugget="no"])
if test "x$enable_yara_nugget" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/yaraNugget])
    YARA_NUGGET='yaraNugget'
    enable_yara_nugget="yes"
fi
AC_SUBST(YARA_NUGGET)

## Snort should be last as it does nasty things to the configure args.

SNORT=''
AC_ARG_ENABLE(snort, [AS_HELP_STRING([--enable-snort], [Build and install the snort nugget.])], [enable_snort=$enableval], [enable_snort="no"])
if test "x$enable_snort" = "xyes" -o "x$enable_everything" = "xyes"; then
    AC_CONFIG_SUBDIRS([nuggets/snort])
    ac_configure_args="$ac_configure_args --enable-ipv6 --enable-gre --enable-mpls --enable-targetbased --enable-decoder-preprocessor-rules --enable-ppm --enable-perfprofiling --enable-zlib --enable-active-response --enable-normalizer --enable-reload --enable-react --enable-flexresp3 --enable-rzb-saac --with-librzb-api=${pwd}/api"
    SNORT='snort'
    enable_snort="yes"
fi
AC_SUBST(SNORT)

## End of nugget options
AC_CONFIG_FILES([
Makefile \
nuggets/Makefile \
])

AC_OUTPUT
AC_MSG_RESULT([
------------------------------------------------------------------------
  Components to build:

    api: ............... ${enable_api}
    dispatcher: ........ ${enable_dispatcher}
    UI: ................ ${enable_ui}

  Nuggets to build:
    masterNugget: ...... ${enable_master_nugget}
    archiveInflate: .... ${enable_archive_inflate}
    clamavNugget: ...... ${enable_clamav_nugget}
    fileInject: ........ ${enable_file_inject}
    fsMonitor: ......... ${enable_fs_monitor}
    fsWalk: ............ ${enable_fs_walk}
    libemuNugget: ...... ${enable_libemu_nugget}
    logNugget: ......... ${enable_log_nugget}
    officeCat: ......... ${enable_office_cat}
    pdfDissector: ...... ${enable_pdf_dissector}
    scriptNugget: ...... ${enable_script_nugget}
    snort: ............. ${enable_snort}
    swfScanner: ........ ${enable_swf_scanner}
    syslogNugget: ...... ${enable_syslog_nugget}
    virusTotal: ........ ${enable_virus_total}
    razord: ............ ${enable_razord}
    yaraNugget: ........ ${enable_yara_nugget}

  Building:

    Type 'make' to compile.

    Type 'make install' to install.

------------------------------------------------------------------------
])
