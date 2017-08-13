bin_PROGRAMS = dispatcher
noinst_HEADERS = \
configuration.h \
console.h \
database.h \
datastore.h \
dbus/messages.h \
dbus/processor.h \
dispatcher.h \
flag_copier.h \
global_cache.h \
judgment_processor.h \
log_processor.h \
request_processor.h \
requeue.h \
routing_list.h \
statistics.h \
submission_processor.h \
snmp/snmp-config.h \
transfer/core.h

dispatcher_SOURCES = \
configuration.c \
console.c \
console_cli.c \
console_ssh.c \
database.c \
datastore.c \
datastore_uuid.c \
dbus/processor.c \
dbus/rt_announce.c \
dbus/rt_update.c \
dbus/rt_request.c \
dbus/file_delete.c \
dispatcher.c \
flag_copier.c \
global_cache.c \
judgment_processor.c \
log_processor.c \
request_processor.c \
requeue.c \
routing_list.c \
statistics.c \
submission_processor.c \
transfer/core.c \
transfer/ssh.c

if WITH_SNMP
dispatcher_SOURCES += snmp/agent.c
dispatcher_SOURCES += snmp/dataTypesTable.c

endif


dispatcher_LDADD = @DISPATCHERLIBRARIES@  @SNMP_LIBS@
dispatcher_LDFLAGS = @SNMP_LDFLAGS@

AM_CPPFLAGS = @DISPATCHERINCLUDES@ @extra_incl@ @SNMP_CFLAGS@
