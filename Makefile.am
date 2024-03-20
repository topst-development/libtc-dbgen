AUTOMAKE_OPTIONS = foreign subdir-objects
CC = @CC@ -Wall -Wno-unused-function
CPP = @CPP@
AM_CFLAGS = $(TCDB_CFLAGS) -I$(top_srcdir)/include
LIBS = @LIBS@ $(TCDB_LIBS)

if AARCH_64
AM_CFLAGS += -DCONFIG_ARCH64
endif

lib_LTLIBRARIES = libTcDBGen.la
libTcDBGen_la_SOURCES = src/sqlite_query.c src/TCDBGen.c src/TCUtil.c src/DBConfig.c
libTcDBGen_la_LDFLAGS = -version-info 1:1:0

include_HEADERS = include/TCDBDefine.h include/TCDBGen.h
