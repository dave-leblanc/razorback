bin_PROGRAMS = fsMonitor

noinst_HEADERS = fsmonitor.h
fsMonitor_SOURCES = fsmonitor.c

if BACKEND_INOTIFY
fsMonitor_SOURCES+= inotify_monitor.c
endif

if BACKEND_FAM
fsMonitor_SOURCES+= fam_monitor.c
endif


fsMonitor_LDADD = @NUGGET_LIBRARIES@

AM_CPPFLAGS = @NUGGET_INCLUDES@ @extra_incl@

