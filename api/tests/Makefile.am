#noinst_PROGRAMS= configTest logTest daemonTest
noinst_PROGRAMS= logTest daemonTest configTest

configTest_SOURCES = configTest.c
configTest_LDADD = -L../src -lrazorback_api @LIBS@

logTest_SOURCES = logTest.c
logTest_LDADD = -L../src -lrazorback_api @LIBS@

#mqTest_SOURCES = mqTest.c
#mqTest_LDADD = -L../src -lrazorback_api 

daemonTest_SOURCES = daemonTest.c
daemonTest_LDADD = -L../src -lrazorback_api @LIBS@

AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir)/include @extra_incl@

