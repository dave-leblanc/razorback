lib_LTLIBRARIES = librazorback_api.la

noinst_HEADERS = \
api_internal.h \
binary_buffer.h \
block_pool_private.h \
command_and_control.h \
connected_entity_private.h \
fantasia.h \
init.h \
inspection.h \
judgment_private.h \
local_cache.h \
messages/core.h \
messages/cnc/core.h \
runtime_config.h \
submission_private.h \
transfer/core.h

librazorback_api_la_SOURCES = \
api.c \
binary_buffer.c \
block.c \
block_pool.c \
block_id.c \
command_and_control.c \
config_file.c \
connected_entity.c \
crypto.c \
daemon.c \
event.c \
fantasia.c \
hash.c \
init.c \
inspection.c \
inspector_queue.c \
list.c \
local_cache.c \
lock.c \
log.c \
judgment.c \
json_buffer.c \
metadata.c \
messages/alert_child.c \
messages/alert_primary.c \
messages/block_submit.c \
messages/cache_req.c \
messages/cache_resp.c \
messages/cnc/bye.c \
messages/cnc/cache_clear.c \
messages/cnc/config_ack.c \
messages/cnc/config_update.c \
messages/cnc/error.c \
messages/cnc/go.c \
messages/cnc/hello.c \
messages/cnc/pause.c \
messages/cnc/paused.c \
messages/cnc/reg_req.c \
messages/cnc/reg_resp.c \
messages/cnc/rereg.c \
messages/cnc/running.c \
messages/cnc/term.c \
messages/core.c \
messages/inspection.c \
messages/judgment.c \
messages/log.c \
messages/output_event.c \
messages/output_log.c \
messages/output_inspection.c \
ntvl.c \
nugget.c \
queue.c \
queue_list.c \
response_queue.c \
runtime_config.c \
socket.c \
string_list.c \
submission.c \
thread.c \
thread_pool.c \
timer.c \
transfer/core.c \
transfer/file.c \
transfer/http.c \
transfer/ssh.c \
uuids.c

librazorback_api_la_CFLAGS = -DBUILDING_SO -I$(top_srcdir)/libssh/include -I$(top_srcdir)/include
librazorback_api_la_LDFLAGS = -version-info @APIVERSION@ @XCCFLAGS@ ../libssh/src/libssh.la

AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir)/libssh/include -I$(top_srcdir)/include @extra_incl@
 
