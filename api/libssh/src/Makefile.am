SUBDIRS = threads

noinst_LTLIBRARIES = libssh.la

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libssh/include


libssh_la_SOURCES = \
agent.c \
auth1.c \
auth.c \
base64.c \
bind.c \
buffer.c \
callbacks.c \
channels1.c \
channels.c \
client.c \
config.c \
connect.c \
crc32.c \
dh.c \
ecdh.c \
error.c \
gcrypt_missing.c \
getpass.c \
gzip.c \
init.c \
kex1.c \
kex.c \
known_hosts.c \
legacy.c \
libcrypto.c \
log.c \
match.c \
messages.c \
misc.c \
options.c \
packet1.c \
packet.c \
packet_cb.c \
packet_crypt.c \
pcap.c \
pki.c \
pki_crypto.c \
poll.c \
scp.c \
server.c \
session.c \
sftp.c \
sftpserver.c \
socket.c \
string.c \
threads.c \
wrapper.c

libssh_la_LIBADD = threads/libssh_threads.la

