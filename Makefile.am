AUTOMAKE_OPTIONS = foreign

#ROOTCINT = rootcint

AM_LDFLAGS = -L$(libdir) -L$(ONLINE_MAIN)/lib 

AM_CPPFLAGS = -I$(includedir) 



include_HEADERS = \
  sharedmemory.h \
  oBufferThreaded.h


lib_LTLIBRARIES = \
  libloggerUtils.la 

bin_PROGRAMS = \
  aml  \
  et_send_data \
  et_send_buffers \
  aml_display

aml_SOURCES = \
  aml.cc \
  oBufferThreaded.cc


aml_LDADD = \
  libloggerUtils.la \
  -lNoRootEvent \
  $(databaselibs) -lz \
  -lrt


aml_display_SOURCES = \
  aml_display.cc

aml_display_LDADD = \
  libloggerUtils.la 



aml_LDFLAGS = $(ADDLDFLAGS)




et_send_data_SOURCES = et_send_data.cc
et_send_data_LDADD = \
  -lNoRootEvent \
  -lmessage \
  -lz \
  -lrt

et_send_buffers_SOURCES = et_send_buffers.cc
et_send_buffers_LDADD = \
  -lNoRootEvent \
  -lmessage \
  -lz \
  -lrt


libloggerUtils_la_SOURCES = \
  sharedmemory.cc \
  et_control.cc




libloggerUtils_la_LIBADD = -lNoRootEvent


